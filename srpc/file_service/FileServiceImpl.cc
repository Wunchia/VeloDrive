#include "FileServiceImpl.h"
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>
#include <string>
#include <vector>
#include <workflow/mysql_types.h>

using std::string;
using std::vector;
using namespace protocol;

const string FileServiceImpl::DB_URL="mysql://root:123456@localhost/VeloDrive";

void FileServiceImpl::ListFiles(ListFilesReq *request,
                                ListFilesResp *response,
                                srpc::RPCContext *ctx)
{
    int32_t uid=request->uid();

    string sql= "SELECT id, filename, size, created_at, last_update "
                "FROM tbl_file WHERE uid=" + std::to_string(uid);

    auto *mysql_task=WFTaskFactory::create_mysql_task(
        DB_URL,DB_RETRY_MAX,
        [response](WFMySQLTask *task){
            int state=task->get_state();
            if(state!=WFT_STATE_SUCCESS){
                response->set_code(1);
                response->set_message("内部服务器错误");
                return;
            }

            MySQLResponse *resp=task->get_resp();
            MySQLResultCursor cursor(resp);

            if(cursor.get_cursor_status()!=MYSQL_STATUS_GET_RESULT){
                response->set_code(1);
                response->set_message("内部服务器错误");
                return;
            }

            response->set_code(0);
            response->set_message("获取文件列表成功");

            vector<MySQLCell> row;
            while(cursor.fetch_row(row)){
                auto *file=response->add_files();
                file->set_file_id(row[0].as_int());
                file->set_filename(row[1].as_string());
                file->set_size(row[2].as_int());
                file->set_created_at(row[3].as_string());
                file->set_updated_at(row[4].as_string());
                row.clear();
            }
        }
    );

    mysql_task->get_req()->set_query(sql);
    ctx->get_series()->push_back(mysql_task);
}
