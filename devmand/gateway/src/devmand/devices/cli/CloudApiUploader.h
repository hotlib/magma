#pragma once

namespace devmand {
namespace devices {
namespace cli {

#include <devmand/channels/cli/plugin/protocpp/CloudApi.grpc.pb.h>
#include <devmand/channels/cli/plugin/protocpp/CloudApi.pb.h>

using devmand::channels::i::cloudapi::DataRequest;
using devmand::channels::cli::cloudapi::DataReceiver;

class CloudApiUploader {
    unique_ptr<DataReceiver> stub_;
    DataRequest request;

    void sentDummyData(){
        request->set_devicename("device name 123");
        request->set_devicedata("device data 123");
 //       stub_->sendData();
//        stub_->set_devicedata("device data 123");
    }
}

} // namespace cli
} // namespace devices
} // namespace devmand
