// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#define LOG_WITH_GLOG
#include <magma_logging.h>

#include <devmand/devices/cli/translation/ReaderRegistry.h>
#include <devmand/test/cli/utils/Json.h>
#include <devmand/test/cli/utils/Log.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/json.h>
#include <gtest/gtest.h>

namespace devmand {
namespace test {
namespace cli {

using namespace devmand::devices::cli;
using namespace devmand::test::utils::json;
using namespace folly;

class ReaderRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    devmand::test::utils::log::initLog();
  }
};

TEST_F(ReaderRegistryTest, api) {
  auto executor = make_shared<CPUThreadPoolExecutor>(2);

  ReaderRegistryBuilder reg;
  typedef class : public Reader {
   public:
    Future<dynamic> read(const Path& path) const override {
      (void)path;
      return Future<dynamic>(dynamic::object());
    }
  } IfcsReader;

  reg.add("/openconfig-interfaces:interfaces", make_shared<IfcsReader>());
  reg.addList(
      "/openconfig-interfaces:interfaces/interface", [](const Path& path) {
        (void)path;
        return Future<vector<dynamic>>(vector<dynamic>{
            dynamic::object("id", "0"), dynamic::object("id", "1")});
      });
  reg.add(
      "/openconfig-interfaces:interfaces/interface/config",
      [executor](const Path& path) {
        // Simulate delay
        return via(executor.get(),
                   []() { this_thread::sleep_for(chrono::milliseconds(500)); })
            .thenValue([path](Unit) -> dynamic {
              return dynamic::object("enabled", "true")(
                  "description", path.getKeys()["id"]);
            });
      });
  reg.add(
      "/openconfig-interfaces:interfaces/interface/state",
      [executor](const Path& path) {
        (void)path;
        // Simulate delay
        return via(executor.get(),
                   []() { this_thread::sleep_for(chrono::milliseconds(500)); })
            .thenValue([path](Unit) { return dynamic::object(); });
      });
  reg.add("/openconfig-vlans:vlans", [](const Path& path) {
    (void)path;
    return Future<dynamic>(dynamic::object());
  });
  auto r = reg.build();

  MLOG(MDEBUG) << *r;
  ASSERT_EQ(
      sortJson("{\n"
               "  \"openconfig-interfaces:interfaces\": {\n"
               "    \"interface\": [\n"
               "      {\n"
               "        \"state\": {},\n"
               "        \"config\": {\n"
               "          \"enabled\": \"true\"\n"
               "        },\n"
               "        \"id\": \"0\"\n"
               "      },\n"
               "      {\n"
               "        \"state\": {},\n"
               "        \"config\": {\n"
               "          \"enabled\": \"true\"\n"
               "        },\n"
               "        \"id\": \"1\"\n"
               "      }\n"
               "    ]\n"
               "  },\n"
               "  \"openconfig-vlans:vlans\": {}\n"
               "}"),
      sortJson(toPrettyJson(r->readConfiguration("/").get())));
  ASSERT_EQ(
      sortJson("{\n"
               "  \"openconfig-interfaces:interfaces\": {\n"
               "    \"interface\": [\n"
               "      {\n"
               "        \"state\": {},\n"
               "        \"config\": {\n"
               "          \"enabled\": \"true\"\n"
               "        },\n"
               "        \"id\": \"0\"\n"
               "      },\n"
               "      {\n"
               "        \"state\": {},\n"
               "        \"config\": {\n"
               "          \"enabled\": \"true\"\n"
               "        },\n"
               "        \"id\": \"1\"\n"
               "      }\n"
               "    ]\n"
               "  }\n"
               "}"),
      sortJson(toPrettyJson(
          r->readConfiguration("/openconfig-interfaces:interfaces").get())));
  ASSERT_EQ(
      sortJson("{\n"
               "  \"interface\": [\n"
               "    {\n"
               "      \"state\": {},\n"
               "      \"config\": {\n"
               "        \"enabled\": \"true\"\n"
               "      },\n"
               "      \"id\": \"0\"\n"
               "    },\n"
               "    {\n"
               "      \"state\": {},\n"
               "      \"config\": {\n"
               "        \"enabled\": \"true\"\n"
               "      },\n"
               "      \"id\": \"1\"\n"
               "    }\n"
               "  ]\n"
               "}"),
      sortJson(toPrettyJson(
          r->readConfiguration("/openconfig-interfaces:interfaces/interface")
              .get())));
  ASSERT_EQ(
      sortJson("{\n"
               "  \"interface\": [\n"
               "    {\n"
               "      \"state\": {},\n"
               "      \"config\": {\n"
               "        \"enabled\": \"true\"\n"
               "      },\n"
               "      \"id\": \"1\"\n"
               "    }\n"
               "  ]\n"
               "}"),
      sortJson(toPrettyJson(
          r->readConfiguration(
               "/openconfig-interfaces:interfaces/interface[id=\"1\"]")
              .get())));
  ASSERT_TRUE(
      r->readConfiguration("/openconfig-interfaces:interfaces/NOTEXISTING/B")
          .hasException());

  // Let the executor finish
  via(executor.get(), []() {}).get();
  executor->join();
}

TEST_F(ReaderRegistryTest, readerError) {
  ReaderRegistryBuilder reg;

  reg.addList(
      "/openconfig-interfaces:interfaces/interface", [](const Path& path) {
        (void)path;
        return Future<vector<dynamic>>(vector<dynamic>{
            dynamic::object("id", "0"), dynamic::object("id", "1")});
      });
  reg.add(
      "/openconfig-interfaces:interfaces/interface/config",
      [](const Path& path) {
        return Future<dynamic>(Try<dynamic>(ReadException(path, "From test")));
      });
  reg.add(
      "/openconfig-interfaces:interfaces/interface/state",
      [](const Path& path) {
        (void)path;
        return Future<dynamic>(dynamic::object);
      });

  auto r = reg.build();
  MLOG(MDEBUG) << *r;

  ASSERT_TRUE(r->readConfiguration("/").hasException());
  ASSERT_TRUE(r->readConfiguration(
                   "/openconfig-interfaces:interfaces/interface[id=\"1\"]")
                  .hasException());
  ASSERT_FALSE(
      r->readConfiguration(
           "/openconfig-interfaces:interfaces/interface[id=\"1\"]/state")
          .hasException());
}

} // namespace cli
} // namespace test
} // namespace devmand
