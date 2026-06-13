#include "auth.srpc.h"
#include "workflow/WFFacilities.h"

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

class AuthServiceServiceImpl : public AuthService::Service
{
public:

	void Register(RegisterReq *request, RegisterResp *response, srpc::RPCContext *ctx) override
	{
		// TODO: fill server logic here
	}

	void Login(LoginReq *request, LoginResp *response, srpc::RPCContext *ctx) override
	{
		// TODO: fill server logic here
	}
};

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1412;
	SRPCServer server;

	AuthServiceServiceImpl authservice_impl;
	server.add_service(&authservice_impl);

	server.start(port);
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
