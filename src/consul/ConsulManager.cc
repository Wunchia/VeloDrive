#include "ConsulManager.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ppconsul/agent.h>
#include <ppconsul/consul.h>
#include <ppconsul/health.h>
#include <string>

using namespace ppconsul;
using std::string;

ConsulManager& ConsulManager::getInstance(){
    static ConsulManager instance;
    return instance;
}

//初始化（GateWay和微服务都调用）
bool ConsulManager::init(const string& consulAddr){
    try{
        _consul=std::make_unique<Consul>(consulAddr);
        _initialized=true;
        std::cout<<"[ConsulManager]连接 Consul: "<<consulAddr<<std::endl;
        return true;
    }catch(const std::exception& e){
        std::cerr<<"[ConsulManager] 连接失败："<<e.what()<<std::endl;
        return false;
    }
}

//注册服务（微服务调用）
bool ConsulManager::registerService(const string& svcName,
                                    const string& ip,
                                    int svcPort,
                                    const string& instanceId)
{
    if(!_initialized) {return false;}
    try{
        agent::Agent ag(*_consul);
        if(instanceId.empty()){
            ag.registerService(
                agent::kw::name=svcName,
                agent::kw::address=ip,
                agent::kw::port=static_cast<uint16_t>(svcPort),
                agent::kw::check=agent::TtlCheck{std::chrono::seconds(10)}
            );
        }else{
            ag.registerService(
                agent::kw::name=svcName,
                agent::kw::address=ip,
                agent::kw::port=static_cast<uint16_t>(svcPort),
                agent::kw::check=agent::TtlCheck{std::chrono::seconds(10)},
                agent::kw::id=instanceId
            );
        }
        std::cout<<"[ConsulManager] 注册："<<svcName
                 <<" @ "<<ip<<":"<<svcPort<<std::endl;
        return true;
    }catch(const std::exception& e){
        std::cerr<<"[ConsulManager] 注册失败："<<e.what()<<std::endl;
        return false;
    }
}

//报告健康情况(微服务调用)
void ConsulManager::reportHealth(const std::string& instanceId){
    if(!_initialized){return;}
    try{
        agent::Agent ag(*_consul);
        ag.servicePass(instanceId);
    }catch(const std::exception& e){
        std::cerr<<"[ConsulManager] 心跳失败："<<e.what()<<std::endl;
    }
}

//发现服务（GateWay调用）
ServiceAddr ConsulManager::discover(const std::string& svcName){
    ServiceAddr addr{"",0};
    if(!_initialized){return addr;}
    try{
        health::Health h(*_consul);
        auto services=h.service(svcName);

        if(!services.empty()){
            auto& info=std::get<1>(services[0]);
            addr.ip=info.address;
            addr.port=info.port;
        }
    }catch(const std::exception &e){
        std::cerr<<"[ConsulManager] 发现失败："<<e.what()<<std::endl;
    }
    return addr;
}
