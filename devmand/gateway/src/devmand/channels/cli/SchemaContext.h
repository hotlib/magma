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
using std::vector;

class SchemaContext {
 private:
  llly_ctx* ctx;

  llly_set* getNodes(Path p) {
    if (not isPathValid(p)) {
      throw std::runtime_error("Path not valid");
    }
    char* schemaPath = llly_path_data2schema(
        ctx,
        const_cast<char*>(p.str().c_str())); // TODO who cleans up the char *?
    MLOG(MINFO) << "schemaPath: " << schemaPath;
    llly_set* pSet = llly_ctx_find_path(
        ctx,
        "/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:subinterfaces/openconfig-interfaces:subinterface/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address");

    if (pSet == nullptr ||
        pSet->number != 1) { // TODO this probably can't happen
      throw std::runtime_error(
          "There should have been a YANG model node for the query!");
    }

    return pSet;
  }

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
    llly_ctx_get_module(ctx, "openconfig-if-ip", NULL, 0);
  }

  bool isPathValid(Path path) {
    return llly_path_data2schema(ctx, const_cast<char*>(path.str().c_str())) !=
        nullptr;
  }

  bool isList(Path p) {
    llly_set* pSet = getNodes(p);
    auto result = pSet->set.s[0]->nodetype == LLLYS_LIST;
    llly_set_free(pSet);
    return result;
  }

  vector<string> getKeys(Path p) {
    llly_set* pSet = getNodes(p);
    lllys_node* node = pSet->set.s[0];
    vector<string> result;

    if (node->nodetype == LLLYS_LIST) {
      auto* list = (lllys_node_list*)node;
      for (uint8_t i = 0; i < list->keys_size; i++) {
        result.emplace_back(string(list->keys[i]->name));
      }
    }
    llly_set_free(pSet);
    return result;
  }
};
} // namespace devmand::channels::cli