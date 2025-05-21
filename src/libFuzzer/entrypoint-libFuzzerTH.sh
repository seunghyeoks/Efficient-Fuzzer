#!/bin/bash
set -e

echo "=== F-Prime CmdDispatcher LibFuzzer 시작 ==="

# 빌드 디렉토리로 이동
mkdir -p /workspace/Efficient-Fuzzer/build/libfuzzer
cd /workspace/Efficient-Fuzzer/build/libfuzzer

# 모든 소스 디렉토리를 포함 경로로 변환
INCLUDE_DIRS=$(find /workspace/Efficient-Fuzzer/src -type d | sort | sed 's/^/-I/')

# fprime 빌드 디렉토리 설정
FPRIME_BUILD_DIR="/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native"

# 라이브러리 경로 설정
LIB_DIR="${FPRIME_BUILD_DIR}/lib/Linux"

# fprime 헤더 파일 경로 추가
FPRIME_INCLUDES="-I${FPRIME_BUILD_DIR} -I${FPRIME_BUILD_DIR}/F-Prime -I/workspace/Efficient-Fuzzer/src/fprime"

echo "=== ActiveTestGTestBase.hpp 경로 검색 ==="
find /workspace/Efficient-Fuzzer -name ActiveTestGTestBase.hpp || echo "ActiveTestGTestBase.hpp 파일을 찾을 수 없습니다."

echo "=== CommandDispatcherGTestBase.hpp 경로 검색 ==="
find /workspace/Efficient-Fuzzer -name CommandDispatcherGTestBase.hpp || echo "CommandDispatcherGTestBase.hpp 파일을 찾을 수 없습니다."


echo "=== 라이브러리 디렉토리 확인 ==="
echo "라이브러리 경로: $LIB_DIR"

# 모든 fprime 정적 라이브러리 링크 (Stub 라이브러리 제외)
ALL_LIBS=()
for lib in "${LIB_DIR}"/*.a; do
    base=$(basename "$lib")
    # Stub 라이브러리는 제외
    if [[ ! "$base" =~ _Stub\.a$ ]]; then
        ALL_LIBS+=("$lib")
    fi
done

# stringFormat 구현을 위한 Fw Types 소스
SNPRINTF_SRC=(/workspace/Efficient-Fuzzer/src/fprime/Fw/Types/snprintf_format.cpp)

# 필요한 시스템 라이브러리 지정
SYS_LIBS="-lpthread -ldl -lrt -lm -lstdc++"

# 컴파일러 및 링커 플래그 설정
CXXFLAGS="-g -O1 -fsanitize=fuzzer,address -std=c++14"
LDFLAGS="-Wl,-z,defs -Wl,--no-as-needed"

echo "=== libFuzzer 컴파일 및 링크 ==="
clang++ ${CXXFLAGS} \
    ${INCLUDE_DIRS} \
    ${FPRIME_INCLUDES} \
    /workspace/Efficient-Fuzzer/src/libFuzzer/libFuzzer_main.cpp \
    /workspace/Efficient-Fuzzer/src/fprime/Svc/CmdDispatcher/test/ut/CommandDispatcherTester.cpp \
    "${SNPRINTF_SRC[@]}" \
    ${LDFLAGS} -Wl,--whole-archive ${ALL_LIBS[@]} -Wl,--no-whole-archive ${SYS_LIBS} \
    -o cmd_dispatcher_fuzzer

if [ $? -ne 0 ]; then
    echo "❌ 컴파일 또는 링크 실패."
    exit 1
fi

echo "✅ 컴파일 성공!"

# 코퍼스 디렉토리 생성
mkdir -p corpus findings

# libFuzzer 실행
echo "=== libFuzzer 실행 시작 ==="

# 더 많은 디버깅 정보 출력 및 어서트 처리 설정
export ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:handle_abort=1:abort_on_error=0"

./cmd_dispatcher_fuzzer -max_len=1024 \
                       -artifact_prefix=findings/ \
                       -ignore_crashes=1 \
                       -print_final_stats=1 \
                       -fork=1 \
                       -runs=-1 \
                       corpus

# 무한 대기 (Ctrl+C로 종료 가능)
echo "=== 퍼징 완료 ==="
echo "결과는 findings/ 디렉토리에 저장됩니다."