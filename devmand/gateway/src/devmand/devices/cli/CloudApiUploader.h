#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/arena.h>
#include <grpc++/grpc++.h>
#include <grpc++/impl/codegen/rpc_method.h>
#include <devmand/channels/cli/plugin/protocpp/CloudApi.grpc.pb.h>
#include <devmand/channels/cli/plugin/protocpp/CloudApi.grpc.pb.h>
#include <devmand/channels/cli/plugin/protocpp/CloudApi.pb.h>
#include <thread>
#include <chrono>

namespace devmand {
namespace devices {
namespace cli {

using devmand::channels::cli::plugin::DataRequest;
using devmand::channels::cli::plugin::DataReceiver;
using grpc::ClientContext;
using std::unique_ptr;
using std::shared_ptr;
using std::string;
using grpc::Status;

class CloudApiUploader {
    ClientContext context;
    string endpoint;
    shared_ptr<grpc::Channel> grpcChannel;
    const unique_ptr<DataReceiver::Stub> stub;
public:
    CloudApiUploader(const string _endpoint) :
    endpoint(_endpoint),
    grpcChannel(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials())),
    stub(DataReceiver::NewStub(grpcChannel)){}

    void uploadData(string deviceName, string deviceData){
        DataRequest request;
        google::protobuf::Empty empty;
        request.set_devicename(deviceName);
        request.set_devicedata(deviceData);
        const Status &status = stub->sendData(&context, request, &empty);
        std::cout << "status: " << status.error_message() << " code: " << status.error_code() << std::endl;
    }
};

} // namespace cli
} // namespace devices
} // namespace devmand
