#include "Cache.h"

#include <exception>

void Cache::insert(const std::string name, void *value, const std::vector<std::string> &deps) {
  if (deps.size() != _deps_size)
    throw std::invalid_argument("Deps size should be " + std::to_string(_deps_size));

  _storage.insert(std::make_pair(name, value));

  auto add_deps = [&](key_to_dep_value_t &deps_set) {
    for (auto i = deps.begin(); i != deps.end(); i++)
      deps_set.insert(std::make_pair(*i, false));
  };

  key_to_dep_t::accessor a_key_to_dep;
  if (_key_to_dep.find(a_key_to_dep, name)) {
    auto deps_set = a_key_to_dep->second;
    add_deps(deps_set);
    a_key_to_dep.release();
  } else {
    key_to_dep_value_t new_deps_set;
    add_deps(new_deps_set);
    _key_to_dep.insert(std::make_pair(name, new_deps_set));
  }

  for (auto i = deps.begin(); i != deps.end(); i++) {
    dep_to_key_t::accessor a_dep_to_key;
    if (_dep_to_key.find(a_dep_to_key, *i)) {
      auto keys_set = a_dep_to_key->second;
      keys_set.insert(std::make_pair(name, false));
    } else {
      dep_to_key_value_t new_keys_set;
      new_keys_set.insert(std::make_pair(name, false));
      _dep_to_key.insert(std::make_pair(*i, new_keys_set));
    }
  }
}

bool Cache::invalidate_cached_data(const std::string &dep_to_del) {
  dep_to_key_t::accessor a_dep_to_del;
  if (!_dep_to_key.find(a_dep_to_del, dep_to_del))
    return false;

  auto dep_to_del_keys_set = a_dep_to_del->second;
  for (auto i = dep_to_del_keys_set.begin(); i != dep_to_del_keys_set.end(); i++) {
    auto key = i->first;
    // remove cached data from storage cause it's depends on dep_to_del
    _storage.erase(key);
    // remove additional data related to this key from _key_to_dep and _dep_to_key
    key_to_dep_t::accessor a_key_to_dep;
    if (_key_to_dep.find(a_key_to_dep, key)) {
      for (auto j = a_key_to_dep->second.begin(); j != a_key_to_dep->second.end(); j++) {
        auto dep = j->first;
        if (dep != dep_to_del) { // we already know dep_to_del should be removed
          dep_to_key_t::accessor a_dep_to_key;
          if (_dep_to_key.find(a_dep_to_key, dep))
            a_dep_to_key->second.erase(key);
        }
      }
      _key_to_dep.erase(a_key_to_dep);
    }
  }
  _dep_to_key.erase(a_dep_to_del);

  return true;
}