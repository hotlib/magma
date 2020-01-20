// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#define LOG_WITH_GLOG
#include <magma_logging.h>

#include <devmand/devices/cli/translation/Path.h>
#include <devmand/channels/cli/SchemaContext.h>
#include <devmand/test/cli/utils/Log.h>
#include <folly/json.h>
#include <gtest/gtest.h>

namespace devmand::test::cli {

using namespace devmand::devices::cli;
using devmand::channels::cli::SchemaContext;
using namespace folly;
using std::string;
class SchemaContextTest : public ::testing::Test {
 protected:
  void SetUp() override {
    devmand::test::utils::log::initLog();
  }
};

    TEST_F(SchemaContextTest, ctxtest) {
        string sklucmi = "/openconfig-interfaces:interfaces/interface[name=\"GigabitEthernet1\"]/subinterfaces/subinterface[index=0]/openconfig-if-ip:ipv4/addresses/address[ip=\"44.44.44.44\"]";
        string bezklucmi = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/openconfig-if-ip:ipv4/addresses/address";
        string simple = "/openconfig-interfaces:interfaces/openconfig-interfaces:interface";
        SchemaContext context;
        context.getNode(Path(simple));
//        const vector <string> &vector = context.getKeys(Path(simple));
//
//        for (const auto &key : vector) {
//            MLOG(MINFO) << key;
//
//}
    }

}
