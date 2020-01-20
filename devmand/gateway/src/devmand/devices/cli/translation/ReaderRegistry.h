// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#define LOG_WITH_GLOG
#include <magma_logging.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/labeled_graph.hpp>
#include <devmand/devices/cli/translation/Path.h>
#include <folly/dynamic.h>
#include <folly/futures/Future.h>
#include <folly/json.h>
#include <ostream>

namespace devmand {
namespace devices {
namespace cli {

using namespace folly;
using namespace std;

typedef std::function<Future<dynamic>(const Path&)> ReaderFunction;
typedef std::function<Future<vector<dynamic>>(const Path&)> ListReaderFunction;

// TODO extract client-facing SPI into a separate *SPI header file

class Reader {
 public:
  virtual Future<dynamic> read(const Path& path) const = 0;
};

class ListReader : public Reader {
 public:
  virtual Future<vector<dynamic>> readKeys(const Path& path) const = 0;
  virtual Future<dynamic> read(const Path& pathWithKeys) const {
    return Future<dynamic>(pathWithKeys.getKeys());
  };
};

class LambdaReader : public Reader {
 private:
  ReaderFunction lambda;

 public:
  LambdaReader(ReaderFunction _lambda) : lambda(_lambda){};
  ~LambdaReader() = default;
  LambdaReader(const LambdaReader&) = delete;
  LambdaReader& operator=(const LambdaReader&) = delete;
  LambdaReader(LambdaReader&&) = delete;
  LambdaReader& operator=(LambdaReader&&) = delete;

  Future<dynamic> read(const Path& path) const override {
    return lambda(path);
  }
};

class LambdaListReader : public ListReader {
 private:
  ListReaderFunction keysLambda;
  ReaderFunction lambda;

 public:
  LambdaListReader(ListReaderFunction _keysLambda, ReaderFunction _lambda)
      : keysLambda(_keysLambda), lambda(_lambda){};

  Future<vector<dynamic>> readKeys(const Path& path) const override {
    return keysLambda(path);
  }

  Future<dynamic> read(const Path& path) const override {
    return lambda(path);
  }
};

class StructuralReader : public Reader {
 public:
  static const shared_ptr<Reader> INSTANCE;

 public:
  StructuralReader() = default;
  ~StructuralReader() = default;
  StructuralReader(const StructuralReader&) = delete;
  StructuralReader& operator=(const StructuralReader&) = delete;
  StructuralReader(StructuralReader&&) = delete;
  StructuralReader& operator=(StructuralReader&&) = delete;

  Future<dynamic> read(const Path& path) const override {
    (void)path;
    return Future<dynamic>(dynamic::object());
  }
};

class CompositeReader : public Reader {
 public:
  typedef map<string, unique_ptr<CompositeReader>> Children;

 protected:
  Path registeredPath;
  shared_ptr<Reader> clientReader;
  Children children;

 public:
  CompositeReader(
      Path _registeredPath,
      shared_ptr<Reader> _clientReader,
      Children&& _children);
  ~CompositeReader() = default;
  CompositeReader(const CompositeReader&) = delete;
  CompositeReader& operator=(const CompositeReader&) = delete;
  CompositeReader(CompositeReader&&) = delete;
  CompositeReader& operator=(CompositeReader&&) = delete;

  Future<dynamic> read(const Path& path) const override;

  friend ostream& operator<<(ostream& os, const CompositeReader& reader);

 protected:
  bool shouldDelegate(const Path& path) const;
};

class CompositeListReader : public CompositeReader {
 public:
  CompositeListReader(
      Path _registeredPath,
      shared_ptr<Reader> _clientReader,
      Children&& _children);

  Future<dynamic> read(const Path& path) const override;

 private:
  Future<dynamic> readSingle(const Path& path) const;
};

class ReadException : public runtime_error {
 public:
  ReadException(Path path, string reason)
      : runtime_error(
          "Unable to read path: " + path.str() + " due to: " + reason){};
};

class ReaderRegistry {
 private:
  unique_ptr<CompositeReader> rootReader;

 public:
  ReaderRegistry(unique_ptr<CompositeReader>&& _rootReader)
      : rootReader(forward<unique_ptr<CompositeReader>>(_rootReader)){};
  ~ReaderRegistry() = default;
  ReaderRegistry(const ReaderRegistry&) = delete;
  ReaderRegistry& operator=(const ReaderRegistry&) = delete;
  ReaderRegistry(ReaderRegistry&&) = delete;
  ReaderRegistry& operator=(ReaderRegistry&&) = delete;

  Future<dynamic> readConfiguration(const Path& path) const;
  Future<dynamic> readState(const Path& path) const;

  friend ostream& operator<<(ostream& os, const ReaderRegistry& registry);
};

struct PathVertex {
  Path path = "/UNINITIALIZED";
};

typedef boost::labeled_graph<
    boost::adjacency_list<
        boost::vecS,
        boost::vecS,
        boost::bidirectionalS,
        PathVertex>,
    Path>
    YangHierarchy;

class ReaderRegistryException : public runtime_error {
 public:
  ReaderRegistryException(string reason) : runtime_error(reason){};
};

class ReaderRegistryBuilder {
 private:
  map<Path, shared_ptr<Reader>> allReaders;

 public:
  ReaderRegistryBuilder() = default;
  ~ReaderRegistryBuilder() = default;
  ReaderRegistryBuilder(const ReaderRegistryBuilder&) = delete;
  ReaderRegistryBuilder& operator=(const ReaderRegistryBuilder&) = delete;
  ReaderRegistryBuilder(ReaderRegistryBuilder&&) = delete;
  ReaderRegistryBuilder& operator=(ReaderRegistryBuilder&&) = delete;

  unique_ptr<ReaderRegistry> build();

  // TODO support subtree / wildcarded readers
  // TODO differentiate between config and operational readers based on path +
  // schema

  template <
      typename T,
      typename enable_if<is_base_of<Reader, T>{}, int>::type = false>
  void add(Path path, shared_ptr<T> reader) {
    addReader(path, static_pointer_cast<Reader>(reader));
  }

  template <
      typename T,
      typename enable_if<is_base_of<ListReader, T>{}, int>::type = false>
  void addList(Path path, shared_ptr<T> reader) {
    addReader(path, static_pointer_cast<Reader>(reader));
  }

  void add(Path path, ReaderFunction lambda);
  void addList(
      Path path,
      ListReaderFunction keysLambda,
      ReaderFunction lambda = [](auto path) { return path.getKeys(); });

 private:
  void addReader(Path path, shared_ptr<Reader> reader);
  unique_ptr<CompositeReader> createCompositeReader(
      const YangHierarchy& pathGraph,
      const PathVertex& vertex) const;
};

} // namespace cli
} // namespace devices
} // namespace devmand
