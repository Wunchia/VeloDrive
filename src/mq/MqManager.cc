#include "MqManager.h"
#include <SimpleAmqpClient/BasicMessage.h>
#include <SimpleAmqpClient/Envelope.h>
#include <exception>
#include <iostream>
// #include <thread> //拉取模式需要
// #include <chrono> //拉取模式需要

using namespace AmqpClient;
using std::string;
using std::cerr;
using std::cout;
using std::endl;

MqManager& MqManager::getInstance(){
    static MqManager instance;
    return instance;
}

bool MqManager::init(const string&uri){
    try{
        _channel=Channel::CreateFromUri(uri);
        _initialized=true;
        cout<<"[MqManager] 连接 RabbitMQ 成功"<<endl;
        return true;
    }catch(const std::exception& e){
        cerr<<"[MqManager] ERROR-连接失败："<<e.what()<<endl;
        return false;
    }
}

bool MqManager::publish(const string& exchange,
                        const string& routingKey,
                        const string& body)
{
    if(!_initialized){return false;}

    try{
        auto msg=BasicMessage::Create(body);
        _channel->BasicPublish(exchange, routingKey ,msg);
        return true;
    }catch(const std::exception& e){
        cerr<<"[MqManager] ERROR-发送失败:"<<e.what()<<endl;
        return false;
    }
}

void MqManager::setupConsumer(const string& queue,
    const string& exchange,const string& routingKey)
{
    if(!_initialized){return;}
    _queueName=queue;
    //声明交换机(幂等：已存在则不变)
    _channel->DeclareExchange(exchange,"direct");

    //声明队列 （幂等：已存在则不变）
    _channel->DeclareQueue(queue,false,true,false,false);
    //                       passive durable exlusive auto_delete

    //绑定 把 routingKey 的消息从exchange路由到queue
    _channel->BindQueue(queue,exchange,routingKey);

    //订阅队列 （推送模式）
    _channel->BasicConsume(queue,"",true,true,true,1);
    //协议帧内容（AMQP basic.consume method）：
    //  queue        = "velo.file.queue"   // 订阅哪个队列
    //  consumer_tag = ""                  // 空 → Broker 自动分配一个唯一 tag
    //  no_local     = true                // 不接收同一连接上发出去的消息
    //  no_ack       = true                // 自动确认（Broker 投递后立即删消息）
    //  exclusive    = true                // 独占：其他消费者连不上这个队列
    //  prefetch     = 1                   // 同一时间最多推 1 条未确认的消息

    cout<<"[MqManager]订阅队列: "<<queue<<" <-- "
    <<exchange<<":"<<routingKey<<endl;
}

string MqManager::consumeOne(){
    if(!_initialized){return "";}

    // 阻塞模式 阻塞等待 RabbitMQ推送消息
    Envelope::ptr_t envelope;
    bool ok=_channel->BasicConsumeMessage(envelope,500);
    if(ok&&envelope && envelope->Message()){
        return envelope->Message()->Body();
    }

    // 拉取模式
    // Envelope::ptr_t envelope;
    // bool ok=_channel->BasicGet(envelope,_queueName,true);
    // if(ok&&envelope&&envelope->Message()){
    //     return envelope->Message()->Body();
    // }
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return "";
}

void MqManager::shutdown(){
    _channel.reset();
    _initialized=false;
}

// BasicConsume（推送）            BasicGet（拉取）
// ─────────────────────          ─────────────────
// 订阅一次，永久生效              每次调都是一次新的独立的请求

// Broker 侧：
//   维护订阅者列表                不维护任何状态
//   消息来了直接推                消息来了等着被拉

// 客户端侧：
//   消息在客户端内存缓冲区排队      消息在 Broker 队列里排队
//   取消息快（内存直接拿）         取消息慢（每次网络往返一次）

// 适合：
//   长期运行的消费者               偶尔查询、需要精确控制节奏
//   高吞吐                        低频率


// T0: PID 1001 (server) 执行 publish()
//     │  FileHandler::upload_file() 调用 MqManager::publish()
//     │  → SimpleAmqpClient 构造 AMQP 协议帧
//     │  → write() 到 TCP socket → 发给 127.0.0.1:5672
//     │
//     ▼
// T1: PID 2002 (Broker/Docker) 执行全部
//     │  RabbitMQ 进程的 TCP 线程收到 basic.publish 帧
//     │  → ① 找交换机 "velo.direct"
//     │  → ② 根据 "file.upload" 找绑定关系
//     │  → ③ 消息入队 "velo.file.queue"
//     │  → ④ 查订阅者列表 → 找到了 (PID 3003 的 BasicConsume 注册的)
//     │  → ⑤ 构造 basic.deliver 帧 → write() 到 PID 3003 的 TCP socket
//     │
//     ▼
// T2: PID 3003 (consumer) 内部，I/O 线程执行
//     │  librabbitmq-c 的后台线程从 TCP socket read()
//     │  → 反序列化 basic.deliver 帧
//     │  → push 进 SimpleAmqpClient 内部的 _message_queue（内存队列）
//     │
//     ▼
// T3: PID 3003 (consumer) 内部，主线程执行
//     │  main() 的 while(running) 调 MqManager::consumeOne()
//     │  → _channel->BasicConsumeMessage(envelope, 500)
//     │  → 从 _message_queue（内存）弹出 Envelope
//     │  → 返回消息体 → consumer_main.cc 处理之
