// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#define LOG_WITH_GLOG
#include <magma_logging.h>

#include <devmand/devices/cli/translation/Path.h>
#include <devmand/test/cli/utils/Log.h>
#include <gtest/gtest.h>

namespace devmand {
namespace test {
namespace cli {

using namespace devmand::devices::cli;
using namespace folly;

class PathTest : public ::testing::Test {
 protected:
  void SetUp() override {
    devmand::test::utils::log::initLog();
  }
};

TEST_F(PathTest, path) {
  Path root = "/";
  ASSERT_EQ(root.getDepth(), 0);
  ASSERT_EQ(root.getSegments(), vector<string>());

  Path ifcs = "/openconfig-interfaces:interfaces";
  ASSERT_EQ(ifcs.getDepth(), 1);
  ASSERT_TRUE(ifcs.isChildOf(root));
  ASSERT_FALSE(ifcs.isChildOf("/something-else"));
  ASSERT_FALSE(root.isChildOf(ifcs));
  ASSERT_EQ(
      ifcs.getSegments(), vector<string>{"openconfig-interfaces:interfaces"});
  ASSERT_EQ(ifcs.getLastSegment(), "openconfig-interfaces:interfaces");
  ASSERT_EQ(ifcs.getParent(), root);

  Path ifc =
      "/openconfig-interfaces:interfaces/interface[id=\"ethernet 0/1\"]/config";
  ASSERT_EQ(ifc.getDepth(), 3);
  ASSERT_TRUE(ifc.isChildOf(root));
  ASSERT_TRUE(ifc.isChildOf(ifcs));
  ASSERT_EQ(
      ifc.unkeyed(), "/openconfig-interfaces:interfaces/interface/config");
  ASSERT_EQ(
      ifc.getParent(),
      "/openconfig-interfaces:interfaces/interface[id=\"ethernet 0/1\"]");
  vector<string> expectedSegments =
      vector<string>{"openconfig-interfaces:interfaces",
                     "interface[id=\"ethernet 0/1\"]",
                     "config"};
  ASSERT_EQ(ifc.getSegments(), expectedSegments);

  EXPECT_THROW(Path("openconfig-interfaces:interfaces"), InvalidPathException);

  //   Invalid paths
  //  Path ifcs = " /openconfig-interfaces:interfaces  ";
  //  Path ifcs = "/openconfig-interfaces:interfaces/";
  //  Path ifcs = "/xyz-sad-asdf";
}

} // namespace cli
} // namespace test
} // namespace devmand
