#include "Cache.h"

#include <exception>
#include <iostream>

static inline std::string to_string(int i) {
  // msvc10 bug - need to cast
  return std::to_string((long long)i);
}

bool Cache::insert(const std::string name, void *value, const std::vector<std::string> &deps) {
  if (deps.size() != _deps_size)
    throw std::invalid_argument("Deps size should be " + to_string(_deps_size));

  storage_t::accessor a_storage;
  if (!_storage.insert(a_storage, std::make_pair(name, value)))
    return false;

  // add deps
  key_to_dep_t::accessor a_key_to_dep;
  concurrent_hash_set new_deps_set;
  _key_to_dep.insert(a_key_to_dep, std::make_pair(name, new_deps_set));
  for (auto i = deps.begin(); i != deps.end(); i++)
    a_key_to_dep->second.insert(std::make_pair(*i, false));

  for (auto i = deps.begin(); i != deps.end(); i++) {
    concurrent_hash_set new_keys_set;
    dep_to_key_t::accessor a_dep_to_key;
    _dep_to_key.insert(a_dep_to_key, std::make_pair(*i, new_keys_set));
    a_dep_to_key->second.insert(std::make_pair(name, false));
  }

  return true;
}

bool Cache::remove(const std::string name) {
  storage_t::accessor a_storage;
  if (!_storage.find(a_storage, name))
    return false;

  // remove additional data related to this key from _key_to_dep and _dep_to_key
  key_to_dep_t::accessor a_key_to_dep;
  if (_key_to_dep.find(a_key_to_dep, name)) {
    for (auto j = a_key_to_dep->second.begin(); j != a_key_to_dep->second.end(); j++) {
      auto dep = j->first;
      dep_to_key_t::accessor a_dep_to_key;
      if (_dep_to_key.find(a_dep_to_key, dep))
        a_dep_to_key->second.erase(name);
    }
    _key_to_dep.erase(a_key_to_dep);
  }
  _storage.erase(a_storage);

  return true;
}

void * Cache::find(const std::string name) {
  storage_t::const_accessor a_storage;
  if (!_storage.find(a_storage, name))
    return nullptr;

  return a_storage->second;
}

bool Cache::invalidate_cached_data(const std::string &dep_to_del) {
  dep_to_key_t::accessor a_dep_to_del;
  if (!_dep_to_key.find(a_dep_to_del, dep_to_del))
    return false;

  concurrent_hash_set keys_to_del(a_dep_to_del->second);
  a_dep_to_del.release();

  for (auto i = keys_to_del.begin(); i != keys_to_del.end(); i++)
    remove(i->first);

  return true;
}

void Cache::print() const {
  for (auto i = _storage.begin(); i != _storage.end(); i++)
    std::cout << i->first << "\t";
  std::cout << "\n\n";

  for (auto i = _key_to_dep.begin(); i != _key_to_dep.end(); i++) {
    std::cout << i->first << ": ";
    for (auto j = i->second.begin(); j != i->second.end(); j++)
      std::cout << i->first << "\t";
    std::cout << "\n";
  }

  for (auto i = _dep_to_key.begin(); i != _dep_to_key.end(); i++) {
    std::cout << i->first << ": ";
    for (auto j = i->second.begin(); j != i->second.end(); j++)
      std::cout << i->first << "\t";
    std::cout << "\n";
  }
}