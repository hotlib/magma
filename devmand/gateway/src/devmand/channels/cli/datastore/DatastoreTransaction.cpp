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
#include <set>

namespace devmand::channels::cli::datastore {

using devmand::channels::cli::datastore::DatastoreException;
using folly::make_optional;
using folly::none;
using std::map;

bool DatastoreTransaction::delete_(Path p) {
  checkIfCommitted();
  string path = p.str();
  if (path.empty()) {
    MLOG(MINFO) << "Nothing to delete for: " << p;
    return false;
  }

  if (Path::ROOT == p && !p.getFirstModuleName().hasValue()) {
    datastoreState->freeTransactionRoot(); // delete all trees
    return true;
  }

  if (not p.getFirstModuleName().hasValue()) {
    MLOG(MINFO) << "Unable to delete path without module name in the path: "
                << p;
    return false;
  }
  string moduleName = p.getFirstModuleName().value();

  if (p.getDepth() == 1) {
    datastoreState->freeTransactionRoot(
        moduleName); // delete a whole specific tree
    return true;
  }

  llly_set* pSet = lllyd_find_path(
      datastoreState->getTransactionRoot(moduleName),
      const_cast<char*>(path.c_str()));

  if (pSet == nullptr) {
    MLOG(MINFO) << "Nothing to delete, " + path +
            " not found for module: " + moduleName;
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
}

lllyd_node* DatastoreTransaction::dynamic2lydNode(dynamic entity) {
  lllyd_node* result = lllyd_parse_mem(
      datastoreState->ctx,
      const_cast<char*>(folly::toJson(entity).c_str()),
      LLLYD_JSON,
      datastoreTypeToLydOption() | LLLYD_OPT_TRUSTED);
  if (result == nullptr) {
    string lyErrMessage(
        llly_errmsg(datastoreState->ctx) == nullptr
            ? ""
            : llly_errmsg(datastoreState->ctx));
    throw DatastoreException(
        "Unable to create subtree from provided data " + lyErrMessage);
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

// returns pairs in form <moduleName, rootNodeOfSingleTree>
vector<LydPair> DatastoreTransaction::splitNodeToRoots(lllyd_node* node) {
  vector<LydPair> result;
  lllyd_node* root = computeRoot(node);
  lllyd_node* next = nullptr; // TODO break 'previous' link as well??
  do {
    next = root->next;
    root->next = nullptr;
    result.emplace_back(string(root->schema->module->name), root);
    root = next;
  } while (root != nullptr);

  return result;
}
void DatastoreTransaction::merge(const Path path, const dynamic& aDynamic) {
  checkIfCommitted();
  dynamic withParents = appendAllParents(path, aDynamic);
  lllyd_node* pNode = dynamic2lydNode(withParents);
  vector<LydPair> pairsToMerge = splitNodeToRoots(pNode);
  for (const auto& toMerge : pairsToMerge) {
    if (datastoreState->getTransactionRoot(toMerge.first) ==
        nullptr) { // if nothing exists yet
      datastoreState->setTransactionRoot(
          toMerge.first, toMerge.second); // set toMerge as new root
    } else {
      // otherwise perform a regular libyang merge
      lllyd_node* tmp = datastoreState->getTransactionRoot(toMerge.first);
      lllyd_merge(tmp, toMerge.second, LLLYD_OPT_DESTRUCT);
      datastoreState->setTransactionRoot(toMerge.first, tmp);
    }
  }
}

void DatastoreTransaction::commit() {
  checkIfCommitted();

  validateBeforeCommit();
  datastoreState->freeRoot(); // free existing trees
  datastoreState->setRootFromTransaction(); // set all new trees
  hasCommited.store(true);
  datastoreState->transactionUnderway.store(false);
}

void DatastoreTransaction::abort() {
  checkIfCommitted();
  datastoreState->freeTransactionRoot(); // free all trees in transaction
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
  for (const auto& rootPair :
       datastoreState->getRootAndTransactionRootPairs()) {
    print(rootPair.second);
  }
}

string DatastoreTransaction::toJson(lllyd_node* initial) {
  char* buff;
  lllyd_print_mem(&buff, initial, LLLYD_JSON, LLLYP_WD_ALL);
  string result(buff);
  free(buff);
  return result;
}

DatastoreTransaction::DatastoreTransaction(
    shared_ptr<DatastoreState> _datastoreState,
    SchemaContext& _schemaContext)
    : datastoreState(_datastoreState), schemaContext(_schemaContext) {
  datastoreState->duplicateForTransaction();
}

lllyd_node* DatastoreTransaction::computeRoot(lllyd_node* n) {
  while (n->parent != nullptr) {
    n = n->parent;
  }
  return n;
}

void DatastoreTransaction::filterMap(
    vector<string> moduleNames,
    map<Path, DatastoreDiff>& data) {
  vector<Path> toBeDeleted;
  for (const auto& m : data) {
    for (const auto& moduleName : moduleNames) {
      if (m.first.str().rfind(moduleName, 0) == 0) {
        toBeDeleted.emplace_back(m.first);
      }
    }
  }
  for (const auto& beDeleted : toBeDeleted) {
    data.erase(beDeleted);
  }
}

map<Path, DatastoreDiff> DatastoreTransaction::diff() {
  map<Path, DatastoreDiff> allDiffs;

  const vector<RootPair>& pairs =
      datastoreState->getRootAndTransactionRootPairs();

  // first - datastore root
  // second - transaction root
  for (const auto& pair : pairs) {
    if (pair.first == nullptr &&
        pair.second == nullptr) { // this would be weird
      continue;
    }

    // if everything was deleted make diff manually
    if (pair.first != nullptr && pair.second == nullptr) {
      const string path = string("/") +
          string(pair.first->schema->module->name) + ":" +
          string(pair.first->schema->name);
      allDiffs.emplace(make_pair(
          path,
          DatastoreDiff(
              parseJson(toJson(pair.first)),
              dynamic::object,
              DatastoreDiffType::deleted,
              Path(path))));
      continue;
    }

    // if everything was created (no previous state available) make diff
    // manually
    if (pair.first == nullptr && pair.second != nullptr) {
      string path = string("/") + string(pair.second->schema->module->name) +
          ":" + string(pair.second->schema->name);
      allDiffs.emplace(make_pair(
          path,
          DatastoreDiff(
              dynamic::object,
              parseJson(toJson(pair.second)),
              DatastoreDiffType::create,
              Path(path))));
      continue;
    }

    map<Path, DatastoreDiff> diffs = libyangDiff(pair.first, pair.second);
    allDiffs.insert(diffs.begin(), diffs.end());
  }

  return allDiffs;
}

map<Path, DatastoreDiff> DatastoreTransaction::diff(
    lllyd_node* a,
    lllyd_node* b) {
  map<Path, DatastoreDiff> allDiffs;

  // this covers the case when root has been completely deleted
  if (b == nullptr) {
    Path rootPath = Path(
        "/" + string(a->schema->module->name) + ":" + string(a->schema->name));
    DatastoreDiff datastoreDiff(
        readAlreadyCommitted(rootPath),
        dynamic::object(),
        DatastoreDiffType::deleted,
        rootPath);
    allDiffs.emplace(rootPath, datastoreDiff);
    return allDiffs;
  }

  // this covers the case when root was created for the first time (i.e.
  // datastoreState is empty)
  if (a == nullptr) {
    Path rootPath = Path(
        "/" + string(b->schema->module->name) + ":" + string(b->schema->name));
    DatastoreDiff datastoreDiff(
        dynamic::object(), read(rootPath), DatastoreDiffType::create, rootPath);
    allDiffs.emplace(rootPath, datastoreDiff);
    return allDiffs;
  }

  vector<string> previousModuleNames;
  while (a != nullptr && b != nullptr) {
    lllyd_node* aNext = a->next;
    lllyd_node* bNext = b->next;
    a->next = nullptr;
    b->next = nullptr;
    map<Path, DatastoreDiff> partialDiff = diff(a, b);
    std::stringstream moduleAndNodeName;
    moduleAndNodeName << "/" << a->schema->module->name << ":"
                      << a->schema->name;
    filterMap(previousModuleNames, partialDiff);
    allDiffs.insert(partialDiff.begin(), partialDiff.end());
    a->next = aNext;
    b->next = bNext;
    a = aNext;
    b = bNext;
    previousModuleNames.emplace_back(moduleAndNodeName.str());
  }

  return allDiffs;
}

map<Path, DatastoreDiff> DatastoreTransaction::libyangDiff(
    lllyd_node* a,
    lllyd_node* b) {
  checkIfCommitted();

  lllyd_difflist* difflist = lllyd_diff(a, b, LLLYD_DIFFOPT_WITHDEFAULTS);
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

vector<Path> DatastoreTransaction::getRegisteredPath(
    vector<DiffPath> registeredPaths,
    Path path,
    DatastoreDiffType type) {
  vector<Path> result;
  const vector<DiffPath> registeredParentsToNotify =
      pickClosestPath(path, registeredPaths, type);

  if (registeredParentsToNotify.empty()) {
    MLOG(MINFO) << "Unhandled event for changed path: " << path.str();
  }

  for (const auto& parentsToNotify : registeredParentsToNotify) {
    if (parentsToNotify.asterix ||
        path.getSegments().size() ==
            (parentsToNotify.path.getSegments().size() + 1) ||
        path.getSegments().size() ==
            parentsToNotify.path.getSegments().size()) {
      result.emplace_back(parentsToNotify.path);
    } else {
      MLOG(MINFO) << "seg diff: " << path.getSegments().size()
                  << " seg reg: " << parentsToNotify.path.getSegments().size()
                  << " Unhandled event for " << parentsToNotify.path.str()
                  << " changed path: " << path.str();
    }
  }

  return result;
}

vector<DiffPath> DatastoreTransaction::pickClosestPath(
    Path path,
    vector<DiffPath> paths,
    DatastoreDiffType type) {
  vector<DiffPath> result;

  // MLOG(MINFO) << "CHANGE: " << path.str();

  if (type == DatastoreDiffType::deleted) {
    for (auto registeredPath : paths) {
      if ((registeredPath.path.isChildOfUnprefixed(path) &&
           registeredPath.path.getDepth() <= path.getDepth()) ||
          (registeredPath.asterix &&
           path.isChildOfUnprefixed(registeredPath.path))) {
        registeredPath.asterix = true; // TODO think about the implications
        result.emplace_back(registeredPath);
      }
    }

    return result;
  }

  if (type == DatastoreDiffType::create) {
    for (auto registeredPath : paths) {
      if ((registeredPath.path.isChildOfUnprefixed(path) &&
           registeredPath.path.getDepth() == path.getDepth()) ||
          (registeredPath.asterix &&
           path.isChildOfUnprefixed(registeredPath.path))) {
        registeredPath.asterix = true; // TODO think about the implications
        result.emplace_back(registeredPath);
      }
    }

    return result;
  }

  unsigned int max = 0;
  DiffPath resultSoFar;
  bool found = false;
  for (const auto& p : paths) {
    if (path.segmentDistance(p.path) > max &&
        path.isChildOfUnprefixed(p.path)) {
      resultSoFar = p;
      max = path.segmentDistance(p.path);
      found = true;
    }
  }
  if (found) {
    result.emplace_back(resultSoFar);
  }

  return result;
}

DatastoreTransaction::~DatastoreTransaction() {
  if (not hasCommited) { // TODO this condition is obsolete I think
    datastoreState->freeTransactionRoot();
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

  if (not path.getFirstModuleName().hasValue()) {
    throw DatastoreException(
        "Unable to determine which tree to read from, path with module name needed");
  }
  const dynamic& aDynamic = read(
      path,
      datastoreState->getTransactionRoot(path.getFirstModuleName().value()));
  if (aDynamic == nullptr) {
    return dynamic::object(); // for diffs we need an empty object
  }
  return aDynamic;
}

dynamic DatastoreTransaction::readAlreadyCommitted(Path path) {
  if (not path.getFirstModuleName().hasValue()) {
    throw DatastoreException(
        "Unable to determine which tree to read from, path with module name needed");
  }
  const dynamic& aDynamic =
      read(path, datastoreState->getRoot(path.getFirstModuleName().value()));
  if (aDynamic == nullptr) {
    return dynamic::object(); // for diffs we need an empty object
  }
  return aDynamic;
}

dynamic DatastoreTransaction::read(Path path, lllyd_node* node) {
  llly_set* pSet = findNode(node, path.str());

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
  if (datastoreState->nothingInTransaction()) {
    DatastoreException ex(
        "Datastore is empty and no changes performed, nothing to validate");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }

  bool isValid = true;

  for (const auto& pair : datastoreState->getRootAndTransactionRootPairs()) {
    lllyd_node* nodeToValidate = pair.second;
    isValid = isValid &&
        (lllyd_validate(&nodeToValidate, datastoreTypeToLydOption(), nullptr) ==
         0);
  }

  return isValid;
}

int DatastoreTransaction::datastoreTypeToLydOption() {
  switch (datastoreState->type) {
    case operational:
      return LLLYD_OPT_GET |
          LLLYD_OPT_STRICT; // operational validation, turns off validation for
      // things like mandatory nodes, leaf-refs etc.
      // because devices do not have to support all
      // mandatory nodes (like BGP) and thus would only
      // cause false validation errors
    case config:
      return LLLYD_OPT_GETCONFIG |
          LLLYD_OPT_STRICT; // config validation with turned off checks
      // because of reasons mentioned above
  }
  return 0;
}

void DatastoreTransaction::splitToMany(
    Path p,
    dynamic input,
    vector<std::pair<string, dynamic>>& result) {
  //    MLOG(MINFO) << "bol som zavolany s : " << p.str()
  //                << " a data: " << toPrettyJson(input);

  result.emplace_back(
      std::make_pair(p.str(), input)); // adding the whole object under the
                                       // name like interface[name='0/1']

  for (const auto& item : input.items()) {
    if (item.second.isArray() || item.second.isObject()) {
      string currentPath = p.str();

      //      MLOG(MINFO) << "idem spracovat: " << currentPath
      //                  << " nazov kluca je: " << item.first.c_str();

      if (p.unkeyed().getLastSegment() !=
          item.first.asString()) { // TODO skip last overlapping segment name
        if (p.str() ==
            "/reallyempty") { // TODO problem s vymazavanim top level elementu
          // + musi byt module prefix kvoli key
          // resolution!
          currentPath = string("/") + item.first.c_str();
        } else {
          currentPath = p.str() + "/" + item.first.c_str();
        }
      }

      if (item.second.isArray()) { // if it is a YANG list i.e. dynamic
                                   // array

        for (unsigned int j = 0; j < item.second.size(); ++j) {
          dynamic arrayObject =
              item.second[j]; // go through the YANG list items
          string currentListPath =
              appendKey(arrayObject, currentPath); // append key to path
          splitToMany(
              Path(currentListPath),
              arrayObject,
              result); // recursively go into the array item
        }
      } else { // a regular object
        //   MLOG(MINFO) << "idem pridat: " << currentPath;
        result.emplace_back(std::make_pair(currentPath, input));
        splitToMany(Path(currentPath), item.second, result);
      }
    }
  }
}

string DatastoreTransaction::appendKey(dynamic data, string pathToList) {
  // MLOG(MINFO) << "hladam kluc pre: " << pathToList;
  Path pathToResolve(pathToList);
  // if the last segment already contains a key, don't append it again
  if (pathToResolve.isLastSegmentKeyed()) {
    return pathToList;
  }
  if (schemaContext.isList(pathToResolve.unkeyed())) { // if it is a list
    for (const auto& key : schemaContext.getKeys(pathToResolve.unkeyed())) {
      return pathToList + "[" + key + "='" + data[key].asString() +
          "']"; // append key to path
    }
  }

  return pathToList;
}

Path DatastoreTransaction::unifyLength(Path registeredPath, Path keyedPath) {
  if (keyedPath.getDepth() <= registeredPath.getDepth()) {
    return keyedPath;
  }

  while (keyedPath.getDepth() != registeredPath.getDepth()) {
    keyedPath = keyedPath.getParent();
  }
  return keyedPath;
}

DiffResult DatastoreTransaction::diff(vector<DiffPath> registeredPaths) {
  checkIfCommitted();
  DiffResult result;
  std::set<Path> alreadyProcessedDiff;
  const map<Path, DatastoreDiff>& diffs = diff();
  // MLOG(MINFO) << "velkost POVODNE DIFFS: " << diffs.size();

  for (const auto& diffItem : diffs) { // take libyang diffs
    const map<Path, DatastoreDiff>& smallerDiffs =
        splitDiff(diffItem.second); // split them to smaller ones
    for (const auto& smallerDiffsItem :
         smallerDiffs) { // map the smaller ones to their registered path
      vector<Path> registeredPathsToNotify = getRegisteredPath(
          registeredPaths,
          smallerDiffsItem.second.keyedPath,
          smallerDiffsItem.second.type);

      if (registeredPathsToNotify.empty()) {
        result.appendUnhandledPath(smallerDiffsItem.first);
      }

      // this is the registered path provided by the handlers
      for (const auto& registeredPathHandlingDiff : registeredPathsToNotify) {
        // we need a keyed path to read before and after state for the
        // handlers
        Path pathForReadingBeforeAfter = unifyLength(
            registeredPathHandlingDiff, smallerDiffsItem.second.keyedPath);
        if (not alreadyProcessedDiff.count(
                pathForReadingBeforeAfter)) { // we could get duplicates e.g.
                                              // multiple leafs are updated in
                                              // the same container (we get
                                              // multiple diffs from libyang,
                                              // but want one resulting diff)
          alreadyProcessedDiff.emplace(pathForReadingBeforeAfter);
        } else {
          continue;
        }
        result.diffs.emplace(std::make_pair(
            registeredPathHandlingDiff,
            DatastoreDiff(
                // we read what the state was before (no just the change but
                // the whole subtree under the registered path)
                readAlreadyCommitted(pathForReadingBeforeAfter),
                // we read what is there now (no just the change but the whole
                // subtree under the registered path)
                read(pathForReadingBeforeAfter),
                smallerDiffsItem.second.type, // we keep the type of change
                pathForReadingBeforeAfter)));
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

llly_set* DatastoreTransaction::findNode(lllyd_node* node, string path) {
  lllyd_node* tmp = node;
  lllyd_node* next;
  llly_set* pSet = nullptr;
  while (tmp != nullptr && pSet == nullptr) {
    next = tmp->next;
    tmp->next = nullptr;
    pSet = lllyd_find_path(tmp, const_cast<char*>(path.c_str()));
    tmp = next;
  }

  return pSet;
}

} // namespace devmand::channels::cli::datastore
