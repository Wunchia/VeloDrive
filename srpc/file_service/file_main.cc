#include "file.srpc.h"
#include "FileServiceImpl.h"
#include "ConsulManager.h"
#include "workflow/WFFacilities.h"
#include <signal.h>

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

static void scheduleHeartbeat(const std::string& instanceId)
{
    auto *timer=WFTaskFactory::create_timer_task(9000000,
        [](WFTimerTask*){
        ConsulManager::getInstance().reportHealth("FileService-8002");
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
	unsigned short port = 8002;
	SRPCServer server;

	FileServiceImpl fileservice_impl;
	server.add_service(&fileservice_impl);

	ConsulManager::getInstance().init();
	ConsulManager::getInstance().registerService("FileService", "127.0.0.1", 8002, "FileService-8002");

	server.start(port);
	scheduleHeartbeat("FileService-8002");
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
