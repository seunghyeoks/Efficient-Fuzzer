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

# fprime 도구 설치 확인 및 설치
echo "=== fprime 도구 설치 확인 ==="
if command -v fprime-fpp-to-xml > /dev/null 2>&1; then
    echo "✅ fprime fpp 도구가 이미 설치되어 있습니다."
else
    echo "fprime fpp 도구를 설치합니다..."
    pip install -r "/workspace/Efficient-Fuzzer/src/fprime/requirements.txt"
    echo "✅ fprime fpp 도구 설치 완료."
fi

# fprime 오토코더 빌드 확인 및 필요시 재실행
echo "=== fprime 오토코더 빌드 확인 ==="
# 필요한 파일이 있는지 확인하고 없으면 빌드 실행
if [ ! -f "/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native/F-Prime/Svc/CmdDispatcher/CommandDispatcherComponentAc.hpp" ]; then
    echo "필요한 빌드 결과물이 없습니다. 빌드를 실행합니다..."
    # 기존 빌드 디렉토리가 있으면 삭제
    if [ -d "/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native" ]; then
        echo "기존 빌드 디렉토리를 정리합니다..."
        rm -rf /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native
    fi
    # 빌드 실행
    cd /workspace/Efficient-Fuzzer/src/fprime
    fprime-util generate
    fprime-util build
    cd /workspace/Efficient-Fuzzer
    
    # 빌드 후 다시 확인
    if [ ! -f "/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native/F-Prime/Svc/CmdDispatcher/CommandDispatcherComponentAc.hpp" ]; then
        echo "❌ 오류: 빌드 후에도 필요한 파일이 생성되지 않았습니다."
        echo "빌드 디렉토리 상태:"
        find /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native -name "*.hpp" | grep -i "cmddispatcher" || echo "관련 파일을 찾을 수 없습니다."
        exit 1
    fi
else
    echo "✅ fprime 오토코더 빌드가 확인되었습니다."
fi

# 빌드 디렉토리로 이동
mkdir -p build/libfuzzer
cd build/libfuzzer

# src 하위 모든 디렉토리를 -I 옵션으로 변환
INCLUDE_DIRS=$(find /workspace/Efficient-Fuzzer/src -type d | sort | sed 's/^/-I/')

# fprime 라이브러리를 찾기 위한 설정
FPRIME_BUILD_DIR="/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native"

# 여러 라이브러리 디렉토리 검색
LIB_DIRS=(
    "${FPRIME_BUILD_DIR}/lib" 
    "${FPRIME_BUILD_DIR}/F-Prime/Fw/*/lib" 
    "${FPRIME_BUILD_DIR}/F-Prime/Svc/*/lib" 
    "${FPRIME_BUILD_DIR}/F-Prime/Drv/*/lib"
    "${FPRIME_BUILD_DIR}/F-Prime/Os/*/lib"
)

# fprime 헤더 파일 위치 추가
FPRIME_INCLUDES="-I${FPRIME_BUILD_DIR} -I${FPRIME_BUILD_DIR}/F-Prime"

# 디버깅: 라이브러리 디렉토리 목록 표시
echo "=== 검색할 라이브러리 디렉토리 ==="
for dir in "${LIB_DIRS[@]}"; do
    echo "- $dir"
done

# 라이브러리 목록을 순서대로 지정
FPRIME_LIBS=""

# 우선 순위가 높은 라이브러리 패턴 목록
PRIORITY_PATTERNS=("CmdDispatcher" "Command" "Cmd" "Buffer" "Port" "Com" "Comp" "Fw_" "Os_" "Serialize")

# 라이브러리 디렉토리에서 모든 .a 파일 검색
echo "=== 라이브러리 검색 중... ==="
ALL_LIBS=()

for dir in "${LIB_DIRS[@]}"; do
    # 디렉토리가 실제로 존재하는 경로로 확장됨
    for lib_path in $dir/*.a; do
        if [ -f "$lib_path" ]; then
            lib_name=$(basename "$lib_path")
            echo "발견된 라이브러리: $lib_path"
            ALL_LIBS+=("$lib_path")
        fi
    done
done

# 라이브러리가 하나도 없으면 오류 표시
if [ ${#ALL_LIBS[@]} -eq 0 ]; then
    echo "❌ 오류: F' 라이브러리를 찾을 수 없습니다!"
    echo "F' 라이브러리 위치 확인:"
    find ${FPRIME_BUILD_DIR} -name "*.a" || echo "라이브러리 파일을 찾을 수 없습니다."
    exit 1
fi

echo "총 ${#ALL_LIBS[@]}개의 라이브러리 발견"

# 우선순위 라이브러리를 먼저 추가
echo "=== 우선순위 라이브러리 추가 ==="
for pattern in "${PRIORITY_PATTERNS[@]}"; do
    for lib in "${ALL_LIBS[@]}"; do
        if [[ "$(basename "$lib")" =~ $pattern ]]; then
            if [[ ! "$FPRIME_LIBS" =~ "$lib" ]]; then
                echo "우선순위 라이브러리 추가: $(basename "$lib")"
                FPRIME_LIBS="$FPRIME_LIBS $lib"
            fi
        fi
    done
done

# 나머지 라이브러리 추가
echo "=== 나머지 라이브러리 추가 ==="
for lib in "${ALL_LIBS[@]}"; do
    if [[ ! "$FPRIME_LIBS" =~ "$lib" ]]; then
        echo "라이브러리 추가: $(basename "$lib")"
        FPRIME_LIBS="$FPRIME_LIBS $lib"
    fi
done

# 명시적으로 필요한 시스템 라이브러리 지정
SYS_LIBS="-lpthread -ldl -lm -lrt"

# 최종 컴파일 명령어 출력
echo "=== libFuzzer 컴파일 명령 ==="
COMPILE_CMD="clang++ -g -O1 -fsanitize=fuzzer,address -std=c++14 \
    ${INCLUDE_DIRS} \
    ${FPRIME_INCLUDES} \
    /workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp \
    /workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp \
    -Wl,--whole-archive ${FPRIME_LIBS} -Wl,--no-whole-archive ${SYS_LIBS} \
    -o cmd_dispatcher_fuzzer"

echo "$COMPILE_CMD"

# libFuzzer 컴파일 (clang을 사용하여 fuzzing 및 sanitizer 활성화)
clang++ -g -O1 -fsanitize=fuzzer,address -std=c++14 \
    ${INCLUDE_DIRS} \
    ${FPRIME_INCLUDES} \
    /workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp \
    /workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp \
    -Wl,--whole-archive ${FPRIME_LIBS} -Wl,--no-whole-archive ${SYS_LIBS} \
    -o cmd_dispatcher_fuzzer

# 컴파일 결과 확인
if [ $? -ne 0 ]; then
    echo "❌ 컴파일 실패. 오류를 확인하세요."
    echo "라이브러리 심볼 확인:"
    for lib in ${FPRIME_LIBS}; do
        echo "=== $lib 에 포함된 심볼 ==="
        nm -C "$lib" | grep -E 'Cmd|InputCmdPort|OutputCmdResponsePort|CmdDispatcher'
    done
    exit 1
fi

echo "✅ 컴파일 성공!"

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