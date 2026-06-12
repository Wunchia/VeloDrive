#include "CloudDiskServer.h"
#include "CryptoUtil.h"

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

static const int TEST_PORT = 18888;
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

class AccountHandlerTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        system("mysql -u root -p123456 VeloDrive "
               "-e \"DELETE FROM tbl_user WHERE username LIKE 'test%'\" "
               "2>/dev/null");

        server_thread = thread(run_server);
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    static void TearDownTestSuite()
    {
        wg.done();
        if (server_thread.joinable())
            server_thread.join();

        system("mysql -u root -p123456 VeloDrive "
               "-e \"DELETE FROM tbl_user WHERE username LIKE 'test%'\" "
               "2>/dev/null");
    }

    static pair<string, int> parse(const string &raw)
    {
        auto pos = raw.rfind('\n');
        if (pos == string::npos) return {"", 0};
        return {raw.substr(0, pos), stoi(raw.substr(pos + 1))};
    }

    static thread server_thread;
};

thread AccountHandlerTest::server_thread;

// ============================================================
//  注册
// ============================================================

TEST_F(AccountHandlerTest, Register_Success)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test001","password":"abc123","confirm":"abc123"})")));

    EXPECT_EQ(status, 201);
    json resp = json::parse(body);
    EXPECT_EQ(resp["status"], "success");
    EXPECT_EQ(resp["data"]["username"], "test001");
}

TEST_F(AccountHandlerTest, Register_Duplicate)
{
    exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test002","password":"abc123","confirm":"abc123"})"));

    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test002","password":"abc123","confirm":"abc123"})")));

    EXPECT_EQ(status, 409);
    json resp = json::parse(body);
    EXPECT_EQ(resp["status"], "error");
}

TEST_F(AccountHandlerTest, Register_EmptyUsername)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"","password":"abc123","confirm":"abc123"})")));

    EXPECT_EQ(status, 400);
    EXPECT_NE(json::parse(body)["message"].get<string>().find("不能为空"), string::npos);
}

TEST_F(AccountHandlerTest, Register_PasswordMismatch)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test003","password":"abc123","confirm":"different"})")));

    EXPECT_EQ(status, 400);
    EXPECT_NE(json::parse(body)["message"].get<string>().find("不一致"), string::npos);
}

TEST_F(AccountHandlerTest, Register_WrongContentType)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: text/plain'",
        R"({"username":"test004","password":"abc123","confirm":"abc123"})")));

    EXPECT_EQ(status, 400);
}

// ============================================================
//  登录
// ============================================================

TEST_F(AccountHandlerTest, Login_Success)
{
    exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test010","password":"mypwd","confirm":"mypwd"})"));

    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/login",
        "-H 'Content-Type: application/json'",
        R"({"username":"test010","password":"mypwd"})")));

    EXPECT_EQ(status, 200);
    json resp = json::parse(body);
    EXPECT_EQ(resp["data"]["tokenType"], "Bearer");
    EXPECT_FALSE(resp["data"]["accessToken"].get<string>().empty());
}

TEST_F(AccountHandlerTest, Login_WrongPassword)
{
    exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test011","password":"correct","confirm":"correct"})"));

    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/login",
        "-H 'Content-Type: application/json'",
        R"({"username":"test011","password":"wrong"})")));

    EXPECT_EQ(status, 401);
}

TEST_F(AccountHandlerTest, Login_NonExistentUser)
{
    auto [body, status] = parse(exec(curl("POST", "/api/v1/auth/login",
        "-H 'Content-Type: application/json'",
        R"({"username":"no_such_user_xyz","password":"x"})")));

    EXPECT_EQ(status, 401);
}

// ============================================================
//  获取当前用户信息
// ============================================================

TEST_F(AccountHandlerTest, GetCurrentUser_Success)
{
    exec(curl("POST", "/api/v1/auth/register",
        "-H 'Content-Type: application/json'",
        R"({"username":"test020","password":"mypwd","confirm":"mypwd"})"));

    auto [login_body, _] = parse(exec(curl("POST", "/api/v1/auth/login",
        "-H 'Content-Type: application/json'",
        R"({"username":"test020","password":"mypwd"})")));

    string token = json::parse(login_body)["data"]["accessToken"];

    auto [body, status] = parse(exec(curl("GET", "/api/v1/user/me",
        "-H 'Authorization: Bearer " + token + "'")));

    EXPECT_EQ(status, 200);
    json resp = json::parse(body);
    EXPECT_EQ(resp["data"]["username"], "test020");
    EXPECT_FALSE(resp["data"]["createdAt"].get<string>().empty());
}

TEST_F(AccountHandlerTest, GetCurrentUser_NoToken)
{
    auto [body, status] = parse(exec(curl("GET", "/api/v1/user/me", "")));
    EXPECT_EQ(status, 401);
}

TEST_F(AccountHandlerTest, GetCurrentUser_InvalidToken)
{
    auto [body, status] = parse(exec(curl("GET", "/api/v1/user/me",
        "-H 'Authorization: Bearer garbage'")));
    EXPECT_EQ(status, 401);
}
