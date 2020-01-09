// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <libyang/libyang.h>
#include <atomic>

namespace devmand::channels::cli::datastore {
using std::atomic_bool;
struct DatastoreState {
  atomic_bool transactionUnderway = ATOMIC_VAR_INIT(false);
  ly_ctx* ctx = nullptr;
  lyd_node* root = nullptr;

  virtual ~DatastoreState() {
    if (root != nullptr) {
      lyd_free(root);
    }
    if (ctx != nullptr) {
      ly_ctx_destroy(ctx, nullptr); // TODO shared context for everyone or each
                                    // datastore has a private one???
    }
  }

 public:
  DatastoreState(ly_ctx* _ctx) : ctx(_ctx) {}

  bool isEmpty() {
    return root == nullptr || ctx == nullptr;
  }
};

typedef struct DatastoreState DatastoreState;
} // namespace devmand::channels::cli::datastore
