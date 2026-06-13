#include "auth.srpc.h"
#include "workflow/WFFacilities.h"

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

static void register_done(RegisterResp *response, srpc::RPCContext *context)
{
}

static void login_done(LoginResp *response, srpc::RPCContext *context)
{
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;

	AuthService::SRPCClient client(ip, port);

	// example for RPC method call
	RegisterReq register_req;
	//register_req.set_message("Hello, srpc!");
	client.Register(&register_req, register_done);

	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
