// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <devmand/devices/cli/translation/Path.h>
#include <libyang/libyang.h>

namespace devmand::channels::cli {

using devmand::devices::cli::Path;
class SchemaContext {
 private:
  llly_ctx* ctx;

 public:
  SchemaContext() {
    ctx = llly_ctx_new("/usr/share/openconfig@0.1.6/", 0);
    llly_ctx_load_module(ctx, "iana-if-type", NULL);
    llly_ctx_load_module(ctx, "openconfig-interfaces", NULL);
  }

 private:
  bool isList(Path path) {
      (void) path;
    return false;
  }
};
} // namespace devmand::channels::cli