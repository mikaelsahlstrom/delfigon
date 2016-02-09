#include <botan/filters.h>

using namespace Botan;

int main(void)
{
  Pipe pipe(new Hash_Filter("SHA-256"), new Hex_Encoder);
  pipe.process_msg("test");
  return 0;
}
