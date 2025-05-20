#!/usr/bin/env bash
set -e

cd /workspace/Efficient-Fuzzer

# 파일 존재 확인
echo "=== 소스 파일 확인 ==="
if [ ! -f "/workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp" ]; then
    echo "❌ 오류: src/libFuzzer/cmd_dis_libfuzzer.cpp 파일이 존재하지 않습니다."
    echo "브랜치 Week13/Final이 올바르게 클론되었는지 확인하세요."
    echo "현재 디렉토리 구조:"
    find /workspace/Efficient-Fuzzer -type f -name "*.cpp" | sort
    exit 1
fi

if [ ! -f "/workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp" ]; then
    echo "❌ 오류: src/harness/CmdDispatcherHarness.cpp 파일이 존재하지 않습니다."
    echo "브랜치 Week13/Final이 올바르게 클론되었는지 확인하세요."
    echo "현재 디렉토리 구조:"
    find /workspace/Efficient-Fuzzer -type f -name "*.cpp" | sort
    exit 1
fi

# fprime submodule 확인
if [ ! -d "/workspace/Efficient-Fuzzer/src/fprime" ] || [ ! -f "/workspace/Efficient-Fuzzer/src/fprime/Svc/CmdDispatcher/CommandDispatcherImpl.hpp" ]; then
    echo "❌ 오류: fprime submodule이 존재하지 않거나 올바르게 초기화되지 않았습니다."
    echo "git submodule 상태:"
    git submodule status
    
    # 누락된 경우 다시 시도
    echo "submodule 다시 초기화 시도..."
    git submodule init
    git submodule update --recursive
    
    # 다시 확인
    if [ ! -f "/workspace/Efficient-Fuzzer/src/fprime/Svc/CmdDispatcher/CommandDispatcherImpl.hpp" ]; then
        echo "❌ submodule 복구 실패. 수동 확인이 필요합니다."
        echo "fprime 디렉토리 구조:"
        find /workspace/Efficient-Fuzzer/src/fprime -type f -name "*.hpp" | grep -i "cmddispatcher" | sort
        exit 1
    fi
    
    echo "✅ submodule 복구 성공."
fi

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