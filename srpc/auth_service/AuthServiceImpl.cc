#include "AuthServiceImpl.h"
#include "CryptoUtil.h"
#include "auth.pb.h"
#include <srpc/rpc_context.h>
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFTask.h>
#include <workflow/Workflow.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/mysql_types.h>
#include <string>
#include <vector>
using std::string;
using std::vector;
using namespace protocol;

const std::string AuthServiceImpl::DB_URL="mysql://root:123456@localhost/VeloDrive";

//===========================================
//              Register
//===========================================
void AuthServiceImpl::Register(RegisterReq* request,RegisterResp* response,srpc::RPCContext *ctx){
    string username=request->username();
    string password=request->password();
    string confirm=request->confirm();

    if(username.empty()||password.empty()){
        response->set_code(2);
        response->set_message("用户名和密码不能为空");
        return;
    }
    if (password != confirm) {
        response->set_code(2);
        response->set_message("两次输入的密码不一致");
        return;
    }
    string salt=CryptoUtil::generate_salt();
    string hash_pwd=CryptoUtil::hash_password(password,salt);

    string sql= "INSERT INTO tbl_user (username, password, salt) VALUES ('"
        + username + "', '" + hash_pwd + "', '" + salt + "')";

    auto*mysql_task=WFTaskFactory::create_mysql_task(
        DB_URL,DB_RETRY_MAX,
        [response,username,ctx](WFMySQLTask*task){
            int state=task->get_state();
            if(state!=WFT_STATE_SUCCESS){
                response->set_code(2);
                response->set_message("内部服务器错误");
                return;
            }
            MySQLResponse*resp=task->get_resp();
            MySQLResultCursor cursor(resp);
            if(cursor.get_cursor_status()==MYSQL_STATUS_OK&&cursor.get_affected_rows()==1){
                response->set_code(0);
                response->set_message("注册成功");
                response->set_user_id(static_cast<int32_t>(cursor.get_insert_id()));
                response->set_username(username);
            }else{
                response->set_code(1);
                response->set_message("用户名已存在");
            }
        }
    );
    mysql_task->get_req()->set_query(sql);
    ctx->get_series()->push_back(mysql_task);
}

//===========================================
//                 Login
//===========================================
void AuthServiceImpl::Login(LoginReq*request,LoginResp*response,srpc::RPCContext*ctx){
    string username=request->username();
    string password=request->password();

    if(username.empty()||password.empty()){
        response->set_code(2);
        response->set_message("用户名和密码不能为空");
        return;
    }

    string sql= "SELECT id, username, password, salt, created_at "
        "FROM tbl_user WHERE username='" + username + "'";

    auto* mysql_task=WFTaskFactory::create_mysql_task(
        DB_URL,DB_RETRY_MAX,
        [response,password,username,ctx](WFMySQLTask*task){
            int state=task->get_state();
            if(state!=WFT_STATE_SUCCESS){
                response->set_code(2);
                response->set_message("内部服务器错误");
                return;
            }

            MySQLResponse*resp=task->get_resp();
            MySQLResultCursor cursor(resp);

            if(cursor.get_cursor_status()!=MYSQL_STATUS_GET_RESULT){
                response->set_code(2);
                response->set_message("内部服务器错误");
                return;
            }

            vector<MySQLCell> row;
            if(!cursor.fetch_row(row)){
                response->set_code(2);
                response->set_message("用户名或密码错误");
                return;
            }
            int    db_id         = row[0].as_int();
            string db_username   = row[1].as_string();
            string db_password   = row[2].as_string();
            string db_salt       = row[3].as_string();
            string db_created_at = row[4].as_string();

            string hash_input=CryptoUtil::hash_password(password, db_salt);
            if(hash_input!=db_password){
                response->set_code(1);
                response->set_message("用户名或密码错误");
                return;
            }

            // 生成 JWT
            User user;
            user.id        = db_id;
            user.username  = db_username;
            user.createdAt = db_created_at;
            std::string token = CryptoUtil::generate_token(user);

            response->set_code(0);
            response->set_message("登录成功");
            response->set_access_token(token);
            response->set_token_type("Bearer");
            response->set_user_id(db_id);
            response->set_username(db_username);
        }
    );
    mysql_task->get_req()->set_query(sql);
    ctx->get_series()->push_back(mysql_task);
}
