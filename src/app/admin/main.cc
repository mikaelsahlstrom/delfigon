#include <base/printf.h>
#include <os/config.h>
#include <os/server.h>
#include <os/reporter.h>
#include <os/attached_rom_dataspace.h>

using namespace Genode;

namespace Server {
  struct Main;
}

struct Server::Main
{
  Allocator&alloc;
  Entrypoint&ep;
  Reporter reporter{"config"};
  Attached_rom_dataspace dataspace;

  Main(Allocator&alloc, Entrypoint &ep): alloc(alloc), ep(ep), dataspace("state")
  {
    reporter.enabled(true);
    report_config();

    dataspace.sigh(status_dispatcher);
  };

  void report_config(void)
  {
    Reporter::Xml_generator xml(reporter, [&] () {
      xml.attribute("verbose", "yes");
    });
  }

  void handle_status(unsigned)
  {
    dataspace.update();
    if (!dataspace.is_valid())
    {
      PDBG("Not valid\n");
      return;
    }
    PDBG("Valid\n");

    try
    {
      Xml_node state(dataspace.local_addr<char>(), dataspace.size());
      unsigned long bytes = state.attribute_value("bytes_transferred", 0ul);
      unsigned long packets = state.attribute_value("packets_transferred", 0ul);
      PDBG("Packets %lu, bytes %lu\n", packets, bytes);
    }
    catch (...)
    {
      PERR("Xml error.\n");
    }
  }

  Signal_rpc_member<Main> status_dispatcher = { ep, *this, &Main::handle_status };
};

namespace Server
{
  char const *name() { return "admin"; }
  size_t stack_size() { return 2*1024*sizeof(long); }

  void construct(Entrypoint &ep)
  {
    static Main main(*env()->heap(), ep);
  }
}
