// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/datastore/DatastoreState.h>

namespace devmand::channels::cli::datastore {
lllyd_node* DatastoreState::getData(string moduleName, LydMap& source) {
  if (!source.count(moduleName)) {
    lllyd_node* newRoot = nullptr;
    source.insert(LydPair(moduleName, newRoot));
  }

  return source[moduleName];
}

void DatastoreState::setData(
    string moduleName,
    lllyd_node* newValue,
    LydMap& source) {
  if (!source.count(moduleName)) {
    lllyd_node* newRoot = nullptr;
    source.insert(LydPair(moduleName, newRoot));
  }

  source[moduleName] = newValue;
}

void DatastoreState::freeData(LydMap& source) {
  for (const auto& item : source) {
    if (item.second != nullptr) {
      lllyd_free(item.second);
    }
    source[item.first] = nullptr;
  }
}

lllyd_node* DatastoreState::computeRoot(lllyd_node* n) {
  while (n->parent != nullptr) {
    n = n->parent;
  }
  return n;
}

void DatastoreState::duplicateForTransaction() {
  for (const auto& item : forest) {
    if (item.second == nullptr) {
      continue;
    }
    setTransactionRoot(item.first, lllyd_dup(item.second, 1));
  }
}

void DatastoreState::setRootFromTransaction() {
  for (const auto& transactionItem : forestInTransaction) {
    if (transactionItem.second == nullptr) {
      continue;
    }
    setRoot(transactionItem.first, computeRoot(transactionItem.second));
    setTransactionRoot(transactionItem.first, nullptr);
  }
}

vector<RootPair> DatastoreState::getRootAndTransactionRootPairs() {
  vector<RootPair> result;
  for (const auto& transactionItem : forestInTransaction) {
    result.emplace_back(
        getRoot(transactionItem.first),
        forestInTransaction[transactionItem.first]);
  }
  return result;
}

void DatastoreState::freeRoot() {
  freeData(forest);
}

void DatastoreState::freeTransactionRoot() {
  freeData(forestInTransaction);
}

void DatastoreState::freeTransactionRoot(string moduleName) {
  if (forestInTransaction[moduleName] != nullptr) {
    lllyd_free(forestInTransaction[moduleName]);
  }
  forestInTransaction[moduleName] = nullptr;
}

lllyd_node* DatastoreState::getRoot(string moduleName) {
  return getData(moduleName, forest);
}

void DatastoreState::setRoot(string moduleName, lllyd_node* newValue) {
  setData(moduleName, newValue, forest);
}

lllyd_node* DatastoreState::getTransactionRoot(string moduleName) {
  return getData(moduleName, forestInTransaction);
}

bool DatastoreState::nothingInTransaction() {
  for (const auto& item : forestInTransaction) {
    if (forestInTransaction[item.first] != nullptr) {
      return false;
    }
  }
  return true;
}

void DatastoreState::setTransactionRoot(
    string moduleName,
    lllyd_node* newValue) {
  setData(moduleName, newValue, forestInTransaction);
}

DatastoreState::~DatastoreState() {
  freeRoot();
  freeTransactionRoot();
}

DatastoreState::DatastoreState(llly_ctx* _ctx, DatastoreType _type)
    : ctx(_ctx), type(_type) {}
} // namespace devmand::channels::cli::datastore