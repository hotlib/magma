// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <libyang/libyang.h>

namespace devmand::channels::cli {

struct DatastoreState {
  ly_ctx* ctx = nullptr;
  lyd_node* root = nullptr;

 public:
  bool isEmpty() {
    return root == nullptr;
  }
};

typedef struct DatastoreState DatastoreState;
} // namespace devmand::channels::cli
