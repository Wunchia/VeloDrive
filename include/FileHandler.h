#pragma once
#include <wfrest/HttpMsg.h>
#include <wfrest/HttpServer.h>

class FileHandler
{
public:
    // GET /api/v1/files
    static void list_files(const wfrest::HttpReq *req,wfrest::HttpResp *resp);

    // POST /api/v1/files
    static void upload_file(const wfrest::HttpReq *req,wfrest::HttpResp *resp);

    // GET /api/v1/file/{id}
    static void download_file(const wfrest::HttpReq *req,wfrest::HttpResp *resp);

private:
    FileHandler()=delete;
};
