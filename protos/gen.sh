#!/bin/bash
# ============================================================
# Proto 代码生成脚本
# 用法: cd protos && bash gen.sh
# ============================================================
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROTOS_DIR="$PROJECT_ROOT/protos"
INCLUDE_DIR="$PROJECT_ROOT/include"
PB_DIR="$PROJECT_ROOT/src/pb"
SRPC_GEN_DIR="$PROJECT_ROOT/srpc/gen"
SRPC_AUTH_DIR="$PROJECT_ROOT/srpc/auth_service"
SRPC_FILE_DIR="$PROJECT_ROOT/srpc/file_service"

echo "=== [protoc] 生成 C++ pb 代码 ==="
protoc --cpp_out="$INCLUDE_DIR" \
    -I"$PROTOS_DIR" \
    "$PROTOS_DIR/auth.proto" \
    "$PROTOS_DIR/file.proto"

echo "=== 移动 .pb.cc 到 $PB_DIR ==="
mv "$INCLUDE_DIR"/*.pb.cc "$PB_DIR/" 2>/dev/null || true

echo "=== [srpc_generator] 生成 SRPC 桩代码 ==="
srpc_generator "$PROTOS_DIR/auth.proto" -o "$SRPC_GEN_DIR"
srpc_generator "$PROTOS_DIR/file.proto" -o "$SRPC_GEN_DIR"

echo "=== 移动骨架代码到服务目录 ==="
mkdir -p "$SRPC_AUTH_DIR" "$SRPC_FILE_DIR"

# srpc_generator 生成的文件名与 proto 文件名对应
if [ -f "$SRPC_GEN_DIR/auth.pb_skeleton.cc" ]; then
    mv "$SRPC_GEN_DIR/auth.pb_skeleton.cc" "$SRPC_AUTH_DIR/server.pb_skeleton.cc"
fi
if [ -f "$SRPC_GEN_DIR/file.pb_skeleton.cc" ]; then
    mv "$SRPC_GEN_DIR/file.pb_skeleton.cc" "$SRPC_FILE_DIR/server.pb_skeleton.cc"
fi

echo "=== 完成 ==="
echo "  .pb.h  → $INCLUDE_DIR/"
echo "  .pb.cc → $PB_DIR/"
echo "  .srpc.h → $SRPC_GEN_DIR/"
echo "  骨架代码 → $SRPC_AUTH_DIR/ , $SRPC_FILE_DIR/"
