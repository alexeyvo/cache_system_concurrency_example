#include "Cache.h"

#include <exception>

void Cache::insert(const std::string name, void *value, const std::vector<std::string> &deps) {
  if (deps.size() != _deps_size)
    throw std::invalid_argument("Deps size should be " + std::to_string(_deps_size));

  _storage.insert(std::make_pair(name, value));

  auto add_deps = [&](key_to_dep_value_t deps_set) {
    for (auto i = deps.begin(); i != deps.end(); i++)
      deps_set.insert(std::make_pair(*i, false));
  };

  key_to_dep_t::accessor a;
  if (_key_to_dep.find(a, name)) {
    auto deps_set = a->second;
    add_deps(deps_set);
    a.release();
  } else {
    key_to_dep_value_t new_deps_set;
    add_deps(new_deps_set);
    _key_to_dep.insert(std::make_pair(name, new_deps_set));
  }

  for (auto i = deps.begin(); i != deps.end(); i++) {
    dep_to_key_t::accessor a2;
    if (_dep_to_key.find(a2, *i)) {
      auto keys_set = a2->second;
      keys_set.insert(std::make_pair(name, false));
      a2.release();
    } else {
      dep_to_key_value_t new_keys_set;
      new_keys_set.insert(std::make_pair(name, false));
      _dep_to_key.insert(std::make_pair(*i, new_keys_set));
    }
  }
}

bool Cache::invalidate_cached_data(const std::string &dep_to_del) {
  dep_to_key_t::accessor a_keys;
  if (_dep_to_key.find(a_keys, dep_to_del)) {
    auto keys_set = a_keys->second;
    for (auto i = keys_set.begin(); i != keys_set.end(); i++) {
      auto key = i->first;
      _storage.erase(key);
      key_to_dep_t::accessor a_deps;
      if (_key_to_dep.find(a_deps, key)) {
        for (auto j = a_deps->second.begin(); j != a_deps->second.end(); j++) {
          auto dep = j->first;
          if (dep != dep_to_del) {
            dep_to_key_t::accessor a3;
            if (_dep_to_key.find(a3, dep)) {
              a3->second.erase(key);
              a3.release();
            }
          }
        }
        a_deps.release();
      }
    }
    a_keys.release();
    _dep_to_key.erase(dep_to_del);
    return true;
  }

  return false;
}