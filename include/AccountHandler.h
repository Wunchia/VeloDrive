#pragma once
#include <wfrest/HttpMsg.h>
#include <wfrest/HttpServer.h>

class AccountHandler
{   //纯工具类 静态方法 不需要构造对象
public:
    // POST /api/v1/auth/register
    static void register_user(const wfrest::HttpReq *req,wfrest::HttpResp*resp);

    // POST /api/v1/auth/login
    static void login(const wfrest::HttpReq *req,wfrest::HttpResp*resp);

    // GET /api/v1/user/me
    static void get_current_user(const wfrest::HttpReq *req,wfrest::HttpResp*resp);

private:
    AccountHandler()=delete;
};
