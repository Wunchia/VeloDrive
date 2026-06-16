#include "auth.srpc.h"
#include "AuthServiceImpl.h"
#include "ConsulManager.h"
#include "workflow/WFFacilities.h"
#include "signal.h"
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

static void scheduleHeartbeat(const std::string& instanceId)
{
    auto *timer=WFTaskFactory::create_timer_task(9000000,
        [](WFTimerTask*){
        ConsulManager::getInstance().reportHealth("AuthService-8001");
    });
    auto *series=Workflow::create_series_work(timer,
        [instanceId](const SeriesWork*){
            scheduleHeartbeat(instanceId);
        });
    series->start();
}

void sig_handler(int signo)
{
	wait_group.done();
}

int main()
{
    signal(SIGINT,sig_handler);
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 8001;
	SRPCServer server;

	AuthServiceImpl authservice_impl;
	server.add_service(&authservice_impl);

	// 注册到Consul
	ConsulManager::getInstance().init();
	ConsulManager::getInstance().registerService(
	    "AuthService", "127.0.0.1", 8001,"AuthService-8001");

	server.start(port);
	scheduleHeartbeat("AuthService-8001");
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
