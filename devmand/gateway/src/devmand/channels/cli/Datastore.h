// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/DatastoreState.h>
#include <devmand/devices/cli/ModelRegistry.h>
#include <devmand/channels/cli/DatastoreTransaction.h>

#include <libyang/libyang.h>

namespace devmand::channels::cli {
using devmand::channels::cli::DatastoreState;
using devmand::channels::cli::DatastoreTransaction;
using devmand::devices::cli::ModelRegistry;
using std::shared_ptr;
using std::unique_ptr;

class Datastore {
 private:
  DatastoreState datastoreState;

 public:
  Datastore(const shared_ptr<ModelRegistry> _mreg);

 private:
  shared_ptr<ModelRegistry> mreg;

   public:
    unique_ptr<DatastoreTransaction>
    newTx(); // operations on transaction are NOT thread-safe
};
} // namespace devmand::channels::cli
