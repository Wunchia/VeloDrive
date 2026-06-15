#include "file.srpc.h"
#include "FileServiceImpl.h"
#include "workflow/WFFacilities.h"
#include <signal.h>

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

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

	server.start(port);
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
