// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <libyang/libyang.h>
#include <ydk/types.hpp>
#include <devmand/devices/cli/ModelRegistry.h>
#include <magma_logging.h>

using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::make_shared;
using ydk::Entity;
using LeafVector = std::vector<pair<string, string>>;
using devmand::devices::cli::ModelRegistry;
using devmand::devices::cli::Model;

namespace devmand {
namespace channels {
namespace cli {

class DatastoreTransaction {
 private:
  ly_ctx* ctx;
  lyd_node* root;
  shared_ptr<ModelRegistry> mreg;

 private:
  void writeLeafs(LeafVector& leafs);
  void print();
  string toJson(lyd_node* initial);

 public:
  DatastoreTransaction(shared_ptr<ModelRegistry> mreg);

  template<typename T>
  shared_ptr<Entity> read(string path) {
      ly_set *pSet = lyd_find_path(root, const_cast<char *>(path.c_str()));
      if(pSet->number != 1) {
          throw std::runtime_error("Too many results from path: " + path);
      }
      auto& bundle = mreg->getBundle(Model::OPENCONFIG_0_1_6);

      const shared_ptr<T>& ydkModel = make_shared<T>();
      const string &json = toJson(pSet->set.d[0]);
      MLOG(MINFO) << "json: " << json;
      return bundle.decode(json, ydkModel);
  }

  void delete2(string path);
  void createLeafs(shared_ptr<Entity> entity, string init, LeafVector& leafs);
  void write(string path, shared_ptr<Entity> entity);
  void create(shared_ptr<Entity> entity);

  void commit();
};
} // namespace cli
} // namespace channels
} // namespace devmand
