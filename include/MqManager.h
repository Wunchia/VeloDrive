#pragma once

#include <SimpleAmqpClient/Channel.h>
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <memory>

class MqManager
{
public:
    static MqManager& getInstance();

    bool init(const std::string& uri="amqp://guest:guest@localhost:5672/%2f");

    bool publish(const std::string& exchange,
                 const std::string& routingKey,
                 const std::string& body);

    void setupConsumer(const std::string& queue,
        const std::string& exchange,const std::string& routingKey);
    std::string consumeOne();

    void shutdown();

private:
    MqManager()=default;
    ~MqManager()=default;
    MqManager(const MqManager&)=delete;
    MqManager& operator=(const MqManager&)=delete;

    std::string _queueName;
    AmqpClient::Channel::ptr_t _channel;
    bool _initialized=false;
};
