#pragma once
#include <wfrest/HttpMsg.h>
#include <wfrest/HttpServer.h>
#include <workflow/Workflow.h>

class AccountHandler
{   //纯工具类 静态方法 不需要构造对象
public:
    // POST /api/v1/auth/register
    static void register_user(const wfrest::HttpReq *req,wfrest::HttpResp*resp,SeriesWork*series);

    // POST /api/v1/auth/login
    static void login(const wfrest::HttpReq *req,wfrest::HttpResp*resp,SeriesWork* series);

    // GET /api/v1/user/me
    static void get_current_user(const wfrest::HttpReq *req,wfrest::HttpResp*resp);

private:
    AccountHandler()=delete;
};
