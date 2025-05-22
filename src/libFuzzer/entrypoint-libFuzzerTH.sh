#!/bin/bash
set -e

echo "=== F-Prime CmdDispatcher LibFuzzer 시작 ==="

# CMake 기반 빌드 디렉토리 설정
BUILD_DIR="/workspace/Efficient-Fuzzer/src/libFuzzer/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=== CMake 구성 시작 ==="
cmake ..

echo "=== Make 빌드 시작 ==="
make cmd_fuzzer

# 빌드 출력 디렉토리로 바이너리 복사
OUTPUT_DIR="/workspace/Efficient-Fuzzer/build/libfuzzer"
mkdir -p "$OUTPUT_DIR"
cp cmd_fuzzer "$OUTPUT_DIR/cmd_dispatcher_fuzzer"

# 코퍼스 디렉토리 생성
mkdir -p corpus findings

# 실행 디렉토리 이동
cd "$OUTPUT_DIR"

echo "=== libFuzzer 실행 시작 ==="
# ASAN 설정
export ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:handle_abort=1:abort_on_error=0"
# 퍼징 실행
./cmd_dispatcher_fuzzer -max_len=1024 \
                       -artifact_prefix=findings/ \
                       -ignore_crashes=1 \
                       -print_final_stats=1 \
                       -fork=1 \
                       -runs=-1 \
                       corpus

# 무한 대기
echo "=== 퍼징 완료 ==="