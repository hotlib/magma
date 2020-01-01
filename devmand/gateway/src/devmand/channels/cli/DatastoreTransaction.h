// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/DatastoreState.h>
#include <devmand/devices/cli/ModelRegistry.h>
#include <libyang/libyang.h>
#include <magma_logging.h>
#include <ydk/types.hpp>

using std::make_shared;
using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using ydk::Entity;
using LeafVector = std::vector<pair<string, string>>;
using devmand::channels::cli::DatastoreState;
using devmand::devices::cli::Model;
using devmand::devices::cli::ModelRegistry;

namespace devmand::channels::cli {

class DatastoreTransaction {
 private:
  DatastoreState& datastoreState;
  lyd_node* root = nullptr;
  shared_ptr<ModelRegistry> mreg;
  void validateBeforeCommit();
  static lyd_node* computeRoot(lyd_node* n);
  void writeLeafs(LeafVector& leafs);
  void print();
  void print(lyd_node* nodeToPrint);

  string toJson(lyd_node* initial);

 public:
  DatastoreTransaction(
      DatastoreState& datastoreState,
      const shared_ptr<ModelRegistry> _mreg);

  template <typename T>
  shared_ptr<T> read(string path) {
    ly_set* pSet = lyd_find_path(root, const_cast<char*>(path.c_str()));
    if (pSet->number != 1) {
      throw std::runtime_error("Too many results from path: " + path);
    }
    auto& bundle = mreg->getBundle(Model::OPENCONFIG_0_1_6);

    const shared_ptr<T>& ydkModel = make_shared<T>();
    const string& json = toJson(pSet->set.d[0]);
    MLOG(MINFO) << "json: " << json;
    return std::static_pointer_cast<T>(bundle.decode(json, ydkModel));
  }

  void delete_(string path);
  void createLeafs(shared_ptr<Entity> entity, string init, LeafVector& leafs);
  void write(string path, shared_ptr<Entity> entity);
  void create(shared_ptr<Entity> entity);

  void commit();
};
} // namespace devmand::channels::cli
