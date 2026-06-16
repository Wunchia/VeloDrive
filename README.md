<p align="center">
  <h1 align="center">VeloDrive</h1>
  <p align="center">个人云盘 · C++17 微服务架构 · 五期渐进式演进</p>
</p>

---

## 项目总览

| 期数 | 主题 | 新增技术 |
|:--:|------|----------|
| ❶ | 基础功能 | wfrest · MySQL · JWT · OpenSSL |
| ❷ | 云存储备份 | 阿里云 OSS C++ SDK |
| ❸ | 异步消息队列 | RabbitMQ · SimpleAmqpClient |
| ❹ | 微服务拆分 | Protobuf · sRPC |
| ❺ | 服务注册与发现 | Consul · ppconsul · TTL 心跳 |

---

## 架构

```
                  Browser / Frontend
                         │  :8848
                         ▼
┌──────────────────────────────────────────────────┐
│                API Gateway (server)               │
│                                                  │
│  Auth endpoints  ── sRPC ──→ AuthService  :8001 │
│  File list       ── sRPC ──→ FileService  :8002 │
│  Upload / Download     →  本地处理 (不经过微服务)  │
│  OSS · MQ Producer · JWT                         │
└───────┬──────────────────────────┬──────────────┘
        │ sRPC                     │ sRPC
        ▼                          ▼
┌───────────────┐          ┌───────────────┐
│ Auth Service  │          │ File Service  │
│    :8001      │          │    :8002      │
│  Register     │          │  ListFiles    │
│  Login        │          │  MySQL        │
│  MySQL        │          └───────────────┘
└───┬───────────┘
    │
    └─────────┬─────────────────┐
              │ 注册 · 心跳 · 发现       │
              ▼                          ▼
      ┌──────────────┐          ┌──────────────┐
      │    Consul    │          │  RabbitMQ    │
      │    :8500     │          │   :5672      │
      │  (Docker)    │          │  (Docker)    │
      └──────────────┘          └──────┬───────┘
                                       │
                                       ▼
                               ┌──────────────┐
                               │   consumer   │
                               │  MQ → OSS    │
                               └──────────────┘
```

---

## 目录结构

```
VeloDrive/
├── CMakeLists.txt
├── build.sh
├── docs/                         ← 项目文档
│   ├── 技术栈与依赖安装.md
│   └── 模块架构与工作原理.md
├── include/                      ← 公共头文件
│   ├── AccountHandler.h            handler 声明
│   ├── CloudDiskServer.h           HTTP 路由
│   ├── ConsulManager.h             注册/发现
│   ├── CryptoUtil.h                哈希 · JWT
│   ├── FileHandler.h              文件 handler
│   ├── MqManager.h                 RabbitMQ
│   ├── OssManager.h                OSS
│   ├── auth.pb.h                   Protobuf 生成
│   └── file.pb.h
├── protos/                       ← Proto 定义
│   ├── auth.proto                  AuthService RPC
│   ├── file.proto                  FileService RPC
│   └── gen.sh                      代码生成脚本
├── src/                          ← Gateway 源码
│   ├── main.cc                    入口
│   ├── handler/                    HTTP handler
│   ├── server/                     路由注册
│   ├── crypto/                     密码/JWT
│   ├── consul/                     Consul SDK
│   ├── mq/                         RabbitMQ · consumer
│   ├── oss/                        OSS SDK
│   └── pb/                         Protobuf 实现
├── srpc/                         ← 微服务
│   ├── gen/                        sRPC 生成代码
│   ├── auth_service/               Auth 微服务
│   └── file_service/               File 微服务
├── tests/                        ← 单元测试
│   └── test_crypto_util.cc
└── www/                          ← 前端静态资源
```

---

## 快速开始

### 1. 启动基础设施

```bash
# MySQL
sudo systemctl start mysql

# RabbitMQ
docker run -d --name rabbit \
  -p 5672:5672 -p 15672:15672 \
  rabbitmq:management

# Consul
docker run -d --name consul \
  -p 8500:8500 \
  consul agent -server -bootstrap-expect=1 -ui -bind=0.0.0.0 -client=0.0.0.0

# OSS 凭证
export ALIBABA_CLOUD_ACCESS_KEY_ID="your_key"
export ALIBABA_CLOUD_ACCESS_KEY_SECRET="your_secret"
```

### 2. 编译

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
```

### 3. 启动（需 4 个终端）

```bash
./build/bin/auth_service    # 终端 1 — 监听 :8001
./build/bin/file_service    # 终端 2 — 监听 :8002
./build/bin/consumer        # 终端 3 — MQ → OSS
./build/bin/server          # 终端 4 — 监听 :8848
```

打开 `http://localhost:8848`

> 如果 `http_proxy` 环境变量已设置，启动前先 `unset http_proxy https_proxy`

---

## API

| 方法 | 路径 | 说明 |
|------|------|------|
| `POST` | `/api/v1/auth/register` | 注册 |
| `POST` | `/api/v1/auth/login` | 登录 |
| `GET` | `/api/v1/user/me` | 当前用户信息 |
| `GET` | `/api/v1/files` | 文件列表 |
| `POST` | `/api/v1/files` | 上传文件 |
| `GET` | `/api/v1/file/{id}` | 下载文件 |

---

## 测试

```bash
./build/bin/unit_tests                        # 直接运行
cd build && ctest --output-on-failure         # 通过 CTest
```

---

## 文档

| 文档 | 内容 |
|------|------|
| [`docs/技术栈与依赖安装.md`](docs/技术栈与依赖安装.md) | 第三方库清单与安装方法 |
| [`docs/模块架构与工作原理.md`](docs/模块架构与工作原理.md) | 请求链路、服务发现、数据流 |
| [`docs/CMake构建系统说明.md`](docs/CMake构建系统说明.md) | CMake 结构与扩展指南 |

---

## 技术栈

`C++17` · `CMake` · `wfrest` · `workflow` · `sRPC` · `Protobuf` · `MySQL` · `RabbitMQ` · `Consul` · `OSS` · `OpenSSL` · `libjwt` · `ppconsul` · `SimpleAmqpClient` · `GoogleTest`
