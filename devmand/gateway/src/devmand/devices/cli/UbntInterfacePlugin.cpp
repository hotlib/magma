// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "UbntInterfacePlugin.h"
#include <devmand/channels/cli/Cli.h>
#include <devmand/channels/cli/IoConfigurationBuilder.h>
#include <devmand/devices/Datastore.h>
#include <devmand/devices/cli/ParsingUtils.h>
#include <devmand/devices/cli/StructuredUbntDevice2.h>
#include <devmand/devices/cli/schema/ModelRegistry.h>
#include <devmand/devices/cli/translation/BindingReaderRegistry.h>
#include <devmand/devices/cli/translation/PluginRegistry.h>
#include <devmand/devices/cli/translation/ReaderRegistry.h>
#include <folly/futures/Future.h>
#include <folly/json.h>
#include <ydk_ietf/iana_if_type.hpp>
#include <ydk_openconfig/openconfig_interfaces.hpp>
#include <ydk_openconfig/openconfig_network_instance.hpp>
#include <ydk_openconfig/openconfig_vlan_types.hpp>
#include <memory>
#include <regex>
#include <unordered_map>
#include "StructuredUbntDevice2.h"

namespace devmand {
namespace devices {
namespace cli {

using namespace devmand::channels::cli;
using namespace std;
using namespace folly;
using namespace ydk;

DeviceType UbntInterfacePlugin::getDeviceType() const {
  return {"ubnt", "*"};
}

static const regex mtuRegx = regex(R"(mtu (.+))");
static const regex descrRegx = regex(R"(description '?(.+?)'?)");
static const regex shutRegx = regex("shutdown");
static const regex typeRegx = regex(R"(interface\s+(.+))");
static const regex ethernetIfcRegx = regex(R"(\d+/\d+)");

static const string parseIfcType(string ifcName) {
  if (ifcName.find("lag") == 0) {
    return "iana-if-type:Ieee8023adLag";
  } else if (ifcName.find("vlan") == 0) {
    return "iana-if-type:L3ipvlan";
  } else if (regex_match(ifcName, ethernetIfcRegx)) {
    return "iana-if-type:ethernetCsmacd";
  }
  return "iana-if-type:Other";
}

class IfcConfigReader : public Reader {
 public:
  Future<dynamic> read(const Path& path, const DeviceAccess& device)
      const override {
    string ifcName = path.getKeysFromSegment("interface")["name"].getString();

    return device.cli()
        ->executeRead(
            ReadCommand::create("show running-config interface " + ifcName))
        .via(folly::getCPUExecutor().get())
        .thenValue([ifcName](auto out) {
          dynamic config = dynamic::object;
          config["name"] = ifcName;
          parseValue(out, mtuRegx, 1, [&config](auto mtuAsString) {
            config["mtu"] = toUI16(mtuAsString);
          });
          parseValue(out, descrRegx, 1, [&config](auto descrAsString) {
            config["description"] = descrAsString;
          });
          config["enabled"] = true;
          parseValue(out, shutRegx, 1, [&config](auto shutdownAsString) {
            config["enabled"] = "shutdown" != shutdownAsString;
          });
          parseValue(out, typeRegx, 1, [&config](auto typeAsString) {
            config["type"] = parseIfcType(typeAsString);
          });
          return config;
        });
  }
};

static const auto mtuState = regex(R"(Max Frame Size.*?(\d+).*)");

class IfcStateReader : public BindingReader {
  using Ifc = openconfig::openconfig_interfaces::Interfaces::Interface;
  using IfcType = ietf::iana_if_type::IanaInterfaceType;
  using AdminState = Ifc::State::AdminStatus;
  using OperState = Ifc::State::OperStatus;

 public:
  Future<shared_ptr<Entity>> read(const Path& path, const DeviceAccess& device)
      const override {
    string ifcName = path.getKeysFromSegment("interface")["name"].getString();

    vector<SemiFuture<string>> cmdFutures;
    cmdFutures.emplace_back(device.cli()->executeRead(
        ReadCommand::create("show running-config interface " + ifcName)));
    cmdFutures.emplace_back(device.cli()->executeRead(
        ReadCommand::create("show interfaces description")));

    return collect(cmdFutures.begin(), cmdFutures.end())
        .thenValue([ifcName](auto cmdOutputs) {
          string output = cmdOutputs[0];
          string stateOut = cmdOutputs[1];

          auto state = make_shared<Ifc::State>();
          state->name = ifcName;

          state->enabled = true;
          parseLeaf<bool>(output, shutRegx, state->enabled, 0, [](auto str) {
            return "shutdown" != str;
          });

          parseLeaf<IfcType>(
              output,
              typeRegx,
              state->type,
              1,
              [&ifcName](auto str) -> IfcType {
                if (str.find("lag") == 0) {
                  return ietf::iana_if_type::Ieee8023adLag();
                } else if (str.find("vlan") == 0) {
                  return ietf::iana_if_type::L3ipvlan();
                } else if (regex_match(ifcName, ethernetIfcRegx)) {
                  return ietf::iana_if_type::EthernetCsmacd();
                }
                return ietf::iana_if_type::Other();
              });

          auto regexIfcState = regex(ifcName + R"(\s+(\S+)\s+(\S+)\s*(.*))");
          parseLeaf<string>(stateOut, regexIfcState, state->description, 3);

          parseValue(stateOut, regexIfcState, 1, [&state](string value) {
            if ("Enable" == value) {
              state->admin_status = AdminState::UP;
            } else if ("Disable" == value) {
              state->admin_status = AdminState::DOWN;
            }
          });

          parseValue(stateOut, regexIfcState, 2, [&state](string value) {
            if ("Up" == value) {
              state->oper_status = OperState::UP;
            } else if ("Down" == value) {
              state->oper_status = OperState::DOWN;
            } else {
              state->oper_status = OperState::UNKNOWN;
            }
          });
          return state;
        });
  }
};

static const auto inOct =
    regex(R"(Total Packets Received \(Octets\).*?(\d+).*)");
static const auto inUPkt = regex(R"(Unicast Packets Received.*?(\d+).*)");
static const auto inMPkt = regex(R"(Multicast Packets Received.*?(\d+).*)");
static const auto inBPkt = regex(R"(Broadcast Packets Received.*?(\d+).*)");
static const auto inDisc = regex(R"(Receive Packets Discarded.*?(\d+).*)");
static const auto inErrors =
    regex(R"(Total Packets Received with MAC Errors.*?(\d+).*)");

static const auto outOct =
    regex(R"(Total Packets Transmitted \(Octets\).*?(\d+).*)");
static const auto outUPkt = regex(R"(Unicast Packets Transmitted.*?(\d+).*)");
static const auto outMPkt = regex(R"(Multicast Packets Transmitted.*?(\d+).*)");
static const auto outBPkt = regex(R"(Broadcast Packets Transmitted.*?(\d+).*)");
static const auto outDisc = regex(R"(Transmit Packets Discarded.*?(\d+).*)");
static const auto outErrors = regex(R"(Total Transmit Errors.*?(\d+).*)");

static const auto counterReset =
    regex(R"(Time Since Counters Last Cleared.*?([^.]+).*)");

class IfcCountersReader : public BindingReader {
  using Ifc = openconfig::openconfig_interfaces::Interfaces::Interface;

 public:
  Future<shared_ptr<Entity>> read(const Path& path, const DeviceAccess& device)
      const override {
    string ifcName = path.getKeysFromSegment("interface")["name"].getString();

    // If is not ethernet interface, do not read anything
    // FIXME, this needs to be checked on device
    if (!regex_match(ifcName, ethernetIfcRegx)) {
      return Future<shared_ptr<Entity>>(make_shared<Ifc::State::Counters>());
    }

    return device.cli()
        ->executeRead(ReadCommand::create("show interface ethernet " + ifcName))
        .via(folly::getCPUExecutor().get())
        .thenValue([ifcName](auto output) {
          auto ctr = make_shared<Ifc::State::Counters>();

          parseLeaf(output, inOct, ctr->in_octets, 1, toUI64);
          parseLeaf(output, inUPkt, ctr->in_unicast_pkts, 1, toUI64);
          parseLeaf(output, inMPkt, ctr->in_multicast_pkts, 1, toUI64);
          parseLeaf(output, inBPkt, ctr->in_broadcast_pkts, 1, toUI64);
          parseLeaf(output, inDisc, ctr->in_discards, 1, toUI64);
          parseLeaf(output, inErrors, ctr->in_errors, 1, toUI64);

          parseLeaf(output, outOct, ctr->out_octets, 1, toUI64);
          parseLeaf(output, outUPkt, ctr->out_unicast_pkts, 1, toUI64);
          parseLeaf(output, outMPkt, ctr->out_multicast_pkts, 1, toUI64);
          parseLeaf(output, outBPkt, ctr->out_broadcast_pkts, 1, toUI64);
          parseLeaf(output, outDisc, ctr->out_discards, 1, toUI64);
          parseLeaf(output, outErrors, ctr->out_errors, 1, toUI64);

          parseLeaf<string>(output, counterReset, ctr->last_clear, 1);

          return ctr;
        });
  }
};

void cli::UbntInterfacePlugin::provideReaders(
    ReaderRegistryBuilder& reg) const {
  // Reader is a DOM lambda
  reg.addList(
      "/openconfig-interfaces:interfaces/interface",
      [](const Path& path, const DeviceAccess& device) {
        (void)path;
        return device.cli()
            ->executeRead(ReadCommand::create("show interfaces description"))
            .via(folly::getCPUExecutor().get())
            .thenValue([](auto output) -> Future<vector<dynamic>> {
              return parseKeys<dynamic>(
                  output,
                  regex(R"(^(\S+)\s+(\S+)\s+(\S+)\s*(.*))"),
                  1,
                  4,
                  [](auto ifcName) {
                    return dynamic::object("name", ifcName);
                  });
            });
      });

  // Reader is a DOM class
  reg.add(
      "/openconfig-interfaces:interfaces/interface/config",
      make_shared<IfcConfigReader>());

  // Reader is a Binding-aware class
  BINDING(reg, openconfigContext)
      .add(
          "/openconfig-interfaces:interfaces/interface/state",
          make_shared<IfcStateReader>());

  // Reader is a Binding-aware class
  BINDING(reg, openconfigContext)
      .add(
          "/openconfig-interfaces:interfaces/interface/state/counters",
          make_shared<IfcCountersReader>());
}

void cli::UbntInterfacePlugin::provideWriters() const {
  // no writers yet
}

UbntInterfacePlugin::UbntInterfacePlugin(BindingContext& _openconfigContext)
    : openconfigContext(_openconfigContext) {}

} // namespace cli
} // namespace devices
} // namespace devmand
