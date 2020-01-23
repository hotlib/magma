// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/datastore/DatastoreState.h>
#include <devmand/devices/cli/schema/ModelRegistry.h>
#include <devmand/devices/cli/schema/Path.h>
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
using ListKeys = std::vector<string>;
using devmand::channels::cli::datastore::DatastoreState;
using devmand::devices::cli::Model;
using devmand::devices::cli::ModelRegistry;
using devmand::devices::cli::Path;
using folly::dynamic;
using folly::parseJson;
using std::atomic_bool;

namespace devmand::channels::cli::datastore {

class DatastoreTransaction {
 private:
  shared_ptr<DatastoreState> datastoreState;
  lllyd_node* root = nullptr;
  atomic_bool hasCommited = ATOMIC_VAR_INIT(false);
  void validateBeforeCommit();
  static lllyd_node* computeRoot(lllyd_node* n);
  int datastoreTypeToLydOption();
  lllyd_node* dynamic2lydNode(dynamic entity);
  void print();
  static void print(LeafVector& v);
  static void printDiffType(LLLYD_DIFFTYPE type);
  void print(lllyd_node* nodeToPrint);
  void checkIfCommitted();
  string toJson(lllyd_node* initial);
  static dynamic appendAllParents(Path path, const dynamic& aDynamic);

 public:
  DatastoreTransaction(shared_ptr<DatastoreState> datastoreState);

  dynamic read(Path path);

  void diff();
  bool isValid();
  void delete_(Path path);
  void merge(Path path, const dynamic& aDynamic);
  void write(Path path, const dynamic& aDynamic);
  void commit();
  void abort();

  virtual ~DatastoreTransaction();
};
} // namespace devmand::channels::cli::datastore
