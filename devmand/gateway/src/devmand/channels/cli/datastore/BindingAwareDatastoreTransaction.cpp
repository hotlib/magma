// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <boost/algorithm/string.hpp>
#include <devmand/channels/cli/datastore/BindingAwareDatastoreTransaction.h>

namespace devmand::channels::cli::datastore {

void BindingAwareDatastoreTransaction::diff() {
  datastoreTransaction.diff();
}

void BindingAwareDatastoreTransaction::delete_(Path path) {
  datastoreTransaction.delete_(path);
}

void BindingAwareDatastoreTransaction::write(
    Path path,
    shared_ptr<Entity> entity) {
    datastoreTransaction.overwrite(path, codec->convert(entity));
}

void BindingAwareDatastoreTransaction::create(shared_ptr<Entity> entity) {
  write("", entity);
}

void BindingAwareDatastoreTransaction::commit() {
  datastoreTransaction.commit();
}

BindingAwareDatastoreTransaction::BindingAwareDatastoreTransaction(
    shared_ptr<DatastoreState> _datastoreState,
    shared_ptr<YdkDynamicCodec> _codec)
    : datastoreTransaction(_datastoreState), codec(_codec) {}

bool BindingAwareDatastoreTransaction::isValid() {
  return datastoreTransaction.isValid();
}

} // namespace devmand::channels::cli::datastore