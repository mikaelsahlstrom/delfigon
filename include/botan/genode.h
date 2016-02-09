#include <base/lock.h>
#include <base/lock_guard.h>

namespace std
{
  typedef Genode::Lock mutex;

  template <typename LT>
  using lock_guard = Genode::Lock_guard<LT>;
}
