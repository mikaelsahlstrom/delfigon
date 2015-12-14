#include <nic_session/connection.h>
#include <nic/packet_allocator.h>
#include <base/allocator_avl.h>
#include <base/printf.h>
#include <os/server.h>

using namespace Genode;

namespace Server {
  struct Main;
}

struct Server::Main
{
  Allocator&alloc;
  Entrypoint&ep;
  enum { BUF_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE * 128 };

  Allocator_avl inner_alloc{&alloc};
  Nic::Connection nic_in{&inner_alloc, BUF_SIZE, BUF_SIZE, "in"};

  Allocator_avl outer_alloc{&alloc};
  Nic::Connection nic_out{&outer_alloc, BUF_SIZE, BUF_SIZE, "out"};

  Main(Allocator&alloc, Entrypoint &ep) : alloc(alloc), ep(ep)
  {
    nic_in.tx_channel()->sigh_ready_to_submit (packet_dispatcher);
    nic_in.tx_channel()->sigh_ack_avail       (packet_dispatcher);
    nic_in.rx_channel()->sigh_ready_to_ack    (packet_dispatcher);
    nic_in.rx_channel()->sigh_packet_avail    (packet_dispatcher);

    nic_out.tx_channel()->sigh_ready_to_submit (packet_dispatcher);
    nic_out.tx_channel()->sigh_ack_avail       (packet_dispatcher);
    nic_out.rx_channel()->sigh_ready_to_ack    (packet_dispatcher);
    nic_out.rx_channel()->sigh_packet_avail    (packet_dispatcher);
  };

  void handle_packet(unsigned)
  {
    PDBG("Got packet\n");
  }

  Signal_rpc_member<Main> packet_dispatcher =
        { ep, *this, &Main::handle_packet};
};

namespace Server
{
  char const *name() { return "nic_diode"; }
  size_t stack_size() { return 2*1024*sizeof(long); }

  void construct(Entrypoint &ep)
  {
    static Main main(*env()->heap(), ep);
  }
}
