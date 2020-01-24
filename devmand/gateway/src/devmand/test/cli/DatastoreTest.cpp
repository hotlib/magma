// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#define LOG_WITH_GLOG
#include <magma_logging.h>
#include <devmand/channels/cli/codecs/YdkDynamicCodec.h>
#include <devmand/channels/cli/datastore/BindingAwareDatastoreTransaction.h>
#include <devmand/channels/cli/datastore/Datastore.h>
#include <devmand/channels/cli/datastore/DatastoreDiff.h>
#include <devmand/channels/cli/datastore/DatastoreTransaction.h>
#include <devmand/devices/Datastore.h>
#include <devmand/devices/cli/schema/ModelRegistry.h>
#include <devmand/test/cli/utils/Log.h>
#include <devmand/test/cli/utils/SampleJsons.h>
#include <folly/json.h>
#include <gtest/gtest.h>

namespace devmand {
namespace test {
namespace cli {

using devmand::channels::cli::codecs::YdkDynamicCodec;
using devmand::channels::cli::datastore::Datastore;
using devmand::channels::cli::datastore::DatastoreDiff;
using devmand::test::utils::cli::interface02TopPath;
using devmand::test::utils::cli::newInterface;
using devmand::test::utils::cli::newInterfaceTopPath;
using devmand::test::utils::cli::openconfigInterfacesInterfaces;
using folly::parseJson;
using folly::toPrettyJson;
using std::runtime_error;
using std::to_string;
using std::unique_ptr;

class DatastoreTest : public ::testing::Test {
 protected:
  shared_ptr<ModelRegistry> mregsh;
  shared_ptr<YdkDynamicCodec> codec;
  void SetUp() override {
    // TODO move somewhere global
    // set extensions and user_types for non-YDK libyang
    //      setenv(
    //              "LLLIBYANG_EXTENSIONS_PLUGINS_DIR",
    //              LLLIBYANG_EXTENSIONS_PLUGINS_DIR,
    //              false);
    //      setenv(
    //              "LIBYANG_USER_TYPES_PLUGINS_DIR",
    //              LIBYANG_USER_TYPES_PLUGINS_DIR, false);
    devmand::test::utils::log::initLog();
    mregsh = make_shared<ModelRegistry>();
    codec = make_shared<YdkDynamicCodec>(mregsh);
  }
};

TEST_F(DatastoreTest, commitWorks) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write("", parseJson(openconfigInterfacesInterfaces));
  transaction->commit();
  transaction = datastore.newTx();
  dynamic data = transaction->read("/openconfig-interfaces:interfaces");
  // in-broadcast-pkts has values 2767640, 2767641, 2767642 in the given
  // interfaces
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(
        data["openconfig-interfaces:interfaces"]["interface"][i]["state"]
            ["counters"]["in-broadcast-pkts"],
        "276764" + to_string(i));
  }
}

TEST_F(DatastoreTest, twoTransactionsAtTheSameTimeNotPermitted) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  EXPECT_THROW(datastore.newTx(), runtime_error);
}

TEST_F(DatastoreTest, abortDisablesRunningTransaction) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->abort();
  EXPECT_THROW(transaction->read("whatever"), runtime_error);
  EXPECT_THROW(transaction->write("", dynamic::object()), runtime_error);
  EXPECT_THROW(transaction->merge("", dynamic::object()), runtime_error);
  EXPECT_THROW(transaction->abort(), runtime_error);
  EXPECT_THROW(transaction->delete_("whatever"), runtime_error);
  EXPECT_THROW(transaction->commit(), runtime_error);
  EXPECT_THROW(transaction->isValid(), runtime_error);
  EXPECT_THROW(transaction->diff(), runtime_error);
}

TEST_F(DatastoreTest, commitDisablesRunningTransaction) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write("", parseJson(openconfigInterfacesInterfaces));

  transaction->commit();
  EXPECT_THROW(transaction->read("whatever"), runtime_error);
  EXPECT_THROW(transaction->write("", dynamic::object()), runtime_error);
  EXPECT_THROW(transaction->merge("", dynamic::object()), runtime_error);
  EXPECT_THROW(transaction->abort(), runtime_error);
  EXPECT_THROW(transaction->delete_("whatever"), runtime_error);
  EXPECT_THROW(transaction->commit(), runtime_error);
  EXPECT_THROW(transaction->isValid(), runtime_error);
  EXPECT_THROW(transaction->diff(), runtime_error);
}

TEST_F(DatastoreTest, commitEndsRunningTransaction) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write("", parseJson(openconfigInterfacesInterfaces));

  transaction->commit();
  EXPECT_NO_THROW(datastore.newTx());
}

TEST_F(DatastoreTest, dontAllowEmptyCommit) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  EXPECT_THROW(transaction->commit(), runtime_error);
}

TEST_F(DatastoreTest, abortEndsRunningTransaction) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->abort();
  EXPECT_NO_THROW(datastore.newTx());
}

TEST_F(DatastoreTest, deleteSubtrees) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write("", parseJson(openconfigInterfacesInterfaces));
  const char* interface03 =
      "/openconfig-interfaces:interfaces/interface[name='0/3']";
  EXPECT_TRUE(transaction->read(interface03) != nullptr);
  transaction->delete_(interface03);
  EXPECT_TRUE(transaction->read(interface03) == nullptr);

  transaction->abort();
}

TEST_F(DatastoreTest, writeNewInterface) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write("", parseJson(openconfigInterfacesInterfaces));
  const char* interface85 =
      "/openconfig-interfaces:interfaces/interface[name='0/85']";

  transaction->write(interface85, parseJson(newInterface));

  dynamic data = transaction->read(
      "/openconfig-interfaces:interfaces/interface[name='0/85']");
  transaction->abort();
  EXPECT_EQ(
      "0/85", data["openconfig-interfaces:interface"][0]["name"].getString());
}

TEST_F(DatastoreTest, identifyInvalidTree) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write(Path(""), parseJson(openconfigInterfacesInterfaces));
  transaction->delete_(
      "/openconfig-interfaces:interfaces/interface[name='0/2']/config");

  EXPECT_FALSE(transaction->isValid());
  transaction->abort();
}

TEST_F(DatastoreTest, mergeChangesInterface) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write(Path(""), parseJson(openconfigInterfacesInterfaces));
  transaction->write(newInterfaceTopPath, parseJson(newInterface));

  dynamic state = transaction->read(newInterfaceTopPath + "/state");
  state["openconfig-interfaces:state"]["mtu"] = 1555;
  state["openconfig-interfaces:state"]["oper-status"] = "UP";
  transaction->merge(newInterfaceTopPath + "/state", state);

  state = transaction->read(newInterfaceTopPath + "/state");
  transaction->abort();
  EXPECT_EQ(state["openconfig-interfaces:state"]["mtu"], 1555);
  EXPECT_EQ(state["openconfig-interfaces:state"]["oper-status"], "UP");
}

TEST_F(DatastoreTest, mergeErasedValueOriginalValueUnchangedInterface) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write(Path(""), parseJson(openconfigInterfacesInterfaces));

  dynamic state = transaction->read(interface02TopPath + "/state");
  state["openconfig-interfaces:state"].erase("mtu");
  transaction->merge(interface02TopPath + "/state", state);
  state = transaction->read(interface02TopPath + "/state");
  EXPECT_EQ(state["openconfig-interfaces:state"]["mtu"], 1518);
}

TEST_F(DatastoreTest, changeLeaf) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write(Path(""), parseJson(openconfigInterfacesInterfaces));

  dynamic enabled = transaction->read(interface02TopPath + "/state/enabled");
  MLOG(MINFO) << toPrettyJson(enabled);
  enabled["openconfig-interfaces:enabled"] = false;
  transaction->merge(interface02TopPath + "/state/enabled", enabled);

  enabled = transaction->read(interface02TopPath + "/state/enabled");
  EXPECT_EQ(enabled["openconfig-interfaces:enabled"], false);
}

TEST_F(DatastoreTest, changeLeafDiff) {
  Datastore datastore(codec, Datastore::operational());
  unique_ptr<channels::cli::datastore::DatastoreTransaction> transaction =
      datastore.newTx();
  transaction->write(Path(""), parseJson(openconfigInterfacesInterfaces));

  transaction->commit();
  transaction = datastore.newTx();
  dynamic errors = transaction->read(interface02TopPath + "/state/counters");
  errors["openconfig-interfaces:counters"]["out-errors"] = "777";
  errors["openconfig-interfaces:counters"]["out-discards"] = "17";
  transaction->merge(interface02TopPath + "/state/counters", errors);
  const map<Path, DatastoreDiff>& map = transaction->diff();
  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(
      map.at("/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state/openconfig-interfaces:counters/openconfig-interfaces:out-discards")
          .after["openconfig-interfaces:out-discards"],
      "17");
  EXPECT_EQ(
      map.at("/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state/openconfig-interfaces:counters/openconfig-interfaces:out-errors")
          .after["openconfig-interfaces:out-errors"],
      "777");
  EXPECT_EQ(
      map.at("/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state/openconfig-interfaces:counters/openconfig-interfaces:out-discards")
          .before["openconfig-interfaces:out-discards"],
      "0");
  EXPECT_EQ(
      map.at("/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state/openconfig-interfaces:counters/openconfig-interfaces:out-errors")
          .before["openconfig-interfaces:out-errors"],
      "0");
}

} // namespace cli
} // namespace test
} // namespace devmand
