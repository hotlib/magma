// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <libyang/libyang.h>
#include <atomic>
#include <map>
#include <vector>

namespace devmand::channels::cli::datastore {
using std::atomic_bool;
using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::vector;
typedef std::map<string, lllyd_node*> LydMap;
typedef pair<string, lllyd_node*> LydPair;
typedef pair<lllyd_node*, lllyd_node*> RootPair;

enum DatastoreType { config, operational };

struct DatastoreState {
  atomic_bool transactionUnderway = ATOMIC_VAR_INIT(false);
  llly_ctx* ctx = nullptr;
  DatastoreType type;
  LydMap forest;
  LydMap forestInTransaction;

 public:
  lllyd_node* getData(string moduleName, LydMap& source);
  void setData(string moduleName, lllyd_node* newValue, LydMap& source);
  void freeData(LydMap& source);
  lllyd_node* computeRoot(lllyd_node* n);
  void duplicateForTransaction();
  void setRootFromTransaction();
  vector<RootPair> getRootAndTransactionRootPairs();
  void freeRoot();
  void freeTransactionRoot();
  void freeTransactionRoot(string moduleName);
  lllyd_node* getRoot(string moduleName);
  void setRoot(string moduleName, lllyd_node* newValue);
  lllyd_node* getTransactionRoot(string moduleName);
  bool nothingInTransaction();
  void setTransactionRoot(string moduleName, lllyd_node* newValue);
  virtual ~DatastoreState();
  DatastoreState(llly_ctx* _ctx, DatastoreType _type);
};

typedef struct DatastoreState DatastoreState;
} // namespace devmand::channels::cli::datastore
