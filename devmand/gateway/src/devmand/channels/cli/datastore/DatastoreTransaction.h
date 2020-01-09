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
using ListKeys = std::vector<string>;
using devmand::channels::cli::datastore::DatastoreState;
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
  static string getData(const dynamic& d);
  void writeLeafs(LeafVector& leafs);
  void print();
  static void print(LeafVector& v);
  static void printDiffType(LYD_DIFFTYPE type);
  std::vector<string> fixSegments(std::vector<string> str);
  void print(lyd_node* nodeToPrint);
  void checkIfCommitted();
  string toJson(lyd_node* initial);
  void traverseDynamic(
      string currentPath,
      const dynamic& aDynamic,
      LeafVector& leafs);
  static bool isCompositeType(const dynamic& aDynamic);
  // static ListKeys lysGetKeys(struct lys_node * node);
  lys_node_list* mightHaveKeys(string path);

 public:
  DatastoreTransaction(shared_ptr<DatastoreState> datastoreState);

  dynamic read(string path);

  void diff();
  void delete_(string path);
  void write(LeafVector& leafs);
  void write(const string path, const dynamic& aDynamic);
  void commit();

  virtual ~DatastoreTransaction();
};
} // namespace devmand::channels::cli::datastore
