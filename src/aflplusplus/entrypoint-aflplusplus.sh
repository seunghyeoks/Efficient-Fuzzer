#!/bin/bash
# AFL++ 컴파일러를 설정
export CC=afl-clang-fast
export CXX=afl-clang-fast++
set -e

echo "=== F-Prime CmdDispatcher AFL++ 시작 ==="

# 빌드 디렉토리 설정
BUILD_DIR="/workspace/Efficient-Fuzzer/src/aflplusplus/build"

echo "=== 기존 빌드 디렉토리 삭제 및 생성 ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"


# === Seed corpus 자동 생성 ===
echo "=== Seed corpus 자동 생성 (corpus/seed_*.bin) ==="
mkdir -p "$BUILD_DIR/corpus"
# NoOp 명령 (opcode 0, 인자 0, context 1)
python3 -c "with open('$BUILD_DIR/corpus/seed_noop.bin','wb') as f: f.write(bytes([0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00]))"
# 사용자 명령 (opcode 0x50, 인자 100, context 2)
python3 -c "with open('$BUILD_DIR/corpus/seed_user.bin','wb') as f: f.write(bytes([0x01,0x00,0x50,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x02,0x00,0x00,0x00]))"


echo "=== CMake 구성 시작 ==="
cmake .. -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX

echo "=== Make 빌드 시작 (cmd_fuzzer_afl) ==="
make cmd_fuzzer_afl

echo "=== 결과 디렉토리 생성 $BUILD_DIR/findings ==="
mkdir -p findings

echo "=== AFL++ fuzzing 시작 ==="
afl-fuzz -i corpus -o findings -- ./cmd_fuzzer_afl @@ 