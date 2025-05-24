#!/bin/bash
# 컴파일러를 clang 계열로 설정 (g++ 대신 clang 사용)
export CC=clang
export CXX=clang++
set -e

echo "=== F-Prime CmdDispatcher LibFuzzer 시작 ==="

# CMake 기반 빌드 디렉토리 설정
BUILD_DIR="/workspace/Efficient-Fuzzer/src/libFuzzer/build"
# find /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/ -name FppConstantsAc.hpp -ls || echo "FppConstantsAc.hpp not found anywhere in UT build directory"

# !!!!! 수정: 빌드 디렉토리 항상 새로 생성 !!!!!
echo "=== 기존 빌드 디렉토리 삭제 ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=== CMake 구성 시작 ==="
# CMake 실행 시 상세 로그 출력 옵션 추가 (필요시)
# cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ..
cmake ..

echo "=== Make 빌드 시작 ==="
# make VERBOSE=1 cmd_fuzzer # 임시 주석 처리. 상세 로그 필요시 이 라인 사용
make cmd_fuzzer # 현재 상태 유지


# 빌드 출력 디렉토리로 바이너리 복사
OUTPUT_DIR="/workspace/Efficient-Fuzzer/build/libfuzzer"
mkdir -p "$OUTPUT_DIR"
 cp cmd_fuzzer "$OUTPUT_DIR/cmd_dispatcher_fuzzer" # 임시 주석 처리

# 실행 디렉토리 이동
 cd "$OUTPUT_DIR" # 임시 주석 처리

# 코퍼스 및 findings 디렉토리 생성 (실행 디렉토리 기준)
 mkdir -p corpus findings # 임시 주석 처리

echo "=== libFuzzer 실행 시작 (현재 비활성화됨) ==="
# ASAN 설정
 export ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:handle_abort=1:abort_on_error=0"
# 퍼징 실행
./cmd_dispatcher_fuzzer -max_len=1024 \
                       -artifact_prefix=findings/ \
                       -print_final_stats=1 \
                       -fork=1 \
                       -runs=-1 \
                       corpus

# 무한 대기
echo "=== 스크립트 완료 (TestNominal 실행) ==="