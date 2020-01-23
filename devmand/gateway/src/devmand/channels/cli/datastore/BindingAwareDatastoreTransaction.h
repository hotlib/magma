// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/channels/cli/codecs/YdkDynamicCodec.h>
#include <devmand/channels/cli/datastore/DatastoreState.h>
#include <devmand/channels/cli/datastore/DatastoreTransaction.h>
#include <devmand/devices/cli/schema/Path.h>

namespace devmand::channels::cli::datastore {
using LeafVector = std::vector<pair<string, string>>;
using devmand::channels::cli::codecs::YdkDynamicCodec;
using devmand::devices::cli::Path;

class BindingAwareDatastoreTransaction {
 private:
  DatastoreTransaction datastoreTransaction;
  shared_ptr<YdkDynamicCodec> codec;
  atomic_bool hasCommited = ATOMIC_VAR_INIT(false);

 public:
  BindingAwareDatastoreTransaction(
      shared_ptr<DatastoreState> datastoreState,
      shared_ptr<YdkDynamicCodec> codec);

 public:
  template <typename T>
  shared_ptr<T> read(Path path) {
    return codec->convert2<T>(datastoreTransaction.read(path));
  }
  void diff();
  void delete_(Path path);
  void write(Path path, shared_ptr<Entity> entity);
  void create(shared_ptr<Entity> entity);
  bool isValid();

  void commit();
};

} // namespace devmand::channels::cli::datastore
