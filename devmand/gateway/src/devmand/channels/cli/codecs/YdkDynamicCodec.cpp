// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/channels/cli/codecs/YdkDynamicCodec.h>

namespace devmand::channels::cli::codecs {

YdkDynamicCodec::YdkDynamicCodec(const shared_ptr<ModelRegistry> _mreg)
    : mreg(_mreg) {}

dynamic YdkDynamicCodec::convert(shared_ptr<Entity> entity) {
  (void)entity;
  // TODO neviem co s tymto
  //  auto& bundle = mreg->getBundle(Model::OPENCONFIG_0_1_6);
  //  return parseJson(bundle.encode(*entity));
  return nullptr;
}
} // namespace devmand::channels::cli::codecs