// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/datastore/Datastore.h>

namespace devmand::channels::cli::datastore {
using std::make_unique;
using devmand::channels::cli::datastore::DatastoreException;

Datastore::Datastore(
    const shared_ptr<YdkDynamicCodec> _codec,
    DatastoreType type)
    : codec(_codec) {
  llly_ctx* pLyCtx = llly_ctx_new("/usr/share/openconfig@0.1.6/", 0);
  llly_ctx_load_module(pLyCtx, "iana-if-type", NULL);
  llly_ctx_load_module(pLyCtx, "openconfig-interfaces", NULL);
  datastoreState = make_shared<DatastoreState>(pLyCtx, type);
}

unique_ptr<DatastoreTransaction> Datastore::newTx() {
  checkIfTransactionRunning();
  setTransactionRunning();
  return make_unique<DatastoreTransaction>(datastoreState);
}

unique_ptr<BindingAwareDatastoreTransaction> Datastore::newBindingTx() {
  checkIfTransactionRunning();
  setTransactionRunning();
  return make_unique<BindingAwareDatastoreTransaction>(datastoreState, codec);
}

void Datastore::checkIfTransactionRunning() {
  if (datastoreState->transactionUnderway) {
      DatastoreException ex("Transaction in datastore already running, only 1 at a time permitted");
      MLOG(MWARNING) << ex.what();
      throw ex;
  }
}

void Datastore::setTransactionRunning() {
  datastoreState->transactionUnderway.store(true);
}

DatastoreType Datastore::operational() {
  return DatastoreType::operational;
}

DatastoreType Datastore::config() {
  return DatastoreType::config;
}
} // namespace devmand::channels::cli::datastore