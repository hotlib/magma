// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/Datastore.h>

namespace devmand::channels::cli {
using std::make_unique;

Datastore::Datastore(const shared_ptr<ModelRegistry> _mreg) : mreg(_mreg) {
  datastoreState.ctx = ly_ctx_new("/usr/share/openconfig@0.1.6/", 0);
  ly_ctx_load_module(datastoreState.ctx, "iana-if-type", NULL);
  ly_ctx_load_module(datastoreState.ctx, "openconfig-interfaces", NULL);
}

unique_ptr<DatastoreTransaction> Datastore::newTx() {
  return make_unique<DatastoreTransaction>(datastoreState, mreg);
}
} // namespace devmand::channels::cli