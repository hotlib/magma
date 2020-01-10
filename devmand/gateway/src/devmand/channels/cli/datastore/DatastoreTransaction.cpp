// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <boost/algorithm/string.hpp>
#include <devmand/channels/cli/datastore/DatastoreTransaction.h>
#include <libyang/tree_data.h>
#include <libyang/tree_schema.h>

namespace devmand::channels::cli::datastore {

using std::runtime_error;

void DatastoreTransaction::delete_(string path) {
  checkIfCommitted();
  ly_set* pSet = lyd_find_path(root, const_cast<char*>(path.c_str()));
  MLOG(MDEBUG) << "Deleting " << pSet->number << " subtrees";
  for (unsigned int j = 0; j < pSet->number; ++j) {
    lyd_free(pSet->set.d[j]);
  }
}

void DatastoreTransaction::write(LeafVector& leafs) {
  // print(leafs);
  checkIfCommitted();
  writeLeafs(leafs);
}

void DatastoreTransaction::write(const string path, const dynamic& aDynamic) {
  std::vector<string> strs;

  boost::split(strs, path, boost::is_any_of("/")); // TODO path empty??
  std::reverse(strs.begin(), strs.end());

  dynamic previous = aDynamic;
  for (const auto& pathSegment : fixSegments(strs)) {
    dynamic obj = dynamic::object;
    obj[pathSegment] = previous;
    previous = obj;
  }

  lyd_node* pNode = lyd_parse_mem(
      datastoreState->ctx,
      const_cast<char*>(folly::toJson(previous).c_str()),
      LYD_JSON,
      datastoreTypeToLydOption());
  delete_(path);
  lyd_merge(root, pNode, LYD_OPT_DESTRUCT);
}

void DatastoreTransaction::commit() {
  checkIfCommitted();

  // validateBeforeCommit(); //TODO not working
  lyd_node* rootToBeMerged = computeRoot(
      root); // need the real root for convenience and copy via lyd_dup
  if (!datastoreState->isEmpty()) {
    lyd_free(datastoreState->root);
  }
  datastoreState->root = rootToBeMerged;

  hasCommited.store(true);
  datastoreState->transactionUnderway.store(false);
  print(datastoreState->root);
}

void DatastoreTransaction::validateBeforeCommit() {
  if (!isValid()) {
    string error = "model is invalid, won't commit changes to the datastore";
    MLOG(MERROR) << error;
    throw runtime_error(error);
  }
}

void DatastoreTransaction::writeLeafs(LeafVector& leafs) {
  for (const auto& leaf : leafs) {
    char* data = const_cast<char*>(leaf.second.c_str());
    lyd_node* node = lyd_new_path(
        root,
        datastoreState->ctx,
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
    shared_ptr<DatastoreState> _datastoreState)
    : datastoreState(_datastoreState) {
  if (not datastoreState->isEmpty()) {
    root = lyd_dup(datastoreState->root, 1);
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
  if (datastoreState->isEmpty()) {
    throw runtime_error("Unable to diff, datastore tree does not yet exist");
  }
  lyd_difflist* difflist =
      lyd_diff(datastoreState->root, root, LYD_DIFFOPT_WITHDEFAULTS);
  if (!difflist) {
    throw runtime_error("something went wrong, no diff possible");
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
    throw runtime_error(
        "transaction already committed, no operations available");
  }
}
// TODO temporary hack
std::vector<string> DatastoreTransaction::fixSegments(std::vector<string> str) {
  std::vector<string> final;
  string broken;
  broken.assign("");

  for (const auto& s : str) {
    if (s.empty())
      continue;
    unsigned long found = s.find("'");
    if (found != std::string::npos) {
      if (broken.empty()) {
        broken.assign(s);
      } else {
        string ss = s + "/" + broken;
        final.push_back(ss);
        broken.assign("");
      }
      continue;
    }
    final.push_back(s);
  }
  final.erase(final.begin());

  return final;
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
      throw runtime_error("LYD_DIFF_END can never be printed");
  }
}

dynamic DatastoreTransaction::read(string path) {
  checkIfCommitted();

  ly_set* pSet = lyd_find_path(root, const_cast<char*>(path.c_str()));
  if (pSet->number != 1) {
    throw runtime_error("Too many results from path: " + path);
  }

  const string& json = toJson(pSet->set.d[0]);
  return parseJson(json);
}

void DatastoreTransaction::print(LeafVector& v) {
  for (const auto& item : v) {
    MLOG(MINFO) << "full path: " << item.first << " data: " << item.second;
  }
}

bool DatastoreTransaction::isValid() {
  if (root == nullptr) {
    throw runtime_error(
        "datastore is empty and no changes performed, nothing to validate");
  }
  // TODO what validation options (LYD_OPT_CONFIG..)??
  return lyd_validate(&root, datastoreTypeToLydOption(), nullptr) == 0;
}

int DatastoreTransaction::datastoreTypeToLydOption() {
  switch (datastoreState->type) {
    case operational:
      return LYD_OPT_DATA;
    case config:
      return LYD_OPT_CONFIG;
  }

  return 0;
}

} // namespace devmand::channels::cli::datastore