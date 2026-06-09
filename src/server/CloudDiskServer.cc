#include "CloudDiskServer.h"
#include "AccountHandler.h"
#include "FileHandler.h"
#include "CryptoUtil.h"
#include "common.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <wfrest/PathUtil.h>
#include <workflow/HttpUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/Workflow.h>
#include <workflow/mysql_types.h>

using namespace std;
using namespace std::placeholders;
using namespace wfrest;
using namespace protocol;
using json = nlohmann::json;

// 数据库的URL
static const string DatabaseURL = "mysql://root:123456@localhost/VeloDrive";
static const int RetryMax = 3;

void CloudDiskServer::register_routes()
{
    // 设置静态资源的路由
    register_www_module();
    register_auth_module();
    register_user_module();
    register_file_module();
    // ...
}

void CloudDiskServer::register_www_module()
{
    server_.Static("/", "./www/index.html");
    server_.Static("/static", "./www/static");
}

void CloudDiskServer::register_auth_module(){
    server_.POST("/api/v1/auth/register",AccountHandler::register_user);
    server_.POST("/api/v1/auth/login",AccountHandler::login);
}

void CloudDiskServer::register_user_module(){
    server_.GET("/api/v1/user/me",AccountHandler::get_current_user);
}

void CloudDiskServer::register_file_module(){
    server_.GET("/api/v1/files",FileHandler::list_files);
    server_.POST("/api/v1/files",FileHandler::upload_file);
    server_.GET("/api/v1/file/{id}",FileHandler::download_file);
}
