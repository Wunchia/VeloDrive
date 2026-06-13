#include "OssManager.h"
#include "MqManager.h"
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <signal.h>

using namespace std;
using json=nlohmann::json;

static volatile bool running=true;
//     ^^^^^^^^
// 告诉编译器：这个变量的值可能被"你看不见的代码"（信号处理函数）随时修改
// 禁止优化————每次 while(running) 必须真正从内存读，不能缓存到寄存器
// 消费者这里没有使用workflow框架 所以使用volatile bool + while 做信号退出
void sig_handler(int){running=false;}

int main(int argc,char *argv[])
{
    signal(SIGINT, sig_handler);

    char* keyId=getenv("ALIBABA_CLOUD_ACCESS_KEY_ID");
    char* keySecret=getenv("ALIBABA_CLOUD_ACCESS_KEY_SECRET");

    if(!keyId||!keySecret){
        cerr<<"[consumer] 请先设置环境变量"<<endl;
        return 1;
    }

    //初始化 OSS 管理器
    OssManager::getInstance().init(
        "oss-cn-wuhan-lr.aliyuncs.com",
        keyId,keySecret,
        "velo-use-20260611",
        "cn-wuhan");

    //初始化 消息队列 管理器
    if(!MqManager::getInstance().init()){
        cerr<<"[consumer] MQ 连接失败"<<endl;
        return 1;
    }
    MqManager::getInstance().setupConsumer("velo.file.queue",
        "velo.direct","file.upload");
    cout<<"[consumer] 开始监听,Ctrl-C 退出"<<endl;

    //死循环处理消息 直到信号将running置为false
    while(running){
        //取到一个消息
        string body=MqManager::getInstance().consumeOne();
        if(body.empty()){continue;}

        try{
            json msg=json::parse(body);
            string hashcode=msg["hashcode"];

            // 读本地文件
            string filepath="./storage/"+hashcode;
            ifstream ifs(filepath,ios::binary);
            if(!ifs){
                cerr<<"[consumer] 文件不存在:"<<filepath<<endl;
                continue;
            }
            string data((istreambuf_iterator<char>(ifs)),
                         istreambuf_iterator<char>());
            ifs.close();

            //上传OSS
            cout<<"[consumer] 正在向阿里云上传："<<hashcode<<" ... "<<endl;
            if(OssManager::getInstance().putObject(hashcode,data)){
                cout<<"[consumer] 上传云成功: "<<hashcode<<endl;
            }else{
                cerr<<"[consumer] 上传云失败: "<<hashcode<<endl;
            }


        }catch(const exception& e){
            cerr<<"[consumer] 异常："<<e.what()<<endl;
        }
    }

    MqManager::getInstance().shutdown();
    OssManager::getInstance().shutdown();
    cout<<"[consumer] Bye"<<endl;
    return 0;
}
