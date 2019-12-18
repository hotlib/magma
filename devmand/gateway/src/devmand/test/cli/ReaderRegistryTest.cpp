// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#define LOG_WITH_GLOG
#include <magma_logging.h>

#include <devmand/devices/cli/ModelRegistry.h>
#include <devmand/devices/cli/ReaderRegistry.h>
#include <devmand/test/cli/utils/Log.h>
#include <gtest/gtest.h>
#include <ydk_openconfig/openconfig_interfaces.hpp>

namespace devmand {
namespace test {
namespace cli {

using namespace devmand::devices::cli;
using namespace ydk;
using namespace std;
using OpenconfigInterfaces = openconfig::openconfig_interfaces::Interfaces;

class ReaderRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    devmand::test::utils::log::initLog();
  }
};

class InterfacesReader : public Reader<OpenconfigInterfaces> {
  void read(shared_ptr<OpenconfigInterfaces> data) override {
    shared_ptr<OpenconfigInterfaces::Interface> ifc =
        make_shared<OpenconfigInterfaces::Interface>();
    ifc->name = "ifc1";
    data->interface.append(ifc);
  }
};

TEST_F(ReaderRegistryTest, read) {
  ModelRegistry mreg;
  Bundle& bundle = mreg.getBundle(Model::OPENCONFIG_0_1_6);

  Path path =
      Path::makePath(bundle, "openconfig-interfaces:interfaces/interface");
  //  path =
  //      Path::makePath(bundle,
  //      "openconfig-interfaces:interfaces/interface/config/type");

  //  std::cout << path;

  ReaderRegistry reg(bundle);
  reg.add<OpenconfigInterfaces>(path, make_shared<InterfacesReader>());

  reg.checkReaderConsistency();

  RootDataNode root = reg.readAll();

  string json = bundle.encodeRoot(root);
  cout << json << endl;
  const shared_ptr<RootDataNode> rootDataNode = bundle.decodeRoot(json);

  cout << rootDataNode << endl;
}

} // namespace cli
} // namespace test
} // namespace devmand
