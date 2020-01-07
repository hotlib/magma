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
#include <devmand/devices/cli/schema/BindingContext.h>
#include <devmand/devices/cli/schema/Path.h>
#include <devmand/devices/cli/schema/SchemaContext.h>
#include <devmand/devices/cli/translation/DeviceAccess.h>
#include <devmand/devices/cli/translation/ReaderRegistry.h>
#include <folly/dynamic.h>
#include <folly/futures/Future.h>
#include <folly/json.h>
#include <ydk/types.hpp>
#include <ostream>

namespace devmand {
namespace devices {
namespace cli {

using namespace folly;
using namespace std;
using namespace ydk;

typedef std::function<
    Future<shared_ptr<Entity>>(const Path&, const DeviceAccess& device)>
    BindingReaderFunction;
// typedef vector<YLeaf> EntityKeys;
// typedef std::function<
//    Future<vector<EntityKeys>>(const Path&, const DeviceAccess& device)>
//    BindingListReaderFunction;

// TODO extract client-facing SPI into a separate *SPI header file

class BindingReader {
 public:
  virtual Future<shared_ptr<Entity>> read(
      const Path& path,
      const DeviceAccess& device) const = 0;
};

// class BindingListReader : public BindingReader {
// public:
//  virtual Future<vector<EntityKeys>> readKeys(
//      const Path& path,
//      const DeviceAccess& device) const = 0;
//
//  virtual Future<shared_ptr<Entity>> read(
//      const Path& pathWithKeys,
//      const DeviceAccess& device) const {
//    (void)device;
//    return Future<dynamic>(pathWithKeys.getKeys());
//  };
//};

class BindingLambdaReader : public BindingReader {
 private:
  BindingReaderFunction lambda;

 public:
  BindingLambdaReader(BindingReaderFunction _lambda) : lambda(_lambda){};
  ~BindingLambdaReader() = default;
  BindingLambdaReader(const BindingLambdaReader&) = delete;
  BindingLambdaReader& operator=(const BindingLambdaReader&) = delete;
  BindingLambdaReader(BindingLambdaReader&&) = delete;
  BindingLambdaReader& operator=(BindingLambdaReader&&) = delete;

  Future<shared_ptr<Entity>> read(const Path& path, const DeviceAccess& device)
      const override {
    return lambda(path, device);
  }
};

// class BindingLambdaListReader : public ListReader {
// private:
//  BindingListReaderFunction keysLambda;
//  BindingReaderFunction lambda;
//
// public:
//  BindingLambdaListReader(BindingListReaderFunction _keysLambda,
//  BindingReaderFunction _lambda)
//      : keysLambda(_keysLambda), lambda(_lambda){};
//
//  Future<vector<EntityKeys>> readKeys(const Path& path, const DeviceAccess&
//  device)
//      const override {
//    return keysLambda(path, device);
//  }
//
//  Future<dynamic> read(const Path& path, const DeviceAccess& device)
//      const override {
//    return lambda(path, device);
//  }
//};

class BindingReaderAdapter : public Reader {
 private:
  shared_ptr<BindingReader> bindingReader;
  BindingContext& context;

 public:
  BindingReaderAdapter(
      shared_ptr<BindingReader> _bindingReader,
      BindingContext& _context);

  Future<dynamic> read(const Path& path, const DeviceAccess& device)
      const override;
};

class BindingReaderRegistryBuilder {
 private:
  ReaderRegistryBuilder& domBuilder;
  BindingContext& context;

 public:
  BindingReaderRegistryBuilder(
      ReaderRegistryBuilder& _domBuilder,
      BindingContext& _context)
      : domBuilder(_domBuilder), context(_context){};
  // No validation against schema will be performed with NO_MODELS context
  ~BindingReaderRegistryBuilder() = default;
  BindingReaderRegistryBuilder(const BindingReaderRegistryBuilder&) = delete;
  BindingReaderRegistryBuilder& operator=(const BindingReaderRegistryBuilder&) =
      delete;
  BindingReaderRegistryBuilder(BindingReaderRegistryBuilder&&) = delete;
  BindingReaderRegistryBuilder& operator=(BindingReaderRegistryBuilder&&) =
      delete;

  // TODO support subtree / wildcarded readers

  template <
      typename T,
      typename enable_if<is_base_of<BindingReader, T>{}, int>::type = false>
  void add(Path path, shared_ptr<T> reader) {
    addReader(path, static_pointer_cast<BindingReader>(reader));
  }

  //  template <
  //      typename T,
  //      typename enable_if<is_base_of<BindingListReader, T>{}, int>::type =
  //      false>
  //  void addList(Path path, shared_ptr<T> reader) {
  //    addReader(path, static_pointer_cast<Reader>(reader));
  //  }

  void add(Path path, BindingReaderFunction lambda);
  //  void addList(
  //      Path path,
  //      BindingListReaderFunction keysLambda,
  //      BindingReaderFunction lambda = [](auto path, auto device) {
  //        (void)device;
  //        return path.getKeys();
  //      });

 private:
  void addReader(Path path, shared_ptr<BindingReader> reader);
};

#define BINDING(reg, ctx) BindingReaderRegistryBuilder(reg, ctx)

} // namespace cli
} // namespace devices
} // namespace devmand
