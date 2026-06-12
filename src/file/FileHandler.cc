#include "FileHandler.h"
#include "CryptoUtil.h"
#include "OssManager.h"
#include "MqManager.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <wfrest/HttpContent.h>
#include <wfrest/HttpDef.h>
#include <wfrest/HttpMsg.h>
#include <wfrest/PathUtil.h>
#include <workflow/MySQLResult.h>
#include <sys/stat.h>
#include <fstream>
#include <cstdio>
#include <workflow/mysql_types.h>

using namespace std;
using namespace wfrest;
using json=nlohmann::json;

//------配置常量------
static const string DB_URL="mysql://root:123456@localhost/VeloDrive";
static const string STORAGE_DIR="./storage";

//------辅助函数------
static void send_error(HttpResp* resp,int code,const string &msg){
    json body;
    body["status"]="error";
    body["message"]=msg;
    resp->set_status(code);
    resp->add_header("Content-Type", "application/json");
    resp->String(body.dump(2));
}

static void send_success(HttpResp* resp,int code,const string &msg,const json &data){
    json body;
    body["status"]="success";
    body["message"]=msg;
    body["data"]=data;
    resp->set_status(code);
    resp->add_header("Content-Type", "application/json");
    resp->String(body.dump(2));
}

// 提取token
static string extract_bearer_token(const HttpReq*req){
    const string &auth=req->header("Authorization");
    if(auth.size()<8||auth.substr(0,7)!="Bearer "){
        return "";
    }
    return auth.substr(7);
}

// 验证token 成功返回uid 失败返回-1 并发送错误信息
static int verify_token(const HttpReq *req,HttpResp *resp){
    string token=extract_bearer_token(req);
    if(token.empty()){
        send_error(resp, 401, "无效的访问令牌");
        return -1;
    }
    User user;
    if(!CryptoUtil::verify_token(token, user)){
        send_error(resp, 401, "无效的访问令牌");
        return -1;
    }
    return user.id;
}

// 验证存储目录存在
static bool ensure_storage_dir(){
    struct stat st;
    if(stat(STORAGE_DIR.c_str(),&st)!=0){
        //目录的权限设置为 0755 rwx r-x r-x
        return mkdir(STORAGE_DIR.c_str(),0755)==0;
    }
    return true;
}

// ==========================================
//            陈列 GET /api/v1/files
// ==========================================
void FileHandler::list_files(const HttpReq *req,HttpResp *resp){
    int uid=verify_token(req,resp);
    if(uid==-1){return;}

    string sql= "SELECT id, filename, hashcode, size, created_at, last_update "
                "FROM tbl_file WHERE uid=" + to_string(uid);
    resp->MySQL(DB_URL,sql,[resp](protocol::MySQLResultCursor *cursor){
        if(cursor->get_cursor_status()!=MYSQL_STATUS_GET_RESULT){
            send_error(resp, 500, "内部服务器错误");
            return;
        }
        json files=json::array();
        vector<protocol::MySQLCell> row;
        while(cursor->fetch_row(row)){
            json f;
            f["fileId"]=row[0].as_int();
            f["filename"]=row[1].as_string();
            f["size"]=row[3].as_int();
            f["createdAt"]=row[4].as_string();
            f["updatedAt"]=row[5].as_string();
            files.push_back(f);
            row.clear();
        }
        json data;
        data["files"]=files;
        send_success(resp, 200, "获取文件列表成功", data);
    });
}

// ==========================================
//            上传 POST /api/v1/files
// ==========================================
void FileHandler::upload_file(const HttpReq *req,HttpResp *resp){
    // 鉴权
    int uid=verify_token(req,resp);
    if(uid==-1){return;}
    //解析请求头
    if(req->content_type()!=MULTIPART_FORM_DATA){
        send_error(resp, 400, "请求格式有误");
        return;
    }

    //解析请求体 解析 multipart 表单 拿到文件名和文件数据
    const Form &form=req->form();
    auto it=form.find("file");
    if(it==form.end()){
        send_error(resp, 400, "请求格式有误");
        return;
    }
    string filename=it->second.first;
    string file_data=it->second.second;
    if(filename.empty()||file_data.empty()){
        send_error(resp, 400, "请求格式有误");
        return;
    }

    //计算文件哈希
    string hashcode=CryptoUtil::generate_hashcode(file_data.c_str(), file_data.size());

    //确保存储目录存在
    ensure_storage_dir();

    //存盘
    string basename=STORAGE_DIR+"/"+hashcode;
    resp->Save(basename,file_data);
    // bool isPutObject=OssManager::getInstance().putObject(hashcode,file_data);
    // if(!isPutObject){
    //     send_error(resp, 500, "上传云存储失败");
    //     return;
    // }

    // 通知消费者备份到OSS （异步，不等结果）
    json mq_msg;
    mq_msg["hashcode"]=hashcode;
    bool ok=MqManager::getInstance().publish("velo.direct", "file.upload", mq_msg.dump(2));
    if(!ok){
        cerr<<"[FileHandler] MQ publish failed!"<<endl;
    }

    //插入数据库
    string sql= "INSERT INTO tbl_file (uid, filename, hashcode, size) VALUES ("
                  + to_string(uid) + ", '"
                  + filename + "', '"
                  + hashcode + "', "
                  + to_string(file_data.size()) + ")";

    resp->MySQL(DB_URL,sql,[resp,filename](protocol::MySQLResultCursor*cursor){
        if(cursor->get_cursor_status()==MYSQL_STATUS_OK&&cursor->get_affected_rows()==1){
            json data;
            data["fileId"]=cursor->get_insert_id();
            data["filename"]=filename;
            send_success(resp, 201, "上传成功", data);
        }else{
            send_error(resp, 500, "内部服务器错误");
        }
    });
}

// ==========================================
//          下载 GET /api/v1/file/{id}
// ==========================================
void FileHandler::download_file(const HttpReq *req,HttpResp *resp){
    //鉴权
    int uid=verify_token(req, resp);
    if(uid==-1){return;}
    //解析请求头 获取路径参数 文件id
    string file_id=req->param("id");
    if(file_id.empty()){
        send_error(resp, 400, "请求格式有误");
        return;
    }

    //查数据库
    string sql= "SELECT uid, filename, hashcode FROM tbl_file WHERE id=" + file_id;
    resp->MySQL(DB_URL,sql,[resp,uid](protocol::MySQLResultCursor* cursor){
        if(cursor->get_cursor_status()!=MYSQL_STATUS_GET_RESULT){
            send_error(resp, 500, "内部服务器错误");
            return;
        }
        vector<protocol::MySQLCell> row;
        if(!cursor->fetch_row(row)){
            send_error(resp, 404, "文件不存在");
            return;
        }

        int db_uid=row[0].as_int();
        string db_filename=row[1].as_string();
        string db_hash=row[2].as_string();

        //检查文件归属
        if(db_uid!=uid){
            send_error(resp,404,"文件不存在");
            return;
        }

        //读盘

        string filepath=STORAGE_DIR+"/"+db_hash;
        struct stat st;
        if(stat(filepath.c_str(),&st)==0){
            resp->File(filepath);//本地有则从本地下载
        }else{  //本地文件损坏（丢失） 则从OSS下载
            string content=OssManager::getInstance().getObject(db_hash);
            if(content.empty()){
                send_error(resp, 500, "云文件下载失败");
                return;
            }
            resp->String(content);
        }
    });
}
