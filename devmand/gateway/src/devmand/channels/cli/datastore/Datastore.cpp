// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/datastore/Datastore.h>

namespace devmand::channels::cli::datastore {
using std::make_unique;

Datastore::Datastore(const shared_ptr<ModelRegistry> _mreg) : mreg(_mreg) {
  ly_ctx* pLyCtx = ly_ctx_new("/usr/share/openconfig@0.1.6/", 0);
  ly_ctx_load_module(pLyCtx, "iana-if-type", NULL);
  ly_ctx_load_module(pLyCtx, "openconfig-interfaces", NULL);
  datastoreState = make_shared<DatastoreState>(pLyCtx);
}

unique_ptr<DatastoreTransaction> Datastore::newTx() {
  return make_unique<DatastoreTransaction>(datastoreState);
}

unique_ptr<BindingAwareDatastoreTransaction> Datastore::newBindingTx() {
  return make_unique<BindingAwareDatastoreTransaction>(datastoreState, mreg);
}
} // namespace devmand::channels::cli::datastore