// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/datastore/DatastoreState.h>
#include <devmand/devices/cli/ModelRegistry.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <libyang/libyang.h>
#include <magma_logging.h>
#include <ydk/types.hpp>
#include <atomic>

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
using folly::dynamic;
using folly::parseJson;
using std::atomic_bool;

namespace devmand::channels::cli::datastore {

class DatastoreTransaction {
 private:
  shared_ptr<DatastoreState> datastoreState;
  lyd_node* root = nullptr;
  atomic_bool hasCommited = ATOMIC_VAR_INIT(false);
  void validateBeforeCommit();
  static lyd_node* computeRoot(lyd_node* n);
  void writeLeafs(LeafVector& leafs);
  void print();
  static void printDiffType(LYD_DIFFTYPE type);
  void print(lyd_node* nodeToPrint);
  void checkIfCommitted();
  string toJson(lyd_node* initial);

 public:
  DatastoreTransaction(shared_ptr<DatastoreState> datastoreState);

  dynamic read(string path) {
    checkIfCommitted();

    ly_set* pSet = lyd_find_path(root, const_cast<char*>(path.c_str()));
    if (pSet->number != 1) {
      throw std::runtime_error("Too many results from path: " + path);
    }

    const string& json = toJson(pSet->set.d[0]);
    MLOG(MINFO) << "json: " << json;
    return parseJson(json);
  }

  void diff();
  void delete_(string path);
  void write(LeafVector & leafs);

  void commit();

  virtual ~DatastoreTransaction();
};
} // namespace devmand::channels::cli::datastore
