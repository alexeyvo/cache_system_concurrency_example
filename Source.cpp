#include <string>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>

#include "Cache.h"

/* example runs 1 thread that invalidates cache and multimple threads that insers data 
   just to test concurrency */

const uint8_t DEPS_SIZE = 2,
              INSERT_THREADS = 2;

Cache c(DEPS_SIZE);
boost::atomic<bool> done(false);
boost::atomic<size_t> inserts(0), removes(0);

std::string gen_user_dep() {
  return "users." + std::to_string(rand() % 64);
}

std::string gen_domain_dep() {
  return "domains." + std::to_string(rand() % 8);
}

int main()
{
  boost::thread cache_invalidator([] {
    while (!done) {
      if (c.invalidate_cached_data(gen_user_dep()))
        removes++;
    }
  });

  boost::thread_group cache_inserters;
  for (auto i = 0; i != INSERT_THREADS; ++i)
    cache_inserters.create_thread([] {
      while (!done) {
        std::vector<std::string> deps(DEPS_SIZE);
        deps[0] = gen_user_dep();
        deps[1] = gen_domain_dep();

        // cache null data, we don't care for now
        c.insert(std::to_string(rand()), nullptr, deps);

        inserts++;
      }
    });

  boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
  done = true;
  cache_inserters.join_all();
  cache_invalidator.join();

  std::cout << inserts << "\t" << removes << "\n";

  return 0;
}