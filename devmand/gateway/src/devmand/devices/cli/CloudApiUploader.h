#pragma once

#define LOG_WITH_GLOG

#include <magma_logging.h>

#include <devmand/channels/cli/plugin/protocpp/CloudApi.grpc.pb.h>
#include <devmand/channels/cli/plugin/protocpp/CloudApi.pb.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/service.h>
#include <grpc++/grpc++.h>
#include <grpc++/impl/codegen/rpc_method.h>
#include <chrono>
#include <thread>

namespace devmand {
namespace devices {
namespace cli {

using devmand::channels::cli::plugin::DataReceiver;
using devmand::channels::cli::plugin::DataRequest;
using grpc::ClientContext;
using grpc::Status;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

class CloudApiUploader {
  string endpoint;
  shared_ptr<grpc::Channel> grpcChannel;
  const unique_ptr<DataReceiver::Stub> stub;
  std::mutex mut;

 public:
  CloudApiUploader(const string _endpoint)
      : endpoint(_endpoint),
        grpcChannel(
            grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials())),
        stub(DataReceiver::NewStub(grpcChannel)) {}

  void uploadData(string deviceName, string deviceData) {
    mut.lock();
    ClientContext context;
    DataRequest request;
    google::protobuf::Empty empty;
    request.set_devicename(deviceName);
    request.set_devicedata(deviceData);
    const Status& status = stub->sendData(&context, request, &empty);
    if (not status.ok()) {
      MLOG(MERROR) << "Unable to connect to cloud API, status: "
                   << status.error_message()
                   << " code: " << status.error_code();
    }
    mut.unlock();
  }
};

} // namespace cli
} // namespace devices
} // namespace devmand
