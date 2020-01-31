// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/datastore/Datastore.h>
#include <devmand/devices/cli/schema/Model.h>

namespace devmand::channels::cli::datastore {
using devmand::channels::cli::datastore::DatastoreException;
using devmand::devices::cli::SchemaContext;
using std::make_unique;
Datastore::Datastore(DatastoreType type) {
  Model model = Model::OPENCONFIG_0_1_6;
  schemaContext = std::make_shared<SchemaContext>(model);
  ydk::path::Repository repo(
      model.getDir(), ydk::path::ModelCachingOption::COMMON);
  bindingCodec =
      std::make_shared<BindingCodec>(repo, model.getDir(), *schemaContext);

  llly_ctx* pLyCtx = llly_ctx_new(model.getDir().c_str(), 0);
  llly_ctx_load_module(pLyCtx, "iana-if-type", NULL);
  llly_ctx_load_module(pLyCtx, "openconfig-interfaces", NULL);
  // schemaCtx.loadModules(pLyCtx, model); //TODO not working
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
  return make_unique<BindingAwareDatastoreTransaction>(
      datastoreState, bindingCodec);
}

void Datastore::checkIfTransactionRunning() {
  if (datastoreState->transactionUnderway) {
    DatastoreException ex(
        "Transaction in datastore already running, only 1 at a time permitted");
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