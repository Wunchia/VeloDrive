#include "AccountHandler.h"
#include "CryptoUtil.h"
#include <nlohmann/json.hpp>
#include <wfrest/HttpDef.h>
#include <wfrest/HttpMsg.h>
#include <workflow/MySQLResult.h>
#include <workflow/Workflow.h>
#include <string>
#include <workflow/mysql_types.h>

using namespace std;
using namespace wfrest;
using json=nlohmann::json;

//------数据库配置常量-------
static const string DB_URL="mysql://root:123456@localhost/VeloDrive";
static const int DB_RETRY_MAX=3;

//---------辅助函数----------

static  void send_error(HttpResp *resp,int status_code,const string &msg){
    json body;
    body["status"]="error";
    body["message"]=msg;
    resp->set_status(status_code);
    resp->add_header("Content-Type","application/json");
    resp->String(body.dump(2));
}

static void send_success(HttpResp *resp,int status_code,const string&msg,const json &data){
    json body;
    body["status"]="success";
    body["message"]=msg;
    body["data"]=data;
    resp->set_status(status_code);
    resp->add_header("Content-Type","application/json");
    resp->String(body.dump(2));
}

// ======================================
//      POST /api/v1/auth/register
// ======================================
void AccountHandler::register_user(const HttpReq *req,HttpResp*resp){
    // 1.解析请求头 校验Content-Type
    // -----可以取出字符串比较---
    // const string &ct=ContentType::to_str(req->content_type());
    // if(ct.find("application/json")==string::npos){
    // -----也可以直接用枚举比较-----
    if(req->content_type()!=wfrest::APPLICATION_JSON){
        send_error(resp, 400, "请求格式有误");
        return;
    }
    // 2.解析请求体 解析json
    json body;
    try{
        body=json::parse(req->body());
    }catch(...){
        send_error(resp,400,"请求格式有误");
        return;
    }
    string username=body.value("username","");
    string password=body.value("password","");
    string confirm=body.value("confirm","");
    // 3.校验参数
    if(username.empty()||password.empty()){
        send_error(resp,400,"用户名和密码不能为空");
        return;
    }
    if(password!=confirm){
        send_error(resp,400,"两次输入的密码不一致");
        return;
    }

    // 4.生成 盐值 和 密码hash
    string salt=CryptoUtil::generate_salt();
    string hash_pwd=CryptoUtil::hash_password(password,salt);

    // 5.拼sql
    string sql="INSERT  INTO tbl_user (username, password, salt) VALUES ('"
                    + username + "', '" + hash_pwd + "', '" + salt + "')";

    // 6.插入数据库 sqlTask
    resp->MySQL(DB_URL, sql, [resp,username](protocol::MySQLResultCursor* cursor) {
        if (cursor->get_cursor_status() == MYSQL_STATUS_OK && cursor->get_affected_rows() == 1) {
            json data;
            data["userId"]=cursor->get_insert_id();
            data["username"]=username;
            send_success(resp,201,"注册成功",data);
        } else {
            send_error(resp,409,"用户名已存在");
        }
    });
}

// ======================================
//      POST /api/v1/auth/login
// ======================================
void AccountHandler::login(const HttpReq *req,HttpResp*resp){
    // 1.解析请求头 content-type
    if(req->content_type()!=wfrest::APPLICATION_JSON){
        send_error(resp, 400, "请求格式有误");
        return;
    }
    // 2.解析请求体 json
    json body;
    try{
        body=json::parse(req->body());
    }catch(...){
        send_error(resp, 400, "请求格式有误");
        return;
    }
    string username=body.value("username","");
    string password=body.value("password","");

    // 3.校验参数
    if(username.empty()||password.empty()){
        send_error(resp, 400, "用户名和密码不能为空");
        return;
    }

    // 4.拼sql
    string sql= "SELECT id, username, password, salt, created_at "
                "FROM tbl_user WHERE username='" + username + "'";

    // 5.查询数据库 sqlTask
    resp->MySQL(DB_URL,sql,[resp,username,password](protocol::MySQLResultCursor* cursor){
        // 没取到结果集
        if(cursor->get_cursor_status()!=MYSQL_STATUS_GET_RESULT){
            send_error(resp, 500, "服务器内部错误");
            return;
        }
        // 取到的行数为0 SQL返回空结果集
        vector<protocol::MySQLCell> row;
        if(!cursor->fetch_row(row)){
            send_error(resp, 401, "用户名或密码错误");
            return;
        }

        // 解析结果集
        int db_id=row[0].as_int();
        string db_username=row[1].as_string();
        string db_password=row[2].as_string();
        string db_salt=row[3].as_string();
        string db_created_at=row[4].as_string();

        // 验证密码
        string hash_input=CryptoUtil::hash_password(password, db_salt);
        if(hash_input!=db_password){
            send_error(resp, 401, "用户名或密码错误");
            return;
        }
        // 生成JWT
        User user;
        user.id=db_id;
        user.username=db_username;
        user.createdAt=db_created_at;
        string token =CryptoUtil::generate_token(user);

        // 构造响应
        json user_data;
        user_data["userId"]=db_id;
        user_data["username"]=db_username;
        json data;
        data["accessToken"]=token;
        data["tokenType"]="Bearer";
        data["user"]=user_data;
        send_success(resp, 200, "登录成功", data);
    });
}

// ======================================
//         GET /api/v1/user/me
// ======================================
void AccountHandler::get_current_user(const HttpReq *req,HttpResp*resp){
    // 提取 token
    const string &auth=req->header("Authorization");
    //没有令牌或者令牌的类型不是"Bearer"
    if(auth.size()<8||auth.substr(0,7)!="Bearer "){
        send_error(resp, 401, "无效的访问令牌");
        return;
    }
    string token=auth.substr(7);

    // 验证 token
    User user;
    //令牌验证失败
    if(!CryptoUtil::verify_token(token, user)){
        send_error(resp, 401, "无效的访问令牌");
        return;
    }

    // 返回用户信息
    json data;
    data["userId"]=user.id;
    data["username"]=user.username;
    data["createdAt"]=user.createdAt;
    send_success(resp, 200, "获取个人信息成功", data);
}
