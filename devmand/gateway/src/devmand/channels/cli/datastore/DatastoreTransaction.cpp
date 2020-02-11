// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <boost/algorithm/string.hpp>
#include <devmand/channels/cli/datastore/Datastore.h>
#include <devmand/channels/cli/datastore/DatastoreTransaction.h>
#include <libyang/tree_data.h>
#include <libyang/tree_schema.h>

namespace devmand::channels::cli::datastore {

using devmand::channels::cli::datastore::DatastoreException;
using folly::make_optional;
using folly::none;
using std::map;

bool DatastoreTransaction::delete_(Path p) {
  checkIfCommitted();
  string path = p.str();
  if (path.empty() || root == nullptr) {
    return false;
  }
  llly_set* pSet = lllyd_find_path(root, const_cast<char*>(path.c_str()));
  if (pSet == nullptr) {
    MLOG(MDEBUG) << "Nothing to delete, " + path + " not found";
    return false;
  } else {
    MLOG(MDEBUG) << "Deleting " << pSet->number << " subtrees";
  }
  for (unsigned int j = 0; j < pSet->number; ++j) {
    lllyd_free(pSet->set.d[j]);
  }
  llly_set_free(pSet);
  return true;
}

void DatastoreTransaction::overwrite(Path path, const dynamic& aDynamic) {
  delete_(path);
  merge(path, aDynamic);
  // print(root);
}

lllyd_node* DatastoreTransaction::dynamic2lydNode(dynamic entity) {
  lllyd_node* result = lllyd_parse_mem(
      datastoreState->ctx,
      const_cast<char*>(folly::toJson(entity).c_str()),
      LLLYD_JSON,
      datastoreTypeToLydOption() | LLLYD_OPT_TRUSTED);
  if (result == nullptr) {
    throw DatastoreException("Unable to create subtree from provided data");
  }

  return result;
}

dynamic DatastoreTransaction::appendAllParents(
    Path p,
    const dynamic& aDynamic) {
  dynamic previous = aDynamic;

  const std::vector<string>& segments = p.unkeyed().getSegments();

  for (long j = static_cast<long>(segments.size()) - 2; j >= 0; --j) {
    string segment = segments[static_cast<unsigned long>(j)];
    if (p.getKeysFromSegment(segment).empty()) {
      dynamic obj = dynamic::object;
      obj[segment] = previous;
      previous = obj;
    } else {
      dynamic obj = dynamic::object;
      const Path::Keys& k = p.getKeysFromSegment(segment);
      for (auto& pair : k.items()) { // adding mandatory keys
        previous[pair.first] = pair.second;
      }
      obj[segment] = dynamic::array(previous);
      previous = obj;
    }
  }

  return previous;
}

void DatastoreTransaction::merge(const Path path, const dynamic& aDynamic) {
  checkIfCommitted();
  if (path.str() != path.PATH_SEPARATOR) {
    dynamic withParents = appendAllParents(path, aDynamic);
    lllyd_node* pNode = dynamic2lydNode(withParents);

    if (root != nullptr) { // there exists something to merge to
      lllyd_merge(root, pNode, LLLYD_OPT_DESTRUCT);
    } else {
      root = pNode;
    }
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
  datastoreState->transactionUnderway.store(false);
}

void DatastoreTransaction::validateBeforeCommit() {
  if (!isValid()) {
    DatastoreException ex(
        "Model is invalid, won't commit changes to the datastore");
    MLOG(MERROR) << ex.what();
    throw ex;
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

map<Path, DatastoreDiff> DatastoreTransaction::diff() {
  checkIfCommitted();
  if (datastoreState->isEmpty()) {
    DatastoreException ex("Unable to diff, datastore tree does not yet exist");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }
  lllyd_difflist* difflist =
      lllyd_diff(datastoreState->root, root, LLLYD_DIFFOPT_WITHDEFAULTS);
  if (!difflist) {
    DatastoreException ex("Something went wrong, no diff possible");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }

  map<Path, DatastoreDiff> diffs;
  for (int j = 0; difflist->type[j] != LLLYD_DIFF_END; ++j) {
    if (difflist->type[j] == LLLYD_DIFF_MOVEDAFTER1 ||
        difflist->type[j] == LLLYD_DIFF_MOVEDAFTER2) {
      continue; // skip node movement changes
    }
    DatastoreDiffType type = getDiffType(difflist->type[j]);
    auto before = parseJson(toJson(difflist->first[j]));
    auto after = parseJson(toJson(difflist->second[j]));

    if (before == after) {
      continue;
    }

    Path path = Path(buildFullPath(
        getExistingNode(difflist->first[j], difflist->second[j], type), ""));

    if (diffs.count(path)) {
      continue;
    }

    auto pair = diffs.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(path),
        std::forward_as_tuple(before, after, type, path));

    if (not pair.second) {
      DatastoreException ex("Something went wrong during diff, can't diff");
      MLOG(MWARNING) << ex.what();
      throw ex;
    }
  }

  lllyd_free_diff(difflist);
  return diffs;
}

Optional<Path> DatastoreTransaction::getRegisteredPath(
    vector<DiffPath> registeredPaths,
    Path path) {
  const Optional<DiffPath>& registeredParentOptional =
      pickClosestPath(path, registeredPaths);
  if (registeredParentOptional) {
    DiffPath registeredParent = registeredParentOptional.value();
    if (registeredParent.asterix ||
        path.getSegments().size() ==
            (registeredParent.path.getSegments().size() + 1) ||
        path.getSegments().size() ==
            registeredParent.path.getSegments().size()) {
      return make_optional(registeredParent.path);
    } else {
      MLOG(MINFO) << "seg diff: " << path.getSegments().size()
                  << " seg reg: " << registeredParent.path.getSegments().size()
                  << " Unhandled event for " << registeredParent.path.str()
                  << " changed path: " << path.str();
    }
  } else {
    MLOG(MINFO) << "Totally Unhandled event for changed path: " << path.str();
  }

  return none;
}

Optional<DiffPath> DatastoreTransaction::pickClosestPath(
    Path path,
    vector<DiffPath> paths) {
  unsigned int max = 0;
  DiffPath result;
  bool found = false;
  for (const auto& p : paths) {
    if (path.segmentDistance(p.path) > max && path.isChildOf(p.path)) {
      result = p;
      max = path.segmentDistance(p.path);
      found = true;
    }
  }

  return found ? make_optional(result) : none;
}

DatastoreTransaction::~DatastoreTransaction() {
  if (not hasCommited && root != nullptr) {
    lllyd_free(root);
  }
  datastoreState->transactionUnderway.store(false);
}

void DatastoreTransaction::checkIfCommitted() {
  if (hasCommited) {
    DatastoreException ex(
        "Transaction already committed or aborted, no operations available");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }
}

DatastoreDiffType DatastoreTransaction::getDiffType(LLLYD_DIFFTYPE type) {
  switch (type) {
    case LLLYD_DIFF_DELETED:
      return DatastoreDiffType::deleted;
    case LLLYD_DIFF_CHANGED:
      return DatastoreDiffType::update;
    case LLLYD_DIFF_CREATED:
      return DatastoreDiffType::create;
    case LLLYD_DIFF_END:
      throw DatastoreException("This diff type is not supported");
    case LLLYD_DIFF_MOVEDAFTER1:
      throw DatastoreException("This diff type is not supported");
    case LLLYD_DIFF_MOVEDAFTER2:
      throw DatastoreException("This diff type is not supported");
    default:
      throw DatastoreException("This diff type is not supported");
  }
}

void DatastoreTransaction::addKeysToPath(
    lllyd_node* node,
    std::stringstream& path) {
  vector<string> keys;
  auto* list = (lllys_node_list*)node->schema;
  for (uint8_t i = 0; i < list->keys_size; i++) {
    keys.emplace_back(string(list->keys[i]->name));
  }
  for (const auto& key : keys) {
    lllyd_node* child = node->child;
    string childName(child->schema->name);
    while (childName != key) {
      child = node->next;
      childName.assign(child->schema->name);
    }
    lllyd_node_leaf_list* leafChild = (lllyd_node_leaf_list*)child;
    string keyValue(leafChild->value_str);
    path << "[" << key << "='" << keyValue << "']";
  }
}

string DatastoreTransaction::buildFullPath(lllyd_node* node, string pathSoFar) {
  std::stringstream path;
  path << "/" << node->schema->module->name << ":" << node->schema->name;
  if (node->schema->nodetype == LLLYS_LIST) {
    addKeysToPath(node, path);
  }
  path << pathSoFar;
  if (node->parent == nullptr) {
    return path.str();
  }
  return buildFullPath(node->parent, path.str());
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
      MLOG(MINFO) << "end of diff:";
  }
}

dynamic DatastoreTransaction::read(Path path) {
  checkIfCommitted();
  return read(path, root);
}

dynamic DatastoreTransaction::readAlreadyCommitted(Path path) {
  if (datastoreState->root == nullptr) {
    return dynamic::object();
  }

  return read(path, datastoreState->root);
}

dynamic DatastoreTransaction::read(Path path, lllyd_node* node) {
  llly_set* pSet = lllyd_find_path(node, const_cast<char*>(path.str().c_str()));

  if (pSet == nullptr) {
    return nullptr;
  }

  if (pSet->number == 0) {
    llly_set_free(pSet);
    return nullptr;
  }

  if (pSet->number > 1) {
    llly_set_free(pSet);

    DatastoreException ex(
        "Too many results from path: " + path.str() +
        " , query must target a unique element");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }

  const string& json = toJson(pSet->set.d[0]);
  llly_set_free(pSet);
  return parseJson(json);
}

void DatastoreTransaction::print(LeafVector& v) {
  for (const auto& item : v) {
    MLOG(MINFO) << "full path: " << item.first << " data: " << item.second;
  }
}

lllyd_node* DatastoreTransaction::getExistingNode(
    lllyd_node* a,
    lllyd_node* b,
    DatastoreDiffType type) {
  if (type == DatastoreDiffType::create && b != nullptr) {
    return b;
  }
  if (type == DatastoreDiffType::deleted && a != nullptr) {
    return a;
  }

  return a == nullptr ? b : a;
}

bool DatastoreTransaction::isValid() {
  checkIfCommitted();
  if (root == nullptr) {
    DatastoreException ex(
        "Datastore is empty and no changes performed, nothing to validate");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }
  return lllyd_validate(&root, datastoreTypeToLydOption(), nullptr) == 0;
}

int DatastoreTransaction::datastoreTypeToLydOption() {
  switch (datastoreState->type) {
    case operational:
      return LLLYD_OPT_GET; // operational validation, turns off validation for
      // things like mandatory nodes, leaf-refs etc.
      // because devices do not have to support all
      // mandatory nodes (like BGP) and thus would only
      // cause false validation errors
    case config:
      return LLLYD_OPT_GETCONFIG; // config validation with turned off checks
      // because of reasons mentioned above
  }
  return 0;
}

void DatastoreTransaction::splitToMany(
    Path p,
    dynamic input,
    vector<std::pair<string, dynamic>>& result) {
  if (input.isArray()) {
    // TODO fix this
    return;
  }

  for (const auto& item : input.items()) {
    if (item.second.isArray() || item.second.isObject()) {
      string currentPath = p.str();
      if (p.getLastSegment() !=
          item.first.asString()) { // TODO skip last overlapping segment name
        currentPath = p.str() + "/" + item.first.c_str();
      }
      result.emplace_back(std::make_pair(currentPath, input));
      splitToMany(Path(currentPath), item.second, result);
    }
  }
}

Path DatastoreTransaction::unifyLength(Path registeredPath, Path keyedPath){
    while(keyedPath.getDepth() != registeredPath.getDepth()){
        keyedPath = keyedPath.getParent();
    }
    return keyedPath;
}

multimap<Path, DatastoreDiff> DatastoreTransaction::diff(
    vector<DiffPath> registeredPaths) {
  multimap<Path, DatastoreDiff> result;
  const map<Path, DatastoreDiff>& diffs = diff();
  for (const auto& diffItem : diffs) { // take libyang diffs
    const map<Path, DatastoreDiff>& smallerDiffs =
        splitDiff(diffItem.second); // split them to smaller ones
    for (const auto& smallerDiffsItem : smallerDiffs) { // merge them
      Optional<Path> registeredPath =
          getRegisteredPath(registeredPaths, smallerDiffsItem.first);
      if (registeredPath) {
          //this is the registered path provided by the handlers
        Path registeredPathHandlingDiff = registeredPath.value();
          //we need a keyed path to read before and after state for the handlers
        Path pathForReadingBeforeAfter = unifyLength(registeredPathHandlingDiff, smallerDiffsItem.second.keyedPath);
        result.emplace(
            std::make_pair(registeredPathHandlingDiff,
                    DatastoreDiff(
                            //we read what the state was before (no just the change but the whole
                            // subtree under the registered path)
                            readAlreadyCommitted(pathForReadingBeforeAfter),
                            //we read what is there now (no just the change but the whole
                            //subtree under the registered path)
                            read(pathForReadingBeforeAfter),
                                  smallerDiffsItem.second.type,
                                  smallerDiffsItem.second.keyedPath
                            )));
      } else {
        // TODO no registered paths for occured event
      }
    }
  }

  return result;
}

map<Path, DatastoreDiff> DatastoreTransaction::splitDiff(DatastoreDiff diff) {
  map<Path, DatastoreDiff> diffs;
  vector<std::pair<string, dynamic>> split;
  if (diff.type == DatastoreDiffType::create) {
    splitToMany(diff.keyedPath, diff.after, split);
    for (const auto& s : split) {
      diffs.emplace(
          s.first,
          DatastoreDiff(diff.before, s.second, diff.type, Path(s.first)));
    }
    return diffs;
  } else if (diff.type == DatastoreDiffType::deleted) {
    splitToMany(diff.keyedPath, diff.before, split);
    for (const auto& s : split) {
      diffs.emplace(
          s.first,
          DatastoreDiff(s.second, diff.after, diff.type, Path(s.first)));
    }
    return diffs;
  }

  diffs.emplace(diff.path, diff);
  return diffs;
}

} // namespace devmand::channels::cli::datastore
