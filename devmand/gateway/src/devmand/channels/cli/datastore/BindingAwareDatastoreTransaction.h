// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/datastore/DatastoreState.h>
#include <devmand/channels/cli/datastore/DatastoreTransaction.h>

namespace devmand::channels::cli::datastore {
using LeafVector = std::vector<pair<string, string>>;

class BindingAwareDatastoreTransaction {
 private:
  DatastoreTransaction datastoreTransaction;
  shared_ptr<ModelRegistry> mreg;
  atomic_bool hasCommited = ATOMIC_VAR_INIT(false);
  void createLeafs(shared_ptr<Entity> entity, string init, LeafVector& leafs);

 public:
  BindingAwareDatastoreTransaction(
      shared_ptr<DatastoreState> datastoreState,
      shared_ptr<ModelRegistry> mreg);

 public:
//  template <typename T>
//  shared_ptr<T> read(string path) {
//    auto& bundle = mreg->getBundle(Model::OPENCONFIG_0_1_6);
//    const shared_ptr<T>& ydkModel = make_shared<T>();
//    string json = folly::toJson(datastoreTransaction.read(path));
//    return std::static_pointer_cast<T>(bundle.decode(json, ydkModel));
//  }
  void diff();
  void delete_(string path);
  void write(string path, shared_ptr<Entity> entity);
  void create(shared_ptr<Entity> entity);

  void commit();
};

} // namespace devmand::channels::cli::datastore
