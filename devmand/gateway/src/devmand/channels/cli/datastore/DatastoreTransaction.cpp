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
  if (path.empty()) {
    return;
  }
  llly_set* pSet = lllyd_find_path(root, const_cast<char*>(path.c_str()));
  MLOG(MDEBUG) << "Deleting " << pSet->number << " subtrees";
  for (unsigned int j = 0; j < pSet->number; ++j) {
    lllyd_free(pSet->set.d[j]);
  }
}

void DatastoreTransaction::write(const string path, const dynamic& aDynamic) {
  delete_(path);
  merge(path, aDynamic);
  // print(root);
}

lllyd_node* DatastoreTransaction::dynamic2lydNode(dynamic entity) {
  return lllyd_parse_mem(
      datastoreState->ctx,
      const_cast<char*>(folly::toJson(entity).c_str()),
      LLLYD_JSON,
      datastoreTypeToLydOption());
}

dynamic DatastoreTransaction::appendAllParents(
    string path,
    const dynamic& aDynamic) {
  dynamic previous = aDynamic;

  std::vector<string> strs;

  boost::split(strs, path, boost::is_any_of("/"));
  std::reverse(strs.begin(), strs.end());

  for (const auto& pathSegment : fixSegments(strs)) {
    dynamic obj = dynamic::object;
    obj[pathSegment] = previous;
    previous = obj;
  }
  return previous;
}

void DatastoreTransaction::merge(const string path, const dynamic& aDynamic) {
  if (!path.empty()) {
    dynamic withParents = appendAllParents(path, aDynamic);
    lllyd_node* pNode = dynamic2lydNode(withParents);
    lllyd_merge(root, pNode, LLLYD_OPT_DESTRUCT);
  } else {
    if (root != nullptr) {
      lllyd_free(root);
    }
    root = dynamic2lydNode(aDynamic);
  }
}

void DatastoreTransaction::commit() {
  checkIfCommitted();

  validateBeforeCommit();
  lllyd_node* rootToBeMerged = computeRoot(
      root); // need the real root for convenience and copy via lllyd_dup
  if (!datastoreState->isEmpty()) {
    lllyd_free(datastoreState->root);
  }
  datastoreState->root = rootToBeMerged;

  hasCommited.store(true);
  datastoreState->transactionUnderway.store(false);
  //  print(datastoreState->root);
}

void DatastoreTransaction::abort() {
  checkIfCommitted();
  if (root != nullptr) {
    lllyd_free(root);
  }

  hasCommited.store(true);
  datastoreState->transactionUnderway.store(
      false); // TODO what if transaction goes out of scope?
}

void DatastoreTransaction::validateBeforeCommit() {
  if (!isValid()) {
    string error = "model is invalid, won't commit changes to the datastore";
    MLOG(MERROR) << error;
    throw runtime_error(error);
  }
}

void DatastoreTransaction::print(lllyd_node* nodeToPrint) {
  char* buff;
  lllyd_print_mem(&buff, nodeToPrint, LLLYD_XML, 0);
  MLOG(MINFO) << buff;
  free(buff);
}

void DatastoreTransaction::print() {
  print(root);
}


    string DatastoreTransaction::toJson(lllyd_node* initial) {
        char* buff;
        lllyd_print_mem(&buff, initial, LLLYD_JSON, LLLYP_WD_ALL);
        string result(buff);
        free(buff);
        return result;
    }



//string DatastoreTransaction::toJson(lllyd_node* initial) {
//    char* buff2;
//    lllyd_print_mem(&buff2, initial, LLLYD_XML, LLLYP_WD_ALL | LLLYP_FORMAT);
//    MLOG(MINFO) << "XML data su: " << buff2;
//    free(buff2);
//    MLOG(MINFO) << "koniec XML";
//
//  char* buff;
//  lllyd_print_mem(&buff, initial, LLLYD_JSON, LLLYP_WD_ALL | LLLYP_FORMAT);
//  string result(buff);
//    MLOG(MINFO) << "JSON data su: " << buff;
//    MLOG(MINFO) << "koniec JSON";
//  free(buff);
//  return result;
//}

DatastoreTransaction::DatastoreTransaction(
    shared_ptr<DatastoreState> _datastoreState)
    : datastoreState(_datastoreState) {
  if (not datastoreState->isEmpty()) {
    root = lllyd_dup(datastoreState->root, 1);
  }
}

lllyd_node* DatastoreTransaction::computeRoot(lllyd_node* n) {
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
  lllyd_difflist* difflist =
      lllyd_diff(datastoreState->root, root, LLLYD_DIFFOPT_WITHDEFAULTS);
  if (!difflist) {
    throw runtime_error("something went wrong, no diff possible");
  }

  for (int j = 0; difflist->type[j] != LLLYD_DIFF_END; ++j) {
    printDiffType(difflist->type[j]);
    print(difflist->first[j]);
  }

  lllyd_free_diff(difflist);
}

DatastoreTransaction::~DatastoreTransaction() {
  if (not hasCommited && root != nullptr) {
    lllyd_free(root);
  }
  // datastoreState->transactionUnderway.store(false); //TODO what if
  // transaction goes out of scope?
}

void DatastoreTransaction::checkIfCommitted() {
  if (hasCommited) {
    throw runtime_error(
        "transaction already committed or aborted, no operations available");
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

void DatastoreTransaction::printDiffType(LLLYD_DIFFTYPE type) {
  switch (type) {
    case LLLYD_DIFF_DELETED:
      MLOG(MINFO) << "deleted subtree:";
      break;
    case LLLYD_DIFF_CHANGED:
      MLOG(MINFO) << "changed value:";
      break;
    case LLLYD_DIFF_MOVEDAFTER1:
      MLOG(MINFO) << "subtree was moved one way:";
      break;
    case LLLYD_DIFF_MOVEDAFTER2:
      MLOG(MINFO) << "subtree was moved another way:";
      break;
    case LLLYD_DIFF_CREATED:
      MLOG(MINFO) << "subtree was added:";
      break;
    case LLLYD_DIFF_END:
      throw runtime_error("lllyD_DIFF_END can never be printed");
  }
}

dynamic DatastoreTransaction::read(string path ) {
  checkIfCommitted();

  llly_set* pSet = lllyd_find_path(root, const_cast<char*>(path.c_str()));

    if (pSet == nullptr || pSet->number == 0) {
        return nullptr;
    }

  if (pSet->number > 1) {
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
  return lllyd_validate(&root, datastoreTypeToLydOption(), nullptr) == 0;
}

int DatastoreTransaction::datastoreTypeToLydOption() {
  switch (datastoreState->type) {
    case operational:
      return LLLYD_OPT_DATA | LLLYD_OPT_DATA_NO_YANGLIB;
    case config:
      return LLLYD_OPT_CONFIG;
  }

  return 0;
}

} // namespace devmand::channels::cli::datastore