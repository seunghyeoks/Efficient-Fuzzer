#!/usr/bin/env bash
set -e

cd /workspace/Efficient-Fuzzer

# 빌드 디렉토리로 이동
cd build/libfuzzer

# libFuzzer 컴파일 (clang을 사용하여 fuzzing 및 sanitizer 활성화)
echo "=== libFuzzer 컴파일 시작 ==="
clang++ -g -O1 -fsanitize=fuzzer,address \
    -I/workspace/Efficient-Fuzzer/src/fprime \
    -I/workspace/Efficient-Fuzzer/src \
    /workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp \
    /workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp \
    -o cmd_dispatcher_fuzzer

# 코퍼스 디렉토리 생성
mkdir -p corpus

# 초기 코퍼스 생성
echo "=== 초기 코퍼스 생성 ==="
# 기본 테스트를 0회 실행하여 코퍼스만 생성
./cmd_dispatcher_fuzzer -runs=0 corpus

# libFuzzer 실행
echo "=== libFuzzer 실행 시작 ==="
mkdir -p findings
./cmd_dispatcher_fuzzer -max_len=1024 -artifact_prefix=findings/ corpus

# 명령줄 인수로 전달된 명령 실행
exec "$@" 