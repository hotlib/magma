// Copyright (c) 2020-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <ydk/types.hpp>
#include <libyang/libyang.h>

using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using ydk::Entity;
using LeafVector = std::vector<pair<string, string>>;

namespace devmand {
namespace channels {
namespace cli {

class DatastoreTransaction {
private:
    ly_ctx* ctx;
    lyd_node * root;
public:

    DatastoreTransaction();

    shared_ptr<Entity> read(string path);

void delete2(string path);
void createLeafs(shared_ptr<Entity> entity, string init, LeafVector & leafs);
void write(string path, shared_ptr<Entity> entity);
void create(shared_ptr<Entity> entity);

void commit();
};
} // namespace cli
} // namespace channels
} // namespace devmand
