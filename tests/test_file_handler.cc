#include "CloudDiskServer.h"
#include "CryptoUtil.h"
#include "OssManager.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <workflow/WFFacilities.h>

#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

using namespace std;
using json = nlohmann::json;

// ============================================================
//  测试配置
// ============================================================

static const int TEST_PORT = 18889;
static WFFacilities::WaitGroup wg(1);

// ============================================================
//  辅助函数
// ============================================================

static string exec(const string &cmd)
{
    array<char, 4096> buf;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
        result += buf.data();
    return result;
}

static string curl(const string &method,
                   const string &path,
                   const string &headers,
                   const string &body = "")
{
    string url = "http://localhost:" + to_string(TEST_PORT) + path;
    string cmd = "curl -s --noproxy '*' -w '\\n%{http_code}' -X " + method
               + " " + headers + " '" + url + "' 2>&1";
    if (!body.empty())
        cmd += " -d '" + body + "'";
    return cmd;
}

static void run_server()
{
    CloudDiskServer server;
    server.register_routes();
    server.start(TEST_PORT);
    wg.wait();
    server.stop();
}

// ============================================================
//  测试夹具
// ============================================================

class FileHandlerTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // 清理测试数据
        system("mysql -u root -p123456 VeloDrive "
               "-e \"DELETE FROM tbl_file WHERE uid IN "
               "(SELECT id FROM tbl_user WHERE username LIKE 'filetest%')\" "
               "2>/dev/null");
        system("mysql -u root -p123456 VeloDrive "
               "-e \"DELETE FROM tbl_user WHERE username LIKE 'filetest%'\" "
               "2>/dev/null");

        // 初始化 OSS（和 main.cc 一致）
        char* key_id     = getenv("ALIBABA_CLOUD_ACCESS_KEY_ID");
        char* key_secret = getenv("ALIBABA_CLOUD_ACCESS_KEY_SECRET");
        OssManager::getInstance().init(
            "oss-cn-wuhan-lr.aliyuncs.com",
            key_id    ? key_id    : "",
            key_secret ? key_secret : "",
            "velo-use-20260611",
            "cn-wuhan");

        server_thread = thread(run_server);
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    static void TearDownTestSuite()
    {
        wg.done();
        if (server_thread.joinable())
            server_thread.join();

        OssManager::getInstance().shutdown();

        system("mysql -u root -p123456 VeloDrive "
               "-e \"DELETE FROM tbl_file WHERE uid IN "
               "(SELECT id FROM tbl_user WHERE username LIKE 'filetest%')\" "
               "2>/dev/null");
        system("mysql -u root -p123456 VeloDrive "
               "-e \"DELETE FROM tbl_user WHERE username LIKE 'filetest%'\" "
               "2>/dev/null");
    }

    static pair<string, int> parse(const string &raw)
    {
        auto pos = raw.rfind('\n');
        if (pos == string::npos) return {"", 0};
        return {raw.substr(0, pos), stoi(raw.substr(pos + 1))};
    }

    // 注册 + 登录，返回 token
    static string register_and_login(const string &username, const string &password)
    {
        exec(curl("POST", "/api/v1/auth/register",
            "-H 'Content-Type: application/json'",
            R"({"username":")" + username + R"(","password":")" + password +
            R"(","confirm":")" + password + R"("})"));

        auto [body, _] = parse(exec(curl("POST", "/api/v1/auth/login",
            "-H 'Content-Type: application/json'",
            R"({"username":")" + username + R"(","password":")" + password + R"("})")));

        return json::parse(body)["data"]["accessToken"].get<string>();
    }

    static thread server_thread;
};

thread FileHandlerTest::server_thread;

// ============================================================
//  列表文件  GET /api/v1/files
// ============================================================

TEST_F(FileHandlerTest, ListFiles_Success)
{
    string token = register_and_login("filetest_list", "abc123");

    auto [body, status] = parse(exec(curl("GET", "/api/v1/files",
        "-H 'Authorization: Bearer " + token + "'")));

    EXPECT_EQ(status, 200);
    json resp = json::parse(body);
    EXPECT_EQ(resp["status"], "success");
    EXPECT_TRUE(resp["data"]["files"].is_array());
}

TEST_F(FileHandlerTest, ListFiles_NoToken)
{
    auto [body, status] = parse(exec(curl("GET", "/api/v1/files", "")));
    EXPECT_EQ(status, 401);
}

TEST_F(FileHandlerTest, ListFiles_InvalidToken)
{
    auto [body, status] = parse(exec(curl("GET", "/api/v1/files",
        "-H 'Authorization: Bearer garbage'")));
    EXPECT_EQ(status, 401);
}

// ============================================================
//  上传文件  POST /api/v1/files
// ============================================================

TEST_F(FileHandlerTest, UploadFile_Success)
{
    string token = register_and_login("filetest_upload", "abc123");

    auto [body, status] = parse(exec(curl("POST", "/api/v1/files",
        "-H 'Authorization: Bearer " + token + "' "
        "-F 'file=@/etc/hostname'")));

    EXPECT_EQ(status, 201);
    json resp = json::parse(body);
    EXPECT_EQ(resp["status"], "success");
    EXPECT_GT(resp["data"]["fileId"].get<int>(), 0);
    EXPECT_FALSE(resp["data"]["filename"].get<string>().empty());
}

TEST_F(FileHandlerTest, UploadFile_NoToken)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/files",
        "-F 'file=@/etc/hostname'")));

    EXPECT_EQ(status, 401);
}

TEST_F(FileHandlerTest, UploadFile_InvalidToken)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/files",
        "-H 'Authorization: Bearer garbage' "
        "-F 'file=@/etc/hostname'")));

    EXPECT_EQ(status, 401);
}

TEST_F(FileHandlerTest, UploadFile_WrongContentType)
{
    string token = register_and_login("filetest_noctype", "abc123");

    // 发送 JSON 而不是 multipart/form-data
    auto [body, status] = parse(exec(curl("POST", "/api/v1/files",
        "-H 'Authorization: Bearer " + token + "' "
        "-H 'Content-Type: application/json'",
        R"({"foo":"bar"})")));

    EXPECT_EQ(status, 400);
}

// ============================================================
//  下载文件  GET /api/v1/file/{id}
// ============================================================

TEST_F(FileHandlerTest, DownloadFile_Success)
{
    string token = register_and_login("filetest_dl", "abc123");

    // 先上传一个文件
    auto [up_body, up_status] = parse(exec(curl("POST", "/api/v1/files",
        "-H 'Authorization: Bearer " + token + "' "
        "-F 'file=@/etc/hostname'")));

    ASSERT_EQ(up_status, 201);
    int file_id = json::parse(up_body)["data"]["fileId"].get<int>();

    // 下载该文件
    auto [body, status] = parse(exec(curl("GET",
        "/api/v1/file/" + to_string(file_id),
        "-H 'Authorization: Bearer " + token + "'")));

    EXPECT_EQ(status, 200);
    EXPECT_FALSE(body.empty());
}

TEST_F(FileHandlerTest, DownloadFile_NotFound)
{
    string token = register_and_login("filetest_dl404", "abc123");

    auto [body, status] = parse(exec(curl("GET", "/api/v1/file/99999999",
        "-H 'Authorization: Bearer " + token + "'")));

    EXPECT_EQ(status, 404);
    json resp = json::parse(body);
    EXPECT_EQ(resp["status"], "error");
}

TEST_F(FileHandlerTest, DownloadFile_NoToken)
{
    auto [body, status] = parse(exec(curl("GET", "/api/v1/file/1", "")));
    EXPECT_EQ(status, 401);
}

TEST_F(FileHandlerTest, DownloadFile_InvalidToken)
{
    auto [body, status] = parse(exec(curl("GET", "/api/v1/file/1",
        "-H 'Authorization: Bearer garbage'")));
    EXPECT_EQ(status, 401);
}

TEST_F(FileHandlerTest, DownloadFile_OtherUsersFile)
{
    // 用户A 上传文件
    string token_a = register_and_login("filetest_ownera", "abc123");
    auto [up_body, up_status] = parse(exec(curl("POST", "/api/v1/files",
        "-H 'Authorization: Bearer " + token_a + "' "
        "-F 'file=@/etc/hostname'")));

    ASSERT_EQ(up_status, 201);
    int file_id = json::parse(up_body)["data"]["fileId"].get<int>();

    // 用户B 尝试下载用户A的文件
    string token_b = register_and_login("filetest_ownerb", "abc123");
    auto [body, status] = parse(exec(curl("GET",
        "/api/v1/file/" + to_string(file_id),
        "-H 'Authorization: Bearer " + token_b + "'")));

    EXPECT_EQ(status, 404);
}

// ============================================================
//  完整流程测试：注册 -> 登录 -> 上传 -> 列表 -> 下载
// ============================================================

TEST_F(FileHandlerTest, FullWorkflow)
{
    // 1. 注册 + 登录
    string token = register_and_login("filetest_flow", "abc123");

    // 2. 列表为空
    {
        auto [body, status] = parse(exec(curl("GET", "/api/v1/files",
            "-H 'Authorization: Bearer " + token + "'")));
        EXPECT_EQ(status, 200);
        EXPECT_EQ(json::parse(body)["data"]["files"].size(), 0u);
    }

    // 3. 上传文件
    int file_id;
    {
        auto [body, status] = parse(exec(curl("POST", "/api/v1/files",
            "-H 'Authorization: Bearer " + token + "' "
            "-F 'file=@/etc/hostname'")));
        EXPECT_EQ(status, 201);
        file_id = json::parse(body)["data"]["fileId"].get<int>();
    }

    // 4. 列表包含刚上传的文件
    {
        auto [body, status] = parse(exec(curl("GET", "/api/v1/files",
            "-H 'Authorization: Bearer " + token + "'")));
        EXPECT_EQ(status, 200);
        json files = json::parse(body)["data"]["files"];
        EXPECT_EQ(files.size(), 1u);
        EXPECT_EQ(files[0]["fileId"].get<int>(), file_id);
    }

    // 5. 下载该文件
    {
        auto [body, status] = parse(exec(curl("GET",
            "/api/v1/file/" + to_string(file_id),
            "-H 'Authorization: Bearer " + token + "'")));
        EXPECT_EQ(status, 200);
        EXPECT_FALSE(body.empty());
    }
}
