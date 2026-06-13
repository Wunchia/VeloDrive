#include "file.srpc.h"
#include "workflow/WFFacilities.h"

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

static void listfiles_done(ListFilesResp *response, srpc::RPCContext *context)
{
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;

	FileService::SRPCClient client(ip, port);

	// example for RPC method call
	ListFilesReq listfiles_req;
	//listfiles_req.set_message("Hello, srpc!");
	client.ListFiles(&listfiles_req, listfiles_done);

	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
