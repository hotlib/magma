// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/devices/cli/schema/ModelRegistry.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <ydk/types.hpp>

namespace devmand::channels::cli::codecs {
using devmand::devices::cli::Model;
using devmand::devices::cli::ModelRegistry;
using folly::dynamic;
using folly::parseJson;
using folly::toJson;
using std::make_shared;
using std::shared_ptr;
using ydk::Entity;

class YdkDynamicCodec {
  shared_ptr<ModelRegistry> mreg;

 public:
  explicit YdkDynamicCodec(shared_ptr<ModelRegistry> _mreg);
  dynamic convert(shared_ptr<Entity> entity);
  template <typename T>
  shared_ptr<T> convert2(dynamic entity) {
      //TODO neviem co s tymto
//    auto& bundle = mreg->getBundle(Model::OPENCONFIG_0_1_6);
//    const shared_ptr<T>& ydkModel = make_shared<T>();
//    return std::static_pointer_cast<T>(bundle.decode(toJson(entity), ydkModel));
return nullptr;
  }
};
} // namespace devmand::channels::cli::codecs
