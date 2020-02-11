// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once
#include <devmand/devices/cli/schema/Path.h>

namespace devmand::test::utils::cli {
using devmand::devices::cli::Path;

string statePathWithKey = "/openconfig-interfaces:interfaces/openconfig-interfaces:interface[name='0/2']/openconfig-interfaces:state";
string statePath = "/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state";
string counterPath = statePath + "/counters";

string openconfigInterfacesInterfaces =
    "{\n"
    "  \"openconfig-interfaces:interfaces\": {\n"
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
    "            \"in-broadcast-pkts\": 2767640,\n"
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
    "            \"in-broadcast-pkts\": 2767641,\n"
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
    "            \"in-broadcast-pkts\": 2767642,\n"
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
    "      }\n"
    "    ]\n"
    "  }\n"
    "}";

Path newInterfaceTopPath(
    "/openconfig-interfaces:interfaces/interface[name='0/85']");

Path interface02TopPath(
    "/openconfig-interfaces:interfaces/interface[name='0/2']");

string operStatus =
    "/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state/openconfig-interfaces:oper-status";

string newInterface =
    "{\n"
    "  \"openconfig-interfaces:interface\": [\n"
    "      {\n"
    "        \"name\": \"0/85\",\n"
    "        \"state\": {\n"
    "          \"type\": \"iana-if-type:ethernetCsmacd\",\n"
    "          \"oper-status\": \"DOWN\",\n"
    "          \"name\": \"0/85\",\n"
    "          \"mtu\": 1518,\n"
    "          \"enabled\": true,\n"
    "          \"description\": \"Some descr\",\n"
    "          \"counters\": {\n"
    "            \"out-unicast-pkts\": \"125182\",\n"
    "            \"out-octets\": \"72669652\",\n"
    "            \"out-multicast-pkts\": \"325039\",\n"
    "            \"out-errors\": \"0\",\n"
    "            \"out-discards\": \"0\",\n"
    "            \"out-broadcast-pkts\": \"7628\",\n"
    "            \"in-unicast-pkts\": \"293117\",\n"
    "            \"in-octets\": \"427066814515\",\n"
    "            \"in-multicast-pkts\": \"5769311\",\n"
    "            \"in-errors\": \"0\",\n"
    "            \"in-discards\": \"0\",\n"
    "            \"in-broadcast-pkts\": \"2767640\"\n"
    "          },\n"
    "          \"admin-status\": \"UP\"\n"
    "        },\n"
    "        \"config\": {\n"
    "          \"type\": \"iana-if-type:ethernetCsmacd\",\n"
    "          \"name\": \"0/85\",\n"
    "          \"mtu\": 1500,\n"
    "          \"enabled\": true,\n"
    "          \"description\": \"This is ifc  0/85\"\n"
    "        },\n"
    "        \"hold-time\": {\n"
    "          \"state\": {\n"
    "            \"up\": 0,\n"
    "            \"down\": 0\n"
    "          }\n"
    "        }\n"
    "      }\n"
    "    ]\n"
    "}";

string interfaceCounters =
    "/openconfig-interfaces:interfaces/openconfig-interfaces:interface/openconfig-interfaces:state/openconfig-interfaces:counters";

string interfaceCountersWithKey =
    "/openconfig-interfaces:interfaces/openconfig-interfaces:interface[name='0/2']/openconfig-interfaces:state/openconfig-interfaces:counters";

string interface02state =
    "{\n"
    "  \"openconfig-interfaces:state\": {\n"
    "    \"admin-status\": \"UP\",\n"
    "    \"counters\": {\n"
    "      \"in-broadcast-pkts\": \"2767640\",\n"
    "      \"in-discards\": \"0\",\n"
    "      \"in-errors\": \"0\",\n"
    "      \"in-multicast-pkts\": \"5769311\",\n"
    "      \"in-octets\": \"427066814515\",\n"
    "      \"in-unicast-pkts\": \"293117\",\n"
    "      \"out-broadcast-pkts\": \"7628\",\n"
    "      \"out-discards\": \"0\",\n"
    "      \"out-errors\": \"0\",\n"
    "      \"out-multicast-pkts\": \"325039\",\n"
    "      \"out-octets\": \"72669652\",\n"
    "      \"out-unicast-pkts\": \"125182\"\n"
    "    },\n"
    "    \"description\": \"Some descr\",\n"
    "    \"enabled\": true,\n"
    "    \"mtu\": 1518,\n"
    "    \"name\": \"0/2\",\n"
    "    \"oper-status\": \"DOWN\",\n"
    "    \"type\": \"iana-if-type:ethernetCsmacd\"\n"
    "  }\n"
    "}";

} // namespace devmand::test::utils::cli
