// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <boost/algorithm/string.hpp>
#include <devmand/channels/cli/DatastoreTransaction.h>
#include <libyang/tree_data.h>

namespace devmand::channels::cli {

void DatastoreTransaction::delete_(string path) {
  checkIfCommitted();
  ly_set* pSet = lyd_find_path(root, const_cast<char*>(path.c_str()));
  MLOG(MDEBUG) << "Deleting " << pSet->number << " subtrees";
  for (unsigned int j = 0; j < pSet->number; ++j) {
    lyd_free(pSet->set.d[j]);
  }
  print();
}

void DatastoreTransaction::write(string path, shared_ptr<Entity> entity) {
  checkIfCommitted();

  LeafVector leafs;
  createLeafs(entity, path, leafs);
  writeLeafs(leafs);
  print();
}

void DatastoreTransaction::create(shared_ptr<Entity> entity) {
  write("", entity);
}

void DatastoreTransaction::commit() {
  checkIfCommitted();

  // validateBeforeCommit();
  lyd_node* rootToBeMerged = computeRoot(
      root); // need the real root for convenience and copy via lyd_dup
  if (!datastoreState.isEmpty()) {
    lyd_free(datastoreState.root);
  }
  datastoreState.root = rootToBeMerged;

  hasCommited.store(true);
  print(datastoreState.root);
}

void DatastoreTransaction::validateBeforeCommit() {
  string error;
  if (root == nullptr) {
    error.assign(
        "datastore is empty and no changes performed, nothing to commit");
  }
  if (lyd_validate(&root, LYD_OPT_CONFIG, nullptr) !=
      0) { // TODO what validation options ??
    error.assign("model is invalid, won't commit changes to the datastore");
  }
  if (!error.empty()) {
    MLOG(MERROR) << error;
    throw std::runtime_error(error);
  }
}

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
      root =
          node; // any node in tree will do (doesn't have to be the actual root)
    }
  }
}

void DatastoreTransaction::print(lyd_node* nodeToPrint) {
  char* buff;
  lyd_print_mem(&buff, nodeToPrint, LYD_XML, 0);
  MLOG(MINFO) << buff;
  free(buff);
}

void DatastoreTransaction::print() {
  print(root);
}

string DatastoreTransaction::toJson(lyd_node* initial) {
  char* buff;
  lyd_print_mem(&buff, initial, LYD_JSON, 0);
  string result(buff);
  free(buff);
  return result;
}

DatastoreTransaction::DatastoreTransaction(
    struct DatastoreState& _datastoreState,
    const shared_ptr<ModelRegistry> _mreg)
    : datastoreState(_datastoreState), mreg(_mreg) {
  if (not datastoreState.isEmpty()) {
    root = lyd_dup(datastoreState.root, 1);
  }
}

lyd_node* DatastoreTransaction::computeRoot(lyd_node* n) {
  while (n->parent != nullptr) {
    n = n->parent;
  }
  return n;
}

void DatastoreTransaction::diff() {
  checkIfCommitted();
  if (datastoreState.isEmpty()) {
    throw std::runtime_error(
        "Unable to diff, datastore tree does not yet exist");
  }
  lyd_difflist* difflist =
      lyd_diff(datastoreState.root, root, LYD_DIFFOPT_WITHDEFAULTS);
  if (!difflist) {
    throw std::runtime_error("something went wrong, no diff possible");
  }

  for (int j = 0; difflist->type[j] != LYD_DIFF_END; ++j) {
    printDiffType(difflist->type[j]);
    print(difflist->first[j]);
  }

  lyd_free_diff(difflist);
}

DatastoreTransaction::~DatastoreTransaction() {
  if (not hasCommited && root != nullptr) {
    lyd_free(root);
  }
}

void DatastoreTransaction::checkIfCommitted() {
  if (hasCommited) {
    throw std::runtime_error(
        "transaction already committed, no operations available");
  }
}

void DatastoreTransaction::printDiffType(LYD_DIFFTYPE type) {
  switch (type) {
    case LYD_DIFF_DELETED:
      MLOG(MINFO) << "deleted subtree:";
      break;
    case LYD_DIFF_CHANGED:
      MLOG(MINFO) << "changed value:";
      break;
    case LYD_DIFF_MOVEDAFTER1:
      MLOG(MINFO) << "subtree was moved one way:";
      break;
    case LYD_DIFF_MOVEDAFTER2:
      MLOG(MINFO) << "subtree was moved another way:";
      break;
    case LYD_DIFF_CREATED:
      MLOG(MINFO) << "subtree was added:";
      break;
    case LYD_DIFF_END:
      throw std::runtime_error("LYD_DIFF_END can never be printed");
  }
}

} // namespace devmand::channels::cli