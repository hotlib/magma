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

namespace devmand::channels::cli::datastore {
using std::atomic_bool;
using std::map;
using std::pair;
using std::string;
using std::make_pair;
typedef std::map<string, lllyd_node*> LydMap;
typedef pair<string, lllyd_node*> LydPair;

enum DatastoreType { config, operational };

struct DatastoreState {
  atomic_bool transactionUnderway = ATOMIC_VAR_INIT(false);
  llly_ctx* ctx = nullptr;
  DatastoreType type;
  LydMap forest;
  LydMap forestInTransaction;


    lllyd_node* getData(string moduleName, LydMap & source){
        if (!source.count(moduleName)){
            lllyd_node* newRoot = nullptr;
            source.insert(LydPair(moduleName, newRoot));
        }

        return source[moduleName];
    }

    void setData(string moduleName, lllyd_node* newValue, LydMap & source){
        if (!source.count(moduleName)){
            lllyd_node* newRoot = nullptr;
            source.insert(LydPair(moduleName, newRoot));
        }

        source[moduleName] = newValue;
    }

    void freeData(LydMap & source){
        for (const auto &item : source) {
            if(item.second != nullptr){
                lllyd_free(item.second);
            }
            source[item.first] = nullptr;
        }
    }

    lllyd_node* computeRoot(lllyd_node* n) {
        while (n->parent != nullptr) {
            n = n->parent;
        }
        return n;
    }


    void duplicateForTransaction(){
        for (const auto &item : forest) {
            if(item.second == nullptr){
                continue;
            }
            setTransactionRoot(item.first, lllyd_dup(item.second, 1));
        }
    }

    void setRootFromTransaction(){

        for (const auto &transactionItem : forestInTransaction) {
            if(transactionItem.second == nullptr){
                continue;
            }
            setRoot(transactionItem.first, computeRoot(transactionItem.second));
        }
    }


    void freeRoot(){
        freeData(forest);
    }

    void freeTransactionRoot(){
        freeData(forestInTransaction);
    }
    lllyd_node* getRoot(string moduleName){
        return getData(moduleName, forest);
    }

    void setRoot(string moduleName, lllyd_node* newValue) {
        setData(moduleName, newValue, forest);
    }

        lllyd_node* getTransactionRoot(string moduleName){
        return getData(moduleName, forestInTransaction);
    }

    void setTransactionRoot(string moduleName, lllyd_node* newValue){
        setData(moduleName, newValue, forestInTransaction);
    }


  virtual ~DatastoreState() {
    if (getRoot("root") != nullptr) { //TODO freeData pre vsetko
      lllyd_free(getRoot("root"));
    }
  }

 public:
  DatastoreState(llly_ctx* _ctx, DatastoreType _type)
      : ctx(_ctx), type(_type) {}

  bool isEmpty() {
    return getRoot("root") == nullptr || ctx == nullptr;
  }
};

typedef struct DatastoreState DatastoreState;
} // namespace devmand::channels::cli::datastore
