#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>

#include "Cache.h"

/* example runs 1 thread that invalidates cache and multimple threads that insers data 
   just to test concurrency */

const uint8_t DEPS_SIZE = 2,
              INSERT_THREADS = 2;

Cache c(DEPS_SIZE);
int cached_data_stub = 1;
boost::atomic<bool> done(false), stop_insert(false);
boost::atomic<size_t> inserts(0), invalidations(0), removes(0), finds(0);

std::string gen_user_dep() {
  return "users." + std::to_string(rand() % 64);
}

std::string gen_domain_dep() {
  return "domains." + std::to_string(rand() % 8);
}

std::string gen_name() {
  return std::to_string(rand() % 512);
}

int main()
{
  boost::thread cache_invalidator([] {
    while (!done)
      if (c.invalidate_cached_data(gen_user_dep()) && !stop_insert)
        invalidations++;
  });

  boost::thread_group cache_inserters;
  for (auto i = 0; i != INSERT_THREADS; ++i)
    cache_inserters.create_thread([] {
      while (!stop_insert) {
        std::vector<std::string> deps(DEPS_SIZE);
        deps[0] = gen_user_dep();
        deps[1] = gen_domain_dep();

        if (c.insert(gen_name(), &cached_data_stub, deps))
          inserts++;
      }
    });

  boost::thread_group cache_removers;
  for (auto i = 0; i != 1; ++i)
    cache_removers.create_thread([] {
      while (!done)
        if (c.remove(gen_name()) && !stop_insert)
          removes++;
    });

  boost::thread_group cache_finders;
  for (auto i = 0; i != 2; ++i)
    cache_finders.create_thread([] {
      while (!done)
        if (c.find(gen_name()) != nullptr && !stop_insert)
          finds++;
    });

  boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
  stop_insert = true;
  cache_inserters.join_all();

  boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
  done = true;
  cache_invalidator.join();
  cache_removers.join_all();
  cache_finders.join_all();

  std::cout << "inserts: " << inserts << "\tremoves: " << removes << "\tfinds: " 
            << finds << "\tinvalidations: " << invalidations << "\n";

  c.print();

  return 0;
}