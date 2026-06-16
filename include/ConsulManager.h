#pragma once
#include <memory>
#include <ppconsul/agent.h>
#include <ppconsul/consul.h>
#include <ppconsul/health.h>
#include <string>

struct ServiceAddr{
    std::string ip;
    int port;
};

class ConsulManager
{
public:
    static ConsulManager& getInstance();

    //初始化（GateWay和微服务都调用）
    bool init(const std::string& consulAddr="127.0.0.1:8500");
    //注册服务（微服务调用）
    bool registerService(const std::string& svcName,const std::string& ip,int svcPort,const std::string& instanceId="");
    //报告健康情况(微服务调用)
    void reportHealth(const std::string& instanceId);
    //发现服务（GateWay调用）
    ServiceAddr discover(const std::string& svcName);

private:
    ConsulManager()=default;
    ~ConsulManager()=default;
    ConsulManager(const ConsulManager&)=delete;
    ConsulManager& operator=(const ConsulManager&)=delete;

    std::unique_ptr<ppconsul::Consul> _consul;
    bool _initialized=false;
};
