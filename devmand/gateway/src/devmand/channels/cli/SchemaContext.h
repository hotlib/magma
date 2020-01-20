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
using std::string;

class SchemaContext {
 private:
  llly_ctx* ctx;

 public:
  SchemaContext() {
    ctx = llly_ctx_new("/usr/share/openconfig@0.1.6/", LLLY_CTX_ALLIMPLEMENTED);
    llly_ctx_load_module(ctx, "iana-if-type", NULL);
    llly_ctx_load_module(ctx, "ietf-interfaces", NULL);
    llly_ctx_load_module(ctx, "ietf-yang-types", NULL);
    llly_ctx_load_module(ctx, "openconfig-extensions", NULL);
    llly_ctx_load_module(ctx, "openconfig-if-ip", NULL);
    llly_ctx_load_module(ctx, "openconfig-vlan", NULL);
    llly_ctx_load_module(ctx, "openconfig-vlan-types", NULL);
    llly_ctx_load_module(ctx, "openconfig-if-aggregate", NULL);
    llly_ctx_load_module(ctx, "openconfig-interfaces", NULL);
    llly_ctx_load_module(ctx, "openconfig-if-ethernet", NULL);
    const lllys_module* pModule =
        llly_ctx_get_module(ctx, "openconfig-if-ip", NULL, 0);
    if (pModule == NULL) {
      MLOG(MINFO) << "je to null";
    } else {
      MLOG(MINFO) << "NENI to null";
    }
  }

  bool isPathValid(Path path) {
    return llly_path_data2schema(
               ctx, const_cast<char*>(path.toString().c_str())) != nullptr;
  }

  bool isList(Path p){
      return getNode(p)->nodetype == LLLYS_LIST;
  }

  lllys_node* getNode(Path p) {
    if (not isPathValid(p)) {
        throw std::runtime_error("Path not valid");
    }
    ly_set* pSet =
        llly_ctx_find_path(ctx, const_cast<char*>(path.toString().c_str()));

    if (pSet->number != 1) {
      throw std::runtime_error(
          "There should have been a YANG model node for the query!"); // TODO
                                                                      // this
                                                                      // probably
                                                                      // can't
                                                                      // happen
    }

    return pSet->set.s[0];
  }

  //  std::vector getKeys(Path p) {
  //    const lys_module* pModule = ly_ctx_get_module(
  //        ctx,
  //        "openconfig-interfaces",
  //        NULL,
  //        0); // TODO name of model hardcoded
  //    ly_set* pSet =
  //        lys_find_path(pModule, pModule->data,
  //        const_cast<char*>(path.c_str()));
  //
  //    if (pSet->number != 1) {
  //      throw std::runtime_error(
  //          "There should have been a YANG model node for the query!");
  //    }
  //
  //    if (pSet->set.s[0]->nodetype != LYS_LIST) {
  //      return nullptr;
  //    }
  //
  //    auto* list = (lys_node_list*)pSet->set.s[0];
  //
  //    if (list->keys_size == 0) {
  //      return nullptr;
  //    }
  //
  //    for (uint8_t i = 0; i < list->keys_size; i++) {
  //      MLOG(MINFO) << "meno kluca: " << list->keys[i]->name;
  //    }
  //
  //    return list;
  //  }

 private:
  bool isList(Path path) {
    (void)path;
    return false;
  }
};
} // namespace devmand::channels::cli