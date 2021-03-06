#include <iostream>
#include <tbb/tbb_thread.h>
#include <tbb/tick_count.h>
#include <tbb/atomic.h>

#include "cache.h"

/* example runs 1 thread that invalidates cache and multimple threads that insers data 
   just to test concurrency */

const uint8_t DEPS_SIZE = 2,
              INSERT_THREADS = 2;

cnv::cache c(DEPS_SIZE);
int cached_data_stub = 1;
tbb::atomic<bool> done;
tbb::atomic<size_t> inserts, invalidations, finds;

static inline std::string to_string(int i) {
  // msvc10 bug - need to cast
  return std::to_string((long long)i);
}

std::string gen_user_dep() {
  return "users." + to_string(rand() % 1024);
}

std::string gen_domain_dep() {
  return "domains." + to_string(rand() % 8);
}

std::string gen_name() {
  return to_string(rand());
}

int main()
{
  tbb::tbb_thread cache_invalidator([] {
    while (!done) {
      if (c.invalidate_cached_data(gen_domain_dep()))
        invalidations++;
    }
  });

  std::vector<tbb::tbb_thread> cache_inserters;
  for (auto i = 0; i != INSERT_THREADS; ++i)
    cache_inserters.push_back(tbb::tbb_thread([] {
      while (!done) {
        std::vector<std::string> deps(DEPS_SIZE);
        deps[0] = gen_user_dep();
        deps[1] = gen_domain_dep();

        if (c.insert(gen_name(), &cached_data_stub, deps))
          inserts++;
      }
    }));

  std::vector<tbb::tbb_thread> cache_finders;
  for (auto i = 0; i != 2; ++i)
    cache_finders.push_back(tbb::tbb_thread([] {
      while (!done)
        if (c.find(gen_name()) != nullptr)
          finds++;
    }));

  tbb::this_tbb_thread::sleep(tbb::tick_count::interval_t(5.0));
  done = true;
  for (auto i = cache_inserters.begin(); i != cache_inserters.end(); i++)
    i->join();
  for (auto i = cache_finders.begin(); i != cache_finders.end(); i++)
    i->join();
  cache_invalidator.join();

  std::cout << "inserts: " << inserts << "\tfinds: " << finds << "\tinvalidations: " << invalidations << "\n";

  for (size_t i = 0; i != 8; i++)
    c.invalidate_cached_data("domains." + to_string(i));

  c.check();

  return 0;
}