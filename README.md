# VeloDrive

个人云盘项目，基于 C++17 / wfrest / workflow / sRPC 的微服务架构。

## 项目分期

| 期数 | 内容 | 技术栈 |
|:--:|------|--------|
| 一期 | 用户注册/登录、文件列表/上传/下载 | wfrest + MySQL + JWT |
| 二期 | 阿里云 OSS 对象存储备份 | OSS C++ SDK |
| 三期 | 异步消息队列 | RabbitMQ + SimpleAmqpClient |
| 四期 | 微服务拆分 | Protobuf + sRPC |
| 五期 | 服务注册与发现 | Consul + ppconsul |

## 架构

```
浏览器 / 前端
    │  HTTP :8848
    ▼
┌─────────────────────────────────────────┐
│  API Gateway (server)                   │
│                                         │
│  /api/v1/auth/*      → sRPC → AuthService
│  /api/v1/files (GET)  → sRPC → FileService
│  /api/v1/files (POST)   留在网关（直接上传)
│  /api/v1/file/{id}      留在网关（直接下载)
│                                         │
│  OSS 备份  ·  MQ 生产者  ·  JWT 验证    │
└──────┬───────────────────┬──────────────┘
       │ sRPC              │ sRPC
       ▼                   ▼
┌──────────────┐   ┌──────────────┐
│ Auth Service │   │ File Service │
│   :8001      │   │   :8002      │
│              │   │              │
│ 注册 / 登录  │   │ 文件列表查询  │
│ MySQL:tbl_user│  │ MySQL:tbl_file│
└──────────────┘   └──────────────┘
       │                   │
       └──────┬────────────┘
              │ 注册/发现
              ▼
      ┌──────────────┐
      │    Consul    │
      │    :8500     │
      │ (Docker)     │
      └──────────────┘

RabbitMQ (Docker) :5672  ← MQ 备份任务
consumer (独立进程)       ← 消费 MQ → OSS 上传
```

## 目录结构

```
VeloDrive/
├── CMakeLists.txt                # 顶层构建
├── build.sh                      # 构建脚本
├── docs/                         # 文档
│   ├── Web网盘项目.pdf
│   └── CMake构建系统说明.md
├── include/                      # 公共头文件
│   ├── AccountHandler.h
│   ├── CloudDiskServer.h
│   ├── ConsulManager.h
│   ├── CryptoUtil.h
│   ├── FileHandler.h
│   ├── MqManager.h
│   ├── OssManager.h
│   ├── auth.pb.h                 # Protobuf 生成
│   └── file.pb.h
├── protos/                       # Proto 定义文件
│   ├── auth.proto
│   ├── file.proto
│   └── gen.sh                    # 代码生成脚本
├── src/                          # 网关源码
│   ├── CMakeLists.txt
│   ├── main.cc                   # 网关入口
│   ├── consul/ConsulManager.cc   # Consul 注册/发现
│   ├── crypto/CryptoUtil.cc      # 加密/JWT
│   ├── handler/
│   │   ├── AccountHandler.cc     # 登录/注册 handler
│   │   └── FileHandler.cc        # 文件 CRUD handler
│   ├── mq/
│   │   ├── MqManager.cc          # RabbitMQ 客户端
│   │   └── consumer_main.cc      # 消息消费者
│   ├── oss/OssManager.cc         # OSS 客户端
│   ├── pb/                       # Protobuf 生成(.cc)
│   └── server/CloudDiskServer.cc # 路由注册
├── srpc/                         # 微服务
│   ├── CMakeLists.txt
│   ├── gen/                      # sRPC 生成代码
│   ├── auth_service/             # Auth 微服务
│   │   ├── auth_main.cc
│   │   └── AuthServiceImpl.cc
│   └── file_service/             # File 微服务
│       ├── file_main.cc
│       └── FileServiceImpl.cc
├── tests/                        # 单元测试
│   ├── CMakeLists.txt
│   └── test_crypto_util.cc
└── www/                          # 前端静态资源
    ├── index.html
    └── static/
```

## 环境依赖

| 组件 | 版本/说明 |
|------|----------|
| C++ | 17+ |
| CMake | 3.15+ |
| MySQL | 8.0+ |
| RabbitMQ | (Docker) `rabbitmq:management` |
| Consul | (Docker) `hashicorp/consul` |
| workflow | wfrest 依赖 |
| sRPC | Protobuf RPC 框架 |
| OpenSSL | libcrypto + libssl |
| libjwt | JWT 生成/验证 |
| ppconsul | Consul C++ 客户端 |
| SimpleAmqpClient | RabbitMQ C++ 客户端 |
| Google Test | 单元测试框架 |

## 启动基础设施

```bash
# MySQL（宿主机）
sudo systemctl start mysql

# RabbitMQ
docker run -d --name rabbit -p 5672:5672 -p 15672:15672 rabbitmq:management

# Consul
docker run -d --name consul -p 8500:8500 consul agent -server -bootstrap-expect=1 -ui -bind=0.0.0.0 -client=0.0.0.0

# OSS 环境变量
export ALIBABA_CLOUD_ACCESS_KEY_ID="your_key"
export ALIBABA_CLOUD_ACCESS_KEY_SECRET="your_secret"
```

## 编译

```bash
# 首次构建
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)

# 后续构建
bash build.sh
```

## 启动微服务（按顺序）

```bash
# 终端 1：Auth Service
./build/bin/auth_service        # 监听 8001

# 终端 2：File Service
./build/bin/file_service        # 监听 8002

# 终端 3：消息消费者
./build/bin/consumer

# 终端 4：API Gateway
./build/bin/server              # 监听 8848
```

打开浏览器访问 `http://localhost:8848`

## API 接口

### 认证

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/auth/register` | 注册 |
| POST | `/api/v1/auth/login`    | 登录 |
| GET  | `/api/v1/user/me`       | 获取当前用户 |

### 文件

| 方法 | 路径 | 说明 |
|------|------|------|
| GET  | `/api/v1/files`     | 文件列表 |
| POST | `/api/v1/files`     | 上传文件 |
| GET  | `/api/v1/file/{id}` | 下载文件 |

## 测试

```bash
# 编译测试
make unit_tests

# 运行测试
./build/bin/unit_tests

# 通过 CTest
cd build && ctest --output-on-failure
```

## 分支

| 分支 | 内容 |
|------|------|
| `main` | 最新代码 |
| `v3-bak` | 三期完成版（单体架构） |
| `v4-bak` | 四期完成版（微服务架构） |
