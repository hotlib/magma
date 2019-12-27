// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <ydk/codec_provider.hpp>
#include <ydk/codec_service.hpp>
#include <ydk/json_subtree_codec.hpp>
#include <ydk/path_api.hpp>
#include <mutex>
#include <regex>
#include <sstream>

namespace devmand {
namespace devices {
namespace cli {

using namespace std;
using namespace ydk;
using namespace ydk::path;

class Model {
 public:
  Model() = delete;
  ~Model() = default;

 protected:
  explicit Model(string _dir) : dir(_dir) {}

 private:
  const string dir;

 public:
  const string& getDir() const {
    return dir;
  }
  bool operator<(const Model& x) const {
    return dir < x.dir;
  }

  static const Model OPENCONFIG_0_1_6;
  static const Model IETF_0_1_5;
};

class RootDataNode {
 private:
  map<string, shared_ptr<DataNode>> children;

 public:
  RootDataNode(map<string, shared_ptr<DataNode>> _children)
      : children(_children){};
  //  RootDataNode(map<string, shared_ptr<Entity>> _children, RootSchemaNode&
  //  rootSchema);
  RootDataNode(DataNode& dataNode);
  map<string, shared_ptr<DataNode>> getChildren();
};

class Bundle {
 public:
  explicit Bundle(const Model& model);
  Bundle() = delete;
  ~Bundle() = default;
  Bundle(const Bundle&) = delete;
  Bundle& operator=(const Bundle&) = delete;
  Bundle(Bundle&&) = delete;
  Bundle& operator=(Bundle&&) = delete;

 private:
  mutex lock; // A bundle is expected to be shared, protect it
  Repository repo;
  CodecServiceProvider codecServiceProvider;
  JsonSubtreeCodec jsonSubtreeCodec;
  Codec jsonCodec;

 public:
  // Binding aware API
  string encode(Entity& entity);
  shared_ptr<Entity> decode(const string& payload, shared_ptr<Entity> pointer);

  // Binding independent (DOM) API
  string encode(DataNode& dataNode);
  shared_ptr<DataNode> decode(const string& payload);

  // Binding independent (DOM) API for ROOT
  string encodeRoot(RootDataNode& dataNode);
  shared_ptr<RootDataNode> decodeRoot(const string& payload);

  // DOM -> BA
  //  shared_ptr<DataNode> toDOM(shared_ptr<Entity> entity);
  //  shared_ptr<Entity> fromDOM(shared_ptr<DataNode>);

  SchemaNode& findSchemaNode(string path);
  RootSchemaNode& rootSchemaNode();
};

class Path {
 private:
  Path(string _path) : path(_path){};
  string path;

 public:
  static Path makePath(Bundle& bundle, string path);

  friend ostream& operator<<(ostream& _stream, Path const& p) {
    _stream << p.path;
    return _stream;
  }
};

class ModelRegistry {
 public:
  ModelRegistry();
  ~ModelRegistry();
  ModelRegistry(const ModelRegistry&) = delete;
  ModelRegistry& operator=(const ModelRegistry&) = delete;
  ModelRegistry(ModelRegistry&&) = delete;
  ModelRegistry& operator=(ModelRegistry&&) = delete;

 public:
  Bundle& getBundle(const Model& model);
  ulong cacheSize();

 private:
  mutex lock; // A bundle is expected to be shared, protect it
  map<string, Bundle> cache;
};

class SerializationException : public exception {
 private:
  string msg;

 public:
  SerializationException(string _cause) {
    msg = _cause;
  };

  SerializationException(Entity& _entity, string _cause) {
    stringstream buffer;
    buffer << "Failed to encode: " << typeid(_entity).name() << " due to "
           << _cause;
    msg = buffer.str();
  };

  SerializationException(DataNode& _entity, string _cause) {
    stringstream buffer;
    buffer << "Failed to encode: " << typeid(_entity).name() << " due to "
           << _cause;
    msg = buffer.str();
  };

  SerializationException(shared_ptr<Entity>& _entity, string _cause)
      : SerializationException(*_entity, _cause){};

 public:
  const char* what() const throw() override {
    return msg.c_str();
  }
};

class SchemaException : public exception {
 private:
  string msg;

 public:
  SchemaException(string prefix, string path, string _cause) {
    stringstream buffer;
    buffer << prefix << path << " due to " << _cause;
    msg = buffer.str();
  };

 public:
  const char* what() const throw() override {
    return msg.c_str();
  }
};

} // namespace cli
} // namespace devices
} // namespace devmand
