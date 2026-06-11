#pragma once
#include <string>
#include <alibabacloud/oss/OssClient.h>
#include <memory>
class OssManager
{
public:
    static OssManager& getInstance();

    bool init(const std::string& endpoint,
              const std::string& accessKeyId,
              const std::string& accessKeySecret,
              const std::string& bucketName,
              const std::string& region="cn_wuhan");

    bool putObject(const std::string& objectName,const std::string& content);

    std::string getObject(const std::string& objectName);

    void shutdown();

private:
    OssManager()=default;
    ~OssManager()=default;
    OssManager(const OssManager&)=delete;
    OssManager& operator=(const OssManager&)=delete;

    std::unique_ptr<AlibabaCloud::OSS::OssClient> _client;
    std::string _bucketName;
    bool _initialized=false;
};
