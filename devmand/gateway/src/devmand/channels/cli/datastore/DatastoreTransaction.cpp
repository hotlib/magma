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
  if (path.empty() || root == nullptr) {
    return false;
  }

  if (Path::ROOT == p ||
      p.getDepth() == 1) { // TODO SOLVE FOR MULTITREE p.getDepth() == 1
    freeRoot();
    return true;
  }

  lllyd_node* tmp = root;
  lllyd_node* next = nullptr;
  llly_set* pSet = nullptr;
  while (tmp != nullptr && pSet == nullptr) {
    next = tmp->next;
    tmp->next = nullptr;
    pSet = lllyd_find_path(tmp, const_cast<char*>(path.c_str()));
    tmp->next = next;
    tmp = next;
  }

  if (pSet == nullptr) {
    MLOG(MDEBUG) << "Nothing to delete, " + path + " not found";
    return false;
  } else {
    MLOG(MDEBUG) << "Deleting " << pSet->number << " subtrees";
  }
  for (unsigned int j = 0; j < pSet->number; ++j) {
    freeRoot(pSet->set.d[j]);
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

void DatastoreTransaction::merge(const Path path, const dynamic& aDynamic) {
  checkIfCommitted();
  dynamic withParents = appendAllParents(path, aDynamic);
  lllyd_node* pNode = dynamic2lydNode(withParents);

  if (root == nullptr) {
    root = pNode;
  } else {
    lllyd_node* tmp = root;
    lllyd_node* next;
    while (tmp != nullptr) {
      next = tmp->next;
      tmp->next = nullptr;
      lllyd_merge(tmp, pNode, 0);
      tmp->next = next;
      tmp = next;
    }
    freeRoot(pNode);
  }
}

void DatastoreTransaction::commit() {
  checkIfCommitted();

  validateBeforeCommit();
  lllyd_node* rootToBeMerged = computeRoot(
      root); // need the real root for convenience and copy via lllyd_dup
  if (!datastoreState->isEmpty()) {
    freeRoot(datastoreState->root);
    datastoreState->root = nullptr;
  }
  datastoreState->root = rootToBeMerged;

  hasCommited.store(true);
  datastoreState->transactionUnderway.store(false);
}

void DatastoreTransaction::abort() {
  checkIfCommitted();

  freeRoot();

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
    shared_ptr<DatastoreState> _datastoreState,
    SchemaContext& _schemaContext)
    : datastoreState(_datastoreState), schemaContext(_schemaContext) {
  if (not datastoreState->isEmpty()) {
    root = lllyd_dup(datastoreState->root, 1);

    lllyd_node* tmp = datastoreState->root->next;
    lllyd_node* tmpRoot = root;
    while (tmp != nullptr) {
      tmpRoot->next = lllyd_dup(tmp, 1);
      tmpRoot = tmpRoot->next;
      tmp = tmp->next;
    }
  }
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
  lllyd_node* a = datastoreState->root;
  lllyd_node* b = root;

  // create/delete root elementu => datastoreState->root alebo root budu NULL
  if (b == nullptr) { // todo toto je delete rootu (este chyba create)
    Path rootPath = Path("/" + string(datastoreState->root->schema->name));
    DatastoreDiff datastoreDiff(
        readAlreadyCommitted(rootPath),
        dynamic::object(),
        DatastoreDiffType::deleted,
        rootPath);
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

map<Path, DatastoreDiff> DatastoreTransaction::diff(
    lllyd_node* a,
    lllyd_node* b) {
  checkIfCommitted();
  if (datastoreState->isEmpty()) {
    DatastoreException ex("Unable to diff, datastore tree does not yet exist");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }
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

  if (type == DatastoreDiffType::deleted) {
    for (auto registeredPath : paths) {
      // MLOG(MINFO) << "registeredPath.path.isChildOf(path): " <<
      // registeredPath.path.isChildOf(path) << " registeredPath: " <<
      // registeredPath.path.str()  << " changed path: " << path;
      if (registeredPath.path.isChildOf(path)) {
        registeredPath.asterix = true; // TODO hack
        result.emplace_back(registeredPath);
      }
    }
  }

  unsigned int max = 0;
  DiffPath resultSoFar;
  bool found = false;
  for (const auto& p : paths) {
    if (path.segmentDistance(p.path) > max && path.isChildOf(p.path)) {
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
  if (not hasCommited) {
    freeRoot();
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

// TODO toto je len tak
//    string DatastoreTransaction::appendToPath(lllyd_node* node, string
//    pathSoFar) {
//
//        llly_set* pSet = findNode(node, pathSoFar);
//
//        if (pSet == nullptr) {
//            return nullptr;
//        }
//
//        if (pSet->number == 0) {
//            llly_set_free(pSet);
//            return nullptr;
//        }
//
//        if (pSet->number > 1) {
//            llly_set_free(pSet);
//
//            DatastoreException ex(
//                    "Too many results from path: " + path.str() +
//                    " , query must target a unique element");
//            MLOG(MWARNING) << ex.what();
//            throw ex;
//        }
//
//        const string& json = toJson(pSet->set.d[0]);
//        llly_set_free(pSet);
//        return parseJson(json);
//
//
//
//        std::stringstream path;
//        path << "/" << node->schema->module->name << ":" <<
//        node->schema->name; if (node->schema->nodetype == LLLYS_LIST) {
//            addKeysToPath(node, path);
//        }
//        path << pathSoFar;
//        if (node->parent == nullptr) {
//            return path.str();
//        }
//        return buildFullPath(node->parent, path.str());
//    }

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
  const dynamic& aDynamic = read(path, root);
  if (aDynamic == nullptr) {
    return dynamic::object(); // for diffs we need an empty object
  }
  return aDynamic;
}

dynamic DatastoreTransaction::readAlreadyCommitted(Path path) {
  if (datastoreState->root == nullptr) {
    return dynamic::object();
  }

  const dynamic& result = read(path, datastoreState->root);
  if (result == nullptr) {
    return dynamic::object(); // for diffs we need an empty object
  }

  return result;
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
  if (root == nullptr) {
    DatastoreException ex(
        "Datastore is empty and no changes performed, nothing to validate");
    MLOG(MWARNING) << ex.what();
    throw ex;
  }

  lllyd_node* tmp = root;
  lllyd_node* next = nullptr;
  bool isValid = true;
  while (tmp != nullptr) {
    next = tmp->next;
    tmp->next = nullptr;
    isValid = isValid &&
        (lllyd_validate(&tmp, datastoreTypeToLydOption(), nullptr) == 0);
    tmp->next = next;
    tmp = next;
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
  //   MLOG(MINFO) << "bol som zavolany s : " << p.str() << " a data: " <<
  //   toPrettyJson(input);

  if (input.isArray()) {
    throw DatastoreException("THIS CAN NEVER HAPPEN!!!!!"); // TODO premysliet
    for (const auto& item : input) {
      splitToMany(p, item, result);
    }
  } else if (input.isObject()) {

    if (p.str() != "/reallyempty") {
        result.emplace_back(std::make_pair(
                p.str(), input)); // adding the whole object under the name
    }

    for (const auto& item : input.items()) {
      if (item.second.isArray() || item.second.isObject()) {
        string currentPath = p.str();
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
            MLOG(MINFO) << "idem vlozit " << currentPath;
            result.emplace_back(std::make_pair(currentPath, input));
            splitToMany(Path(currentPath), item.second, result);
          }
        }
      }
    }
  }
}

string DatastoreTransaction::appendKey(dynamic data, string pathToList) {
  if (schemaContext.isList(Path(pathToList))) { // if it is a list
    for (const auto& key : schemaContext.getKeys(Path(pathToList))) {
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
  for (const auto& diffItem : diffs) { // take libyang diffs
    const map<Path, DatastoreDiff>& smallerDiffs =
        splitDiff(diffItem.second); // split them to smaller ones
    for (const auto& smallerDiffsItem :
         smallerDiffs) { // map the smaller ones to their registered path
      vector<Path> registeredPathsToNotify = getRegisteredPath(
          registeredPaths,
          smallerDiffsItem.first,
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
    const Path& pathToSend =
        diff.keyedPath.getDepth() == 1 ? "" : diff.keyedPath;
    MLOG(MINFO) << "idem poslat: " << pathToSend;
    splitToMany(
        pathToSend,
        diff.after,
        split); // TODO neposielam ked je top level diff.keyedPath
    for (const auto& s : split) {
      diffs.emplace(
          s.first,
          DatastoreDiff(diff.before, s.second, diff.type, Path(s.first)));
    }
    return diffs;
  } else if (diff.type == DatastoreDiffType::deleted) {
    const Path& pathToSend = diff.keyedPath.getDepth() == 1
        ? Path("/reallyempty")
        : diff.keyedPath; // TODO fake top level element marker
    splitToMany(pathToSend, diff.before, split);
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

void DatastoreTransaction::freeRoot() {
  freeRoot(root);
  root = nullptr;
}

void DatastoreTransaction::freeRoot(lllyd_node* r) {
  lllyd_node* next = nullptr;
  while (r != nullptr) {
    next = r->next;
    r->next = nullptr;
    lllyd_free(r);
    r->next = next;
    r = r->next;
  }
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

//    lllyd_node * DatastoreTransaction::find(dynamic entity) {
//        return nullptr;
//    }

} // namespace devmand::channels::cli::datastore
