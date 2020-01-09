// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/datastore/BindingAwareDatastoreTransaction.h>
#include <devmand/channels/cli/datastore/DatastoreState.h>
#include <devmand/channels/cli/datastore/DatastoreTransaction.h>
#include <devmand/devices/cli/ModelRegistry.h>

#include <libyang/libyang.h>

namespace devmand::channels::cli::datastore {
using devmand::channels::cli::datastore::BindingAwareDatastoreTransaction;
using devmand::channels::cli::datastore::DatastoreState;
using devmand::channels::cli::datastore::DatastoreTransaction;
using devmand::channels::cli::datastore::DatastoreType;
using devmand::devices::cli::ModelRegistry;
using std::shared_ptr;
using std::unique_ptr;

class Datastore {
 private:
  shared_ptr<ModelRegistry> mreg;
  shared_ptr<DatastoreState> datastoreState;
  void checkIfTransactionRunning();
  void setTransactionRunning();

 public:
  static DatastoreType operational();
  static DatastoreType config();

  Datastore(const shared_ptr<ModelRegistry> _mreg, DatastoreType _type);

  unique_ptr<DatastoreTransaction>
  newTx(); // operations on transaction are NOT thread-safe
  unique_ptr<BindingAwareDatastoreTransaction>
  newBindingTx(); // operations on transaction are NOT thread-safe
};
} // namespace devmand::channels::cli::datastore
