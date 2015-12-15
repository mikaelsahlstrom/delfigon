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
    // There is room in the send queue.
    nic_in.tx_channel()->sigh_ready_to_submit (packet_dispatcher);
    // There is a ack in the ack queue.
    nic_in.tx_channel()->sigh_ack_avail       (packet_dispatcher);

    // There is room in the ack queue.
    nic_in.rx_channel()->sigh_ready_to_ack    (packet_dispatcher);
    // There is a packet in the receive queue.
    nic_in.rx_channel()->sigh_packet_avail    (packet_dispatcher);

    // There is room in the send queue.
    nic_out.tx_channel()->sigh_ready_to_submit (packet_dispatcher);
    // There is a ack in the ack queue.
    nic_out.tx_channel()->sigh_ack_avail       (packet_dispatcher);

    // There is room in the ack queue.
    nic_out.rx_channel()->sigh_ready_to_ack    (packet_dispatcher);
    // There is a packet in the receive queue.
    nic_out.rx_channel()->sigh_packet_avail    (packet_dispatcher);
  };

  void handle_packet(unsigned)
  {
    PDBG("Got packet\n");

    for (;;)
    {
      while (nic_in.tx()->ack_avail())
      {
        PDBG("inner release packet\n");
        nic_in.tx()->release_packet(nic_in.tx()->get_acked_packet());
      }

      if (!nic_in.tx()->ready_to_submit())
      {
        return;
      }
      PDBG("inner ready to submit\n");

      if (!nic_out.rx()->ready_to_ack())
      {
        return;
      }
      PDBG("outer ready to ack\n");

      if (!nic_out.rx()->packet_avail())
      {
        return;
      }
      PDBG("outer packet avail\n");

      //size_t const packet_size = nic_out.rx()->peek_packet().size();
      Packet_descriptor packet_for_inner;
      try
      {
        packet_for_inner = nic_in.tx()->alloc_packet(2048);
      }
      catch (...)
      {
        return;
      }
      PDBG("inner tx packet alloced\n");

      Packet_descriptor const packet_from_outer = nic_out.rx()->get_packet();
      Genode::memcpy(nic_in.tx()->packet_content(packet_for_inner),
                     nic_out.rx()->packet_content(packet_from_outer),
                     2048);
      nic_in.tx()->submit_packet(packet_for_inner);
      nic_out.rx()->acknowledge_packet(packet_from_outer);
      PDBG("Packet sent\n");
    }
  }

  Signal_rpc_member<Main> packet_dispatcher = { ep, *this, &Main::handle_packet };
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
