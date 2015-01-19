#pragma once

#include <vector>
#include <tbb/concurrent_hash_map.h>

class Cache
{
  typedef tbb::concurrent_hash_map<std::string, void *> storage_t;
  // we use bool type as stub cause de don't care about values they always false(there is no concurrent_hash_set class)
  // we just need to keep set of strings for each string key
  typedef tbb::concurrent_hash_map<std::string, bool> concurrent_hash_set;
  typedef tbb::concurrent_hash_map<std::string, concurrent_hash_set> key_to_dep_t;
  typedef tbb::concurrent_hash_map<std::string, concurrent_hash_set> dep_to_key_t;

  /* keeps cached data for each key */
  storage_t _storage;
  /* keeps all keys for each dep know what data to invalidate when single dep changes */
  dep_to_key_t _dep_to_key;
  /* keeps all deps for each key to track invalid entries inside _dep_to_key and remove them */
  key_to_dep_t _key_to_dep;
  /* for example for key a calculated from dep1 and dep2:
     _dep_to_key[dep1] = { a }
     _dep_to_key[dep2] = { a }
     _storage[a] = cached_data_ptr
     _key_to_dep[a] = { dep1, dep2 }
     if dep1 changes we 
       1. find keys that should be invalidated in _dep_to_key by dep1(result is a)
       2. remove cached data from _storage by key a
       3. we need to remove all a values from _dep_to_key cause they are invalid,
          so we peek(remove and remember) their keys from _key_to_dep(result is dep1, and dep2)
       4. we remove entries with key dep1 and value a, with key dep2 and value a from _dep_to_key
  */

  // trying to insert data with different deps size is programming error
  uint8_t _deps_size;
public:
  Cache(uint8_t deps_size) : _deps_size(deps_size) {}
  ~Cache() {}
  bool insert(const std::string name, void *value, const std::vector<std::string> &deps);
  bool remove(const std::string name);
  void * find(const std::string name);
  // this will be private, for now will be public
  bool invalidate_cached_data(const std::string &dep);
  void print() const;
};
