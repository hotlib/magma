// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <boost/algorithm/string/trim.hpp>
#include <devmand/Application.h>
#include <devmand/channels/cli/Cli.h>
#include <devmand/devices/State.h>
#include <devmand/devices/cli/ModelRegistry.h>
#include <devmand/test/cli/utils/Json.h>
#include <devmand/test/cli/utils/Log.h>
#include <folly/futures/Future.h>
#include <folly/json.h>
#include <gtest/gtest.h>
#include <libyang/libyang.h>
#include <libyang/tree_data.h>
#include <magma_logging.h>

namespace devmand {
namespace test {
namespace cli {

using namespace devmand::test::utils::json;
using namespace std;
using namespace devmand::devices::cli;


    static const char* if_data =
            "{\n"
            "\"openconfig-interfaces:interfaces\": {\n"
            "\"interface\": [\n"
            "{\n"
            "\"name\": \"0/11\",\n"
            "\"config\": {\n"
            "\"name\": \"0/11\",\n"
            "\"type\": \"iana-if-type:ethernetCsmacd\",\n"

            "\"enabled\": true\n"
            "}\n"
            "}\n"
            "]\n"
            "}\n"
            "}";



//static const char* if_data =
//    "{\n"
//    "\"openconfig-interfaces:interfaces\": {\n"
//    "\"interface\": [\n"
//    "{\n"
//    "\"name\": \"0/11\",\n"
//    "\"config\": {\n"
//    "\"name\": \"0/11\",\n"
//    "\"type\": \"iana-if-type:ethernetCsmacd\",\n"
//
//    "\"enabled\": true\n"
//    "},\n"
//    "\"state\": {\n"
//    "\"name\": \"0/11\",\n"
//    "\"admin-status\": \"UP\",\n"
//    "\"oper-status\": \"DOWN\",\n"
//    "\"type\": \"iana-if-type:ethernetCsmacd\"\n"
//
//    "}\n"
//    "}\n"
//    "]\n"
//    "}\n"
//    "}";

/*
static const char *if_data = "{\n"
                             " \"openconfig-interfaces:interfaces\": {\n"
                             "    \"interface\": [\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/2\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/2\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/2\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"description\": \"Some descr\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/2\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/3\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/3\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/3\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/3\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/4\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/4\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/4\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/4\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/5\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/5\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/5\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/5\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/6\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/6\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/6\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/6\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/7\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/7\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/7\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/7\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  0/8\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"0/8\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"0/8\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"0/8\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      },\n"
                             "      {\n"
                             "        \"config\": {\n"
                             "          \"description\": \"This is ifc  3/6\",\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1500,\n"
                             "          \"name\": \"3/6\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        },\n"
                             "        \"name\": \"3/6\",\n"
                             "        \"state\": {\n"
                             "          \"admin-status\": \"UP\",\n"
                             "          \"counters\": {\n"
                             "            \"in-broadcast-pkts\": 2767644,\n"
                             "            \"in-discards\": 0,\n"
                             "            \"in-errors\": 0,\n"
                             "            \"in-multicast-pkts\": 5769311,\n"
                             "            \"in-octets\": 427066814515,\n"
                             "            \"in-unicast-pkts\": 293117,\n"
                             "            \"out-broadcast-pkts\": 7628,\n"
                             "            \"out-discards\": 0,\n"
                             "            \"out-errors\": 0,\n"
                             "            \"out-multicast-pkts\": 325039,\n"
                             "            \"out-octets\": 72669652,\n"
                             "            \"out-unicast-pkts\": 125182\n"
                             "          },\n"
                             "          \"enabled\": true,\n"
                             "          \"mtu\": 1518,\n"
                             "          \"name\": \"3/6\",\n"
                             "          \"oper-status\": \"DOWN\",\n"
                             "          \"type\": \"iana-if-type:ethernetCsmacd\"\n"
                             "        }\n"
                             "      }\n"
                             "    ]\n"
                             "  }\n"
                             "}";



    static const char *if_data =
"{"
    "\"ietf-interfaces:interfaces\": {"
    "\"interface\": ["
    "{"
    "\"name\": \"iface1\","
    "\"description\": \"iface1 dsc\","
    "\"@type\": {"
    "\"yang:type_attr\":\"1\""
    "},"
    "\"enabled\": true,"
    "\"ietf-ip:ipv4\": {"
    "\"@\": {"
    "\"yang:ip_attr\":\"14\""
    "},"
    "\"enabled\": true,"
    "\"forwarding\": true,"
    "\"mtu\": 68,"
    "\"address\": ["
    "{"
    "\"ip\": \"10.0.0.1\""
    "},"
    "{"
    "\"ip\": \"172.0.0.1\","
    "\"prefix-length\": 16"
    "}"
    "],"
    "\"neighbor\": ["
    "{"
    "\"ip\": \"10.0.0.2\","
    "\"link-layer-address\": \"01:34:56:78:9a:bc:de:f0\""
    "}"
    "]"
    "},"
    "\"ietf-ip:ipv6\": {"
    "\"@\": {"
    "\"yang:ip_attr\":\"16\""
    "},"
    "\"enabled\": true,"
    "\"forwarding\": false,"
    "\"mtu\": 1280,"
    "\"address\": ["
    "{"
    "\"ip\": \"2001:abcd:ef01:2345:6789:0:1:1\","
    "\"prefix-length\": 64"
    "}"
    "],"
    "\"neighbor\": ["
    "{"
    "\"ip\": \"2001:abcd:ef01:2345:6789:0:1:2\","
    "\"link-layer-address\": \"01:34:56:78:9a:bc:de:f0\""
    "}"
    "],"
    "\"dup-addr-detect-transmits\": 52,"
    "\"autoconf\": {"
    "\"create-global-addresses\": true"
    "}"
    "}"
    "},"
    "{"
    "\"name\": \"iface2\","
    "\"description\": \"iface2 dsc\","
    "\"type\": \"iana-if-type:softwareLoopback\","
    "\"@type\": {"
    "\"yang:type_attr\":\"2\""
    "},"
    "\"enabled\": false,"
    "\"ietf-ip:ipv4\": {"
    "\"@\": {"
    "\"yang:ip_attr\":\"24\""
    "},"
    "\"address\": ["
    "{"
    "\"ip\": \"10.0.0.5\""
    "},"
    "{"
    "\"ip\": \"172.0.0.5\","
    "\"prefix-length\": 16"
    "}"
    "],"
    "\"neighbor\": ["
    "{"
    "\"ip\": \"10.0.0.1\","
    "\"link-layer-address\": \"01:34:56:78:9a:bc:de:fa\""
    "}"
    "]"
    "},"
    "\"ietf-ip:ipv6\": {"
    "\"@\": {"
    "\"yang:ip_attr\":\"26\""
    "},"
    "\"address\": ["
    "{"
    "\"ip\": \"2001:abcd:ef01:2345:6789:0:1:5\","
    "\"prefix-length\": 64"
    "}"
    "],"
    "\"neighbor\": ["
    "{"
    "\"ip\": \"2001:abcd:ef01:2345:6789:0:1:1\","
    "\"link-layer-address\": \"01:34:56:78:9a:bc:de:fa\""
    "}"
    "],"
    "\"dup-addr-detect-transmits\": 100,"
    "\"autoconf\": {"
    "\"create-global-addresses\": true"
    "}"
    "}"
    "}"
    "]"
    "}"
    "}";
*/



class YangYdkParsingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    devmand::test::utils::log::initLog();
  }
  ModelRegistry mreg;
};

TEST_F(YangYdkParsingTest, parsetest) {
  ly_ctx* ctx = ly_ctx_new("/usr/share/openconfig@0.1.6/", 0);

//  ly_ctx_load_module(ctx, "iana-crypt-hash", NULL);
  ly_ctx_load_module(ctx, "iana-if-type", NULL);
//  ly_ctx_load_module(ctx, "ietf-diffserv-action", NULL);
//  ly_ctx_load_module(ctx, "ietf-diffserv-classifier", NULL);
//  ly_ctx_load_module(ctx, "ietf-diffserv-policy", NULL);
//  ly_ctx_load_module(ctx, "ietf-diffserv-target", NULL);
//  ly_ctx_load_module(ctx, "ietf-event-notifications", NULL);
//  ly_ctx_load_module(ctx, "ietf-inet-types", NULL);
//  ly_ctx_load_module(ctx, "ietf-interfaces-ext", NULL);
 // ly_ctx_load_module(ctx, "ietf-interfaces", NULL);
//  ly_ctx_load_module(ctx, "ietf-ip", NULL);
//  ly_ctx_load_module(ctx, "ietf-ipv4-unicast-routing", NULL);
//  ly_ctx_load_module(ctx, "ietf-ipv6-unicast-routing", NULL);
//  ly_ctx_load_module(ctx, "ietf-key-chain", NULL);
//  ly_ctx_load_module(ctx, "ietf-netconf-acm", NULL);
//  ly_ctx_load_module(ctx, "ietf-netconf-monitoring", NULL);
//  ly_ctx_load_module(ctx, "ietf-netconf-notifications", NULL);
//  ly_ctx_load_module(ctx, "ietf-netconf-with-defaults", NULL);
//  ly_ctx_load_module(ctx, "ietf-netconf", NULL);
//  ly_ctx_load_module(ctx, "ietf-ospf", NULL);
//  ly_ctx_load_module(ctx, "ietf-restconf-monitoring", NULL);
//  ly_ctx_load_module(ctx, "ietf-routing", NULL);
//  ly_ctx_load_module(ctx, "ietf-syslog-types", NULL);
//  ly_ctx_load_module(ctx, "ietf-yang-library", NULL);
//  ly_ctx_load_module(ctx, "ietf-yang-push", NULL);
//  ly_ctx_load_module(ctx, "ietf-yang-smiv2", NULL);
//  ly_ctx_load_module(ctx, "ietf-yang-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-acl", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-common", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-ethernet", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-ipv4", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-ipv6", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-mpls", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-network-instance", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-pf", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-aft", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-common-multiprotocol", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-common-structure", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-common", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-global", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-neighbor", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-peer-group", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-policy", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-bgp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-channel-monitor", NULL);
//  ly_ctx_load_module(ctx, "openconfig-extensions", NULL);
//  ly_ctx_load_module(ctx, "openconfig-if-aggregate", NULL);
//  ly_ctx_load_module(ctx, "openconfig-if-ethernet", NULL);
//  ly_ctx_load_module(ctx, "openconfig-if-ip", NULL);
//  ly_ctx_load_module(ctx, "openconfig-inet-types", NULL);
  ly_ctx_load_module(ctx, "openconfig-interfaces", NULL);
//  ly_ctx_load_module(ctx, "openconfig-isis-lsdb-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-isis-lsp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-isis-policy", NULL);
//  ly_ctx_load_module(ctx, "openconfig-isis-routing", NULL);
//  ly_ctx_load_module(ctx, "openconfig-isis-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-isis", NULL);
//  ly_ctx_load_module(ctx, "openconfig-lacp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-lldp-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-lldp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-local-routing", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-igp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-ldp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-rsvp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-sr", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-static", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-te", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-mpls", NULL);
//  ly_ctx_load_module(ctx, "openconfig-network-instance-l2", NULL);
//  ly_ctx_load_module(ctx, "openconfig-network-instance-l3", NULL);
//  ly_ctx_load_module(ctx, "openconfig-network-instance-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-network-instance", NULL);
//  ly_ctx_load_module(ctx, "openconfig-optical-amplifier", NULL);
//  ly_ctx_load_module(ctx, "openconfig-packet-match-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-packet-match", NULL);
//  ly_ctx_load_module(ctx, "openconfig-platform-transceiver", NULL);
//  ly_ctx_load_module(ctx, "openconfig-platform-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-platform", NULL);
//  ly_ctx_load_module(ctx, "openconfig-policy-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-rib-bgp-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-rib-bgp", NULL);
//  ly_ctx_load_module(ctx, "openconfig-routing-policy", NULL);
//  ly_ctx_load_module(ctx, "openconfig-rsvp-sr-ext", NULL);
//  ly_ctx_load_module(ctx, "openconfig-segment-routing", NULL);
//  ly_ctx_load_module(ctx, "openconfig-telemetry", NULL);
//  ly_ctx_load_module(ctx, "openconfig-terminal-device", NULL);
//  ly_ctx_load_module(ctx, "openconfig-transport-line-common", NULL);
//  ly_ctx_load_module(ctx, "openconfig-transport-line-protection", NULL);
//  ly_ctx_load_module(ctx, "openconfig-transport-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-vlan-types", NULL);
//  ly_ctx_load_module(ctx, "openconfig-vlan", NULL);
//  ly_ctx_load_module(ctx, "openconfig-yang-types", NULL);
//  ly_ctx_load_module(ctx, "policy-types", NULL);

  // MLOG(MDEBUG) << if_data;
  //  (void)pModule;

  lyd_node* pNode = lyd_parse_mem(ctx, if_data, LYD_JSON, LYD_OPT_CONFIG);

  if (pNode == NULL) {
    MLOG(MDEBUG) << "pNode is NULL";
  }

  return;
}

} // namespace cli
} // namespace test
} // namespace devmand