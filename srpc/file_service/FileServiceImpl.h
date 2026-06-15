#pragma once
#include "file.pb.h"
#include "file.srpc.h"
#include <srpc/rpc_context.h>
#include <string>

class FileServiceImpl : public FileService::Service
{
public:
    void ListFiles(ListFilesReq *request,ListFilesResp *response,
                   srpc::RPCContext *ctx)override;

private:
    static const std::string DB_URL;
    static const int DB_RETRY_MAX=3;
};
