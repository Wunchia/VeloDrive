#pragma once
#include "auth.pb.h"
#include "auth.srpc.h"
#include "CryptoUtil.h"
#include <srpc/rpc_context.h>
#include <string>

class AuthServiceImpl:public AuthService::Service
{
public:
    void Register(RegisterReq *request,RegisterResp *response,
                srpc::RPCContext *ctx)override;

    void Login(LoginReq *request,LoginResp *response,
        srpc::RPCContext *ctx)override;

private:
    static const std::string DB_URL;
    static const int DB_RETRY_MAX=3;
};
