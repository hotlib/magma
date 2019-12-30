// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <boost/algorithm/string.hpp>
#include <devmand/channels/cli/DatastoreTransaction.h>

namespace devmand::channels::cli {

void DatastoreTransaction::delete2(string path) {
  ly_set* pSet = lyd_find_path(root, const_cast<char*>(path.c_str()));
  MLOG(MDEBUG) << "Deleting " << pSet->number << " subtrees";
  for (unsigned int j = 0; j < pSet->number; ++j) {
    lyd_free(pSet->set.d[j]);
  }
  print();
}

void DatastoreTransaction::write(string path, shared_ptr<Entity> entity) {
  LeafVector leafs;
  createLeafs(entity, path, leafs);
  writeLeafs(leafs);
  print();
}

void DatastoreTransaction::create(shared_ptr<Entity> entity) {
  LeafVector leafs;
  createLeafs(entity, string(""), leafs);
  writeLeafs(leafs);
  print();
}

void DatastoreTransaction::commit() {}

void DatastoreTransaction::createLeafs(
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

void DatastoreTransaction::writeLeafs(LeafVector& leafs) {
  for (const auto& leaf : leafs) {
    char* data = const_cast<char*>(leaf.second.c_str());
    lyd_node* node = lyd_new_path(
        root,
        datastoreState.ctx,
        leaf.first.c_str(),
        data,
        LYD_ANYDATA_STRING,
        LYD_PATH_OPT_UPDATE);
    if (root == nullptr) {
      root = node; // any node in tree will do
    }
  }
}

void DatastoreTransaction::print() {
  char* buff;
  lyd_print_mem(&buff, root, LYD_XML, 0);
  MLOG(MINFO) << "current datatree (as XML):";
  MLOG(MINFO) << buff;
}

string DatastoreTransaction::toJson(lyd_node* initial) {
  char* buff;
  lyd_print_mem(&buff, initial, LYD_JSON, 0);
  return string(buff);
}

DatastoreTransaction::DatastoreTransaction(
    struct DatastoreState _datastoreState,
    const shared_ptr<ModelRegistry> _mreg)
    : datastoreState(_datastoreState), mreg(_mreg) {}

} // namespace devmand::channels::cli