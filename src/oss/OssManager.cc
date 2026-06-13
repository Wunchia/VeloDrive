#include <alibabacloud/oss/OssClient.h>
#include <alibabacloud/oss/client/ClientConfiguration.h>
#include <alibabacloud/oss/model/GetObjectRequest.h>
#include <alibabacloud/oss/model/GetObjectResult.h>
#include <alibabacloud/oss/model/PutObjectRequest.h>
#include <iostream>
#include <memory>
#include <sstream>
#include "OssManager.h"

using namespace AlibabaCloud::OSS;

OssManager& OssManager::getInstance(){
    static OssManager instance;
    return instance;
}

bool OssManager::init(const std::string& endpoint,
          const std::string& accessKeyId,
          const std::string& accessKeySecret,
          const std::string& bucketName,
          const std::string& region)
{
    // 1.初始化网络等资源
    InitializeSdk();

    // 2.设置OSS账号信息，创建OSSClient
    ClientConfiguration conf;
    _client=std::make_unique<OssClient>(endpoint,accessKeyId,accessKeySecret,conf);
    _client->SetRegion(region);

    // 3.设置bucket名称
    _bucketName=bucketName;
    _initialized=true;
    return _initialized;
}

bool OssManager::putObject(const std::string& objectName,const std::string& content){
    // 3.上传文件
    std::shared_ptr<std::iostream>stream=std::make_shared<std::stringstream>(std::move(content));
    PutObjectRequest request(_bucketName,objectName,stream);
    auto outcome=_client->PutObject(request);

    // 4.错误处理
    if(outcome.isSuccess()){
        std::cout << "[PutObject Success]"<<std::endl;
    }else{
        std::cerr<<"[PutObject Failed]:"<<" code:"<<outcome.error().Code()
                 <<", message:"<<outcome.error().Message()
                 <<", requestId:"<<outcome.error().RequestId()<<std::endl;
    }
    return outcome.isSuccess();
}

std::string OssManager::getObject(const std::string& objectName){
    GetObjectRequest request(_bucketName, objectName);
      auto outcome = _client->GetObject(request);
      if (outcome.isSuccess()) {
        auto content=outcome.result().Content();
        std::ostringstream oss;
        oss<<content->rdbuf();
        std::cout << "[GetObject Success]"<<std::endl;
        return oss.str();
      }else{
        std::cerr<< "[GetObject Failed]:"<<" code:"<<outcome.error().Code()
                 <<", message:"<< outcome.error().Message()
                 <<", requestId:"<<outcome.error().RequestId()<<std::endl;
        return "";
      }
}

void OssManager::shutdown(){
    _client.reset();
    ShutdownSdk();
}
