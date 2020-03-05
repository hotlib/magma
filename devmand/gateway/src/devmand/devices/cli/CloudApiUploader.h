#pragma once

namespace devmand {
namespace devices {
namespace cli {

#include <devmand/channels/cli/plugin/protocpp/CloudApi.grpc.pb.h>
#include <devmand/channels/cli/plugin/protocpp/CloudApi.pb.h>
#include <thread>
#include <chrono>

            using devmand::channels::cli::plugin::DataRequest;
using devmand::channels::cli::plugin::DataReceiver;
using grpc::ClientContext;

class CloudApiUploader {
    //unique_ptr<DataReceiver> stub_2;
public:
    
    void sentDummyData(){
        ClientContext context;
        string endpoint = "localhost:50051";
        shared_ptr<grpc::Channel> grpcChannel =
                grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
        const unique_ptr<DataReceiver::Stub> &ptr = DataReceiver::NewStub(grpcChannel);
        //unique_ptr<DataReceiver> stub_(ptr);
        DataRequest request;
        google::protobuf::Empty empty;
        request.set_devicename("device name 123");
        request.set_devicedata("device data 123");
        const Status &status = ptr->sendData(&context, request, &empty);
        //std::this_thread::sleep_for (std::chrono::seconds(2));
        std::cout << "status: " << status.error_message() << " code: " << status.error_code() << std::endl;
 //       stub_->sendData();
//        stub_->set_devicedata("device data 123");
    }
};

} // namespace cli
} // namespace devices
} // namespace devmand
