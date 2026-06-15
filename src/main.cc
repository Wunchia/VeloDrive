#include "CloudDiskServer.h"
#include "OssManager.h"
#include "MqManager.h"
#include <google/protobuf/stubs/common.h>
#include <iostream>
#include <signal.h>

using namespace std;

WFFacilities::WaitGroup waitGroup(1);

void sig_handler(int)
{
    waitGroup.done();
}

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    signal(SIGINT, sig_handler);
    srand(time(NULL)); // 设置随机种子

    char* Access_Key_ID=getenv("ALIBABA_CLOUD_ACCESS_KEY_ID");
    char* Access_Key_Secret=getenv("ALIBABA_CLOUD_ACCESS_KEY_SECRET");
    string keyId=Access_Key_ID?Access_Key_ID:"";
    string keySecret=Access_Key_Secret?Access_Key_Secret:"";

    //初始化oss单例
    OssManager::getInstance().init(
        "oss-cn-wuhan-lr.aliyuncs.com",
        keyId,keySecret,
        "velo-use-20260611",
        "cn-wuhan");

    //初始化 MQ 单例
    MqManager::getInstance().init();

    CloudDiskServer server;

    // 注册路由
    server.register_routes();

    if (server.start(8848) == 0) {
        server.list_routes();
        waitGroup.wait();
        server.stop();
    } else {
        cerr << "Error: Server start FAILED!" << endl;
    }

    //销毁oss单例
    OssManager::getInstance().shutdown();
    //销毁MQ单例
    MqManager::getInstance().shutdown();
    google::protobuf::ShutdownProtobufLibrary();
    cout<<"[Server]: Bye ~ "<<endl;
}
