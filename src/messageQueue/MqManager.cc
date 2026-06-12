#include "MqManager.h"
#include <SimpleAmqpClient/BasicMessage.h>
#include <SimpleAmqpClient/Envelope.h>
#include <exception>
#include <iostream>
#include <thread>
#include <chrono>

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
    // _channel->BasicConsume(queue);
    cout<<"[MqManager]订阅队列: "<<queue<<" <-- "
    <<exchange<<":"<<routingKey<<endl;
}

string MqManager::consumeOne(){
    if(!_initialized){return "";}

    // 阻塞模式 阻塞等待 RabbitMQ推送消息
    // Envelope::ptr_t envelope=_channel->BasicConsumeMessage();
    // if(envelope && envelope->Message()){
    //     return envelope->Message()->Body();
    // }
    // return "";

    // 拉取模式
    Envelope::ptr_t envelope;
    bool ok=_channel->BasicGet(envelope,_queueName,true);
    if(ok&&envelope&&envelope->Message()){
        return envelope->Message()->Body();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return "";

}

void MqManager::shutdown(){
    _channel.reset();
    _initialized=false;
}
