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

void BindingAwareDatastoreTransaction::delete_(string path) {
  datastoreTransaction.delete_(path);
}

void BindingAwareDatastoreTransaction::write(
    string path,
    shared_ptr<Entity> entity) {
  LeafVector leafs;
  createLeafs(entity, path, leafs);
  datastoreTransaction.write(leafs);
}

void BindingAwareDatastoreTransaction::create(shared_ptr<Entity> entity) {
  datastoreTransaction.write("", codec->convert(entity));
  // write("", entity);
}

void BindingAwareDatastoreTransaction::createLeafs(
    shared_ptr<Entity> entity,
    string init,
    LeafVector& leafs) {
  string prefix = init + "/" + entity->get_segment_path();
  if (entity->get_name_leaf_data().size() > 0) { // process only leafs with data

    for (const auto& data : entity->get_name_leaf_data()) {
      std::vector<string> strs;
      boost::split(strs, data.first, boost::is_any_of(" "));
      string leafPath = prefix + "/" + strs[0];
      string leafData = data.second.value;
      leafs.emplace_back(std::make_pair(leafPath, leafData));
    }
  }

  for (const auto& child : entity->get_children()) {
    createLeafs(child.second, prefix, leafs);
  }
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