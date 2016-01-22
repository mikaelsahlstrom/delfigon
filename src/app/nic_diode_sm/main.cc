#include <nic_session/connection.h>
#include <nic/packet_allocator.h>
#include <base/allocator_avl.h>
#include <base/printf.h>
#include <os/server.h>
#include <os/config.h>
#include <os/reporter.h>
#include <timer_session/connection.h>

#include "state_machine/hsm.h"

namespace Server
{
	char const * name();
	size_t       stack_size();
	void         construct(Entrypoint &);

	enum class Event;
	class      Diode;

	/****************************************************************************
	 ** State definitions
	 ****************************************************************************/
	typedef hsm::state::Composite<Diode, 0>            Top;
	typedef hsm::state::Composite<Diode, 1, Top>       Operating;
	typedef hsm::state::Leaf     <Diode, 1, Operating> Blocking;
	typedef hsm::state::Leaf     <Diode, 1, Operating> Streaming;
}

/******************************************************************************
 ** Event definition
 ******************************************************************************/
enum class Server::Event
{
	CONFIG_UPDATED,   /* Configuration file is updated */
	REPORT_STATUS,    /* Request for status update generated */
	US_PKT_SUBMITTED, /* Upstream packet source has submitted packet */
	US_ACK_EXTRACTED, /* Upstream packet source has extracted ack */
	DS_PKT_EXTRACTED, /* Downstream packet sink has extracted packet */
	DS_ACK_SUBMITTED  /* Downstream packet sink has submitted ack */
};

/******************************************************************************
 ** Diode declaration
 ******************************************************************************/
class Server::Diode : public hsm::Basic<Server::Diode, Server::Event>
{
	typedef typename hsm::Basic<Server::Diode, Server::Event> Base;
	
	struct Stream_port
	{
		Stream_port(Allocator &, Genode::size_t tx_buf_sz,
						Genode::size_t rx_buf_sz, const char * label);

		Allocator_avl   alloc;
		Nic::Connection nic;
	};

	struct Upstream_port : public Stream_port
	{
		Upstream_port(Allocator &, Genode::size_t, const char * label);
	};
	
	struct Downstream_port : public Stream_port
	{
		Downstream_port(Allocator &, Genode::size_t, const char * label);
	};

	static const size_t BUF_PKT_SZ = 128;
	static const size_t BUF_SZ =
		static_cast<unsigned>(Nic::Packet_allocator::DEFAULT_PACKET_SIZE)
		*  BUF_PKT_SZ;

	void         connect_signals();
	template<Event E>
	void         dispatch(unsigned);

	Reporter          _reporter;
	Genode::size_t    _bytes_streamed;
	Genode::size_t    _pkts_streamed;
	unsigned          _nsignals;
	
	Signal_rpc_member<Diode> _config_updated;
	Signal_rpc_member<Diode> _report_status;
	Signal_rpc_member<Diode> _us_pkt_submitted;
	Signal_rpc_member<Diode> _us_ack_extracted;
	Signal_rpc_member<Diode> _ds_pkt_extracted;
	Signal_rpc_member<Diode> _ds_ack_submitted;

public:
	struct Config
	{
		Config();
	
		bool     blocking;
		bool     verbose;
		unsigned report_period;
	};
	
	Diode(Allocator &, Entrypoint &);

	void update_config();
	void report_status();
	void receive_acks();
	void forward_pkts();

	Config            cfg;
	Timer::Connection status_timer; /* Triggers periodic report requests */
	Upstream_port     usp;          /* Upstream port */
	Downstream_port   dsp;          /* Downstream port */
};

/******************************************************************************
 ** Server definitions
 ******************************************************************************/
char const * Server::name()
{
	return "nic_diode_sm";
}

Genode::size_t Server::stack_size()
{
	return 2 * 1024 * sizeof(long);
}

void Server::construct(Entrypoint & ep)
{	
	 PDBG("Diode not constructed yet \n");
	 static Diode diode(* env()->heap(), ep);
	 PDBG("Diode constructed\n");
}

/******************************************************************************
 ** Stream_port definition
 ******************************************************************************/
Server::Diode::Stream_port::Stream_port(Allocator & a,
													 Genode::size_t upstream_buf_sz,
													 Genode::size_t downstream_buf_sz,
													 const char *   label)
	
	: alloc (& a),
	  nic   (& alloc, upstream_buf_sz, downstream_buf_sz, label)
{}

Server::Diode::Upstream_port::Upstream_port(Allocator & a,
														  Genode::size_t buf_sz,
														  const char * label)
	: Stream_port(a, 0, buf_sz, label)
{}

Server::Diode::Downstream_port::Downstream_port(Allocator & a,
																Genode::size_t buf_sz,
																const char * label)
	: Stream_port(a, buf_sz, 0, label)
{}

/******************************************************************************
 ** Config definition
 ******************************************************************************/
Server::Diode::Config::Config()
	: blocking      (true),
	  verbose       (false),
	  report_period (10000000)
{}

/******************************************************************************
 ** Diode definition
 ******************************************************************************/
void Server::Diode::connect_signals()
{
	PDBG("Server::Diode::connect_signals\n");
   config()->sigh(_config_updated);	
	status_timer.sigh(_report_status);	
	usp.nic.rx_channel()->sigh_packet_avail   (_us_pkt_submitted);
	usp.nic.rx_channel()->sigh_ready_to_ack   (_us_ack_extracted);
	dsp.nic.tx_channel()->sigh_ready_to_submit(_ds_pkt_extracted);
	dsp.nic.tx_channel()->sigh_ack_avail      (_ds_ack_submitted);
}	

template<Server::Event E>
void Server::Diode::dispatch(unsigned nsignals)
{
	PDBG("Server::Diode::dispatch; %d\n", static_cast<unsigned>(E));
	_nsignals = nsignals;
	Base::dispatch(E);
}

Server::Diode::Diode(Allocator & a, Entrypoint & ep)
	: _reporter         ("status"),
	  _bytes_streamed   (0),
	  _pkts_streamed    (0),
	  _nsignals         (0),
	  _config_updated   (ep, * this, & Diode::dispatch<Event::CONFIG_UPDATED>),
	  _report_status    (ep, * this, & Diode::dispatch<Event::REPORT_STATUS>),
	  _us_pkt_submitted (ep, * this, & Diode::dispatch<Event::US_PKT_SUBMITTED>),
	  _us_ack_extracted (ep, * this, & Diode::dispatch<Event::US_ACK_EXTRACTED>),
	  _ds_pkt_extracted (ep, * this, & Diode::dispatch<Event::DS_PKT_EXTRACTED>),
	  _ds_ack_submitted (ep, * this, & Diode::dispatch<Event::DS_ACK_SUBMITTED>),
	  cfg               (),
	  status_timer      (),
	  usp               (a, BUF_SZ, "out"),
	  dsp               (a, BUF_SZ, "in")
{
	connect_signals();
};

void Server::Diode::update_config()
{
	config()->reload();
	cfg.blocking = config()->xml_node().attribute_value("blocking", false);
	cfg.verbose = config()->xml_node().attribute_value("verbose", false);
	cfg.report_period = config()->xml_node().attribute_value("report_interval", 1000000u);
	status_timer.trigger_periodic(cfg.report_period);
	PDBG("blocking: %d\n", cfg.blocking);
	PDBG("verbose: %d\n", cfg.verbose);
	PDBG("report_interval: %d\n", cfg.report_period);
}

void Server::Diode::report_status()
{
	Reporter::Xml_generator xml(_reporter, [&] () {
			xml.attribute("StreamedBytes", _bytes_streamed);
			xml.attribute("StreamedPkts", _pkts_streamed);
		});
}

void Server::Diode::receive_acks()
{
	while (dsp.nic.tx()->ack_avail())	{
		PDBG("downstream release packet\n");
		dsp.nic.tx()->release_packet(dsp.nic.tx()->get_acked_packet());
	}
}

void Server::Diode::forward_pkts()
{
	while (usp.nic.rx()->packet_avail()
			 && usp.nic.rx()->ready_to_ack()
			 && dsp.nic.tx()->ready_to_submit()) {
		Packet_descriptor const us_pkt = usp.nic.rx()->get_packet();
		Packet_descriptor ds_pkt;
		try {
			ds_pkt = dsp.nic.tx()->alloc_packet(us_pkt.size());
		}
		catch (...) {
			return;
		}
		Genode::memcpy(dsp.nic.tx()->packet_content(ds_pkt),
							usp.nic.rx()->packet_content(us_pkt), us_pkt.size());
		dsp.nic.tx()->submit_packet(ds_pkt);
		usp.nic.rx()->acknowledge_packet(us_pkt);
		_pkts_streamed++;
		_bytes_streamed += us_pkt.size();
	}
}

namespace hsm { namespace state {
/******************************************************************************
 ** Top state definition
 ******************************************************************************/
	template<>
	inline void Server::Top::handle_init(Server::Diode & arg)
	{
		PDBG("Server::Top::handle_init\n");
		Init<Server::Operating> t(arg);
	}

/******************************************************************************
 ** Operating definition
 ******************************************************************************/
	template<>
	inline void Server::Operating::handle_init(Server::Diode & arg)
	{
		PDBG("Server::Operating::handle_init\n");
		arg.update_config();
		if (arg.cfg.blocking) {
			Init<Server::Blocking> t(arg);
		} else {
			Init<Server::Streaming> t(arg);
		}
	}

	template<>
	template<typename LEAF>
	inline void
	Server::Operating::handle_event(Server::Diode & h, const LEAF & l) const
	{
		PDBG("Server::Operating::handle_init\n");
		using hsm::state::Transition;
		switch(h.get_event()) {
		case Server::Event::REPORT_STATUS:
		{
			h.report_status();
			return;
		}
		default:
			break;
		}
		Parent::handle_event(h, l);
	}

/******************************************************************************
 ** Blocking definition
 ******************************************************************************/
	template<>
	template<typename LEAF>
	inline void
	Server::Blocking::handle_event(Server::Diode & h, const LEAF & l) const
	{
		PDBG("Server::Blocking::handle_event\n");
		switch(h.get_event()) {
		case Server::Event::CONFIG_UPDATED:
		{
			h.update_config();
			if (! h.cfg.blocking) {
				hsm::state::Transition<LEAF, This, Server::Streaming> t(h);
			}
			return;
		}
		default:
			break;
		}
		Parent::handle_event(h, l);
	}

/******************************************************************************
 ** Streaming definition
 ******************************************************************************/
	template<>
	template<typename LEAF>
	inline void
	Server::Streaming::handle_event(Server::Diode & h, const LEAF & l) const
	{
		PDBG("Server::Streaming::handle_event\n");
		switch(h.get_event()) {
		case Server::Event::CONFIG_UPDATED:
		{
			h.update_config();
			if (h.cfg.blocking) {
				hsm::state::Transition<LEAF, This, Server::Blocking> t(h);
			}
			return;
		}
		case Server::Event::DS_ACK_SUBMITTED:
		{
			h.receive_acks();
			return;
		}
		case Server::Event::US_PKT_SUBMITTED:
		case Server::Event::US_ACK_EXTRACTED:
		case Server::Event::DS_PKT_EXTRACTED:
		{
			h.forward_pkts();
			return;
		}
		default:
			break;
		}
		Parent::handle_event(h, l);
	}
}
}
