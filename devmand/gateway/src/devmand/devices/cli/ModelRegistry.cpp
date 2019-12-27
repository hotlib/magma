// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/devices/cli/ModelRegistry.h>
#include <ydk/codec_provider.hpp>
#include <ydk/entity_data_node_walker.hpp>
#include <sstream>
#include <typeinfo>

namespace devmand {
namespace devices {
namespace cli {

using namespace ydk;
using namespace std;
using namespace ydk::path;

Path Path::makePath(Bundle& bundle, string path) {
  try {
    // Validate
    bundle.findSchemaNode(path);
    return Path(path);
  } catch (SchemaException& e) {
    throw;
  }
}

const Model Model::OPENCONFIG_0_1_6 = Model("/usr/share/openconfig@0.1.6");
const Model Model::IETF_0_1_5 = Model("/usr/share/ietf@0.1.5");

static void (*const noop)() = []() {};

Bundle::Bundle(const Model& model)
    : repo(Repository(model.getDir(), ModelCachingOption::COMMON)),
      codecServiceProvider(CodecServiceProvider(repo, EncodingFormat::JSON)),
      jsonSubtreeCodec(JsonSubtreeCodec()),
      jsonCodec(Codec()) {
  codecServiceProvider.initialize(model.getDir(), model.getDir(), noop);
}

string Bundle::encode(Entity& entity) {
  try {
    DataNode& node = get_data_node_from_entity(entity, rootSchemaNode());
    return encode(node);
  } catch (std::exception& e) {
    throw SerializationException(entity, e.what());
  }
}

string Bundle::encode(DataNode& dataNode) {
  lock_guard<std::mutex> lg(lock);

  try {
    string json = jsonCodec.encode(dataNode, EncodingFormat::JSON, true);
    return json;
  } catch (std::exception& e) {
    throw SerializationException(dataNode, e.what());
  }
}

string Bundle::encodeRoot(RootDataNode& dataNode) {
  try {
    stringstream output;
    output << "{";
    for (auto& rE : dataNode.getChildren()) {
      string childJson = encode(*rE.second);
      output << childJson.substr(1, childJson.length() - 2);
    }
    output << "}" << endl;

    return output.str();
  } catch (std::exception& e) {
    throw SerializationException(e.what());
  }
}

shared_ptr<Entity> Bundle::decode(
    const string& payload,
    shared_ptr<Entity> pointer) {
  try {
    shared_ptr<DataNode> node = decode(payload);
    get_entity_from_data_node(node.get(), pointer);
    return pointer;
  } catch (std::exception& e) {
    throw SerializationException(pointer, e.what());
  }
}

shared_ptr<DataNode> Bundle::decode(const string& payload) {
  lock_guard<std::mutex> lg(lock);

  try {
    return jsonCodec.decode(rootSchemaNode(), payload, EncodingFormat::JSON);
  } catch (std::exception& e) {
    throw SerializationException(e.what());
  }
}

shared_ptr<RootDataNode> Bundle::decodeRoot(const string& payload) {
  try {
    shared_ptr<DataNode> ptr =
        jsonCodec.decode(rootSchemaNode(), payload, EncodingFormat::JSON);
    return make_shared<RootDataNode>(*ptr);
  } catch (std::exception& e) {
    throw SerializationException(e.what());
  }
}

SchemaNode& Bundle::findSchemaNode(std::string path) {
  try {
    RootSchemaNode& node = codecServiceProvider.get_root_schema_for_bundle("");
    return *(node.find(path).back());
  } catch (std::exception& e) {
    throw SchemaException("Unable to find in schema: ", path, e.what());
  }
}

RootSchemaNode& Bundle::rootSchemaNode() {
  return codecServiceProvider.get_root_schema_for_bundle("");
}

ModelRegistry::ModelRegistry() {
  // Set plugin directory for libyang according to:
  // https://github.com/CESNET/libyang/blob/c38295963669219b7aad2618b9f1dd31fa667caa/FAQ.md
  // and CMakeLists.ydk
  setenv("LIBYANG_EXTENSIONS_PLUGINS_DIR", LIBYANG_PLUGINS_DIR, false);
}

ModelRegistry::~ModelRegistry() {
  cache.clear();
}

ulong ModelRegistry::cacheSize() {
  lock_guard<std::mutex> lg(lock);

  return cache.size();
}

Bundle& ModelRegistry::getBundle(const Model& model) {
  lock_guard<std::mutex> lg(lock);

  auto it = cache.find(model.getDir());
  if (it != cache.end()) {
    return it->second;
  } else {
    auto pair = cache.emplace(model.getDir(), model);
    return pair.first->second;
  }
}

// Root entity

map<string, shared_ptr<DataNode>> RootDataNode::getChildren() {
  return children;
}

// RootDataNode::RootDataNode(
//    map<string, shared_ptr<Entity>> _children,
//    RootSchemaNode& _rootSchema) {
//  map<string, shared_ptr<DataNode>> transformed;
//  for (auto& childEntity : _children) {
//    DataNode& node =
//        get_data_node_from_entity(*(childEntity.second), _rootSchema);
//    transformed.emplace(childEntity.first, node);
//  }
//
//  children = transformed;
//}

RootDataNode::RootDataNode(DataNode& dataNode) {
  map<string, shared_ptr<DataNode>> transformed;
  for (auto& childDNode : dataNode.get_children()) {
    transformed.emplace(childDNode->get_path(), childDNode);
  }

  children = transformed;
}

} // namespace cli
} // namespace devices
} // namespace devmand
