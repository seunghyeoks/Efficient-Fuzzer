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

# 라이브러리 경로 설정
LIB_DIR="${FPRIME_BUILD_DIR}/lib/Linux"

# fprime 헤더 파일 위치 추가
FPRIME_INCLUDES="-I${FPRIME_BUILD_DIR} -I${FPRIME_BUILD_DIR}/F-Prime -I/workspace/Efficient-Fuzzer/src/fprime"

# 디버깅: 라이브러리 디렉토리 목록 표시
echo "=== 라이브러리 디렉토리 확인 ==="
echo "라이브러리 경로: $LIB_DIR"

# 라이브러리 목록을 순서대로 지정
FPRIME_LIBS=""

# 라이브러리 직접 지정 (중요 순서에 따라)
OS_IMPL_LIBS=(
    "${LIB_DIR}/libOs_Posix_Shared.a" 
    "${LIB_DIR}/libOs_Mutex_Posix.a" 
    "${LIB_DIR}/libOs_File_Posix.a" 
    "${LIB_DIR}/libOs_RawTime_Posix.a" 
    "${LIB_DIR}/libOs_Task_Posix.a" 
    "${LIB_DIR}/libOs_Console_Posix.a" 
    "${LIB_DIR}/libOs_Cpu_Linux.a" 
    "${LIB_DIR}/libOs_Memory_Linux.a" 
    "${LIB_DIR}/libOs.a"
)

# ActiveComponentBase 및 QueuedComponentBase 를 포함하는 라이브러리가 중요
FW_COMPONENT_LIBS=(
    "${LIB_DIR}/libFw_Obj.a" # 기본 객체
    "${LIB_DIR}/libFw_Port.a" # PortBase 등
    "${LIB_DIR}/libFw_Comp.a" # PassiveComponentBase
    "${LIB_DIR}/libFw_CompQueued.a" # QueuedComponentBase, ActiveComponentBase
)

CORE_LIBS=(
    "${LIB_DIR}/libFw_Types.a"
    "${LIB_DIR}/libFw_Cmd.a" 
    "${LIB_DIR}/libFw_Com.a" 
    "${LIB_DIR}/libFw_Buffer.a" 
    "${LIB_DIR}/libFw_Logger.a" 
    "${LIB_DIR}/libSvc_CmdDispatcher.a" 
    "${LIB_DIR}/libFw_Tlm.a" 
    "${LIB_DIR}/libFw_Time.a" 
    "${LIB_DIR}/libFw_Log.a"
    "${LIB_DIR}/libUtils.a" # stringFormat을 포함할 수 있음
    "${LIB_DIR}/libsnprintf-format.a" # stringFormat 대체 또는 보조
    "${LIB_DIR}/libconfig.a"
    "${LIB_DIR}/libSvc_Ping.a"
)

# OS 구현 라이브러리 추가
echo "=== OS 구현 라이브러리 추가 ==="
for lib in "${OS_IMPL_LIBS[@]}"; do
    if [ -f "$lib" ]; then
        echo "라이브러리 추가: $(basename "$lib")"
        FPRIME_LIBS="$FPRIME_LIBS $lib"
    else
        echo "경고: OS 라이브러리 $lib 파일이 존재하지 않습니다."
    fi
done

# Fw Component 라이브러리 추가
echo "=== Fw Component 라이브러리 추가 ==="
for lib in "${FW_COMPONENT_LIBS[@]}"; do
    if [ -f "$lib" ]; then
        echo "라이브러리 추가: $(basename "$lib")"
        FPRIME_LIBS="$FPRIME_LIBS $lib"
    else
        echo "경고: Fw Component 라이브러리 $lib 파일이 존재하지 않습니다."
    fi
done

# 핵심 라이브러리 추가
echo "=== 핵심 라이브러리 추가 ==="
for lib in "${CORE_LIBS[@]}"; do
    if [ -f "$lib" ]; then
        echo "라이브러리 추가: $(basename "$lib")"
        FPRIME_LIBS="$FPRIME_LIBS $lib"
    else
        echo "경고: 핵심 라이브러리 $lib 파일이 존재하지 않습니다."
    fi
done

# 나머지 모든 라이브러리 추가 (위에서 명시적으로 추가된 것 제외)
echo "=== 나머지 라이브러리 추가 ==="
find "${LIB_DIR}" -name "*.a" | sort | while read -r lib; do
    # 이미 추가된 라이브러리는 건너뛰기
    if [[ ! "$FPRIME_LIBS" =~ "$lib" ]]; then
        echo "라이브러리 추가: $(basename "$lib")"
        FPRIME_LIBS="$FPRIME_LIBS $lib"
    fi
done

# 명시적으로 필요한 시스템 라이브러리 지정
SYS_LIBS="-lpthread -ldl -lrt -lm -lstdc++ -lutil"

# 컴파일러 및 링커 플래그 설정
CXXFLAGS="-g -O1 -fsanitize=fuzzer,address -std=c++14 -fvisibility=default -D_GLIBCXX_USE_CXX11_ABI=1"
LDFLAGS="-Wl,-z,defs -Wl,--no-as-needed"

# 최종 컴파일 명령어 출력
echo "=== libFuzzer 컴파일 명령 ==="
COMPILE_CMD="clang++ ${CXXFLAGS} \
    ${INCLUDE_DIRS} \
    ${FPRIME_INCLUDES} \
    /workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp \
    /workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp \
    ${LDFLAGS} -Wl,--start-group ${FPRIME_LIBS} -Wl,--end-group ${SYS_LIBS} \
    -o cmd_dispatcher_fuzzer"

echo "$COMPILE_CMD"

# libFuzzer 컴파일
clang++ ${CXXFLAGS} \
    ${INCLUDE_DIRS} \
    ${FPRIME_INCLUDES} \
    /workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp \
    /workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp \
    ${LDFLAGS} -Wl,--start-group ${FPRIME_LIBS} -Wl,--end-group ${SYS_LIBS} \
    -o cmd_dispatcher_fuzzer

# 컴파일 결과 확인
if [ $? -ne 0 ]; then
    echo "❌ 컴파일 실패. 오류를 확인하세요."
    echo "누락된 심볼 확인:"
    
    # 누락된 심볼들에 대해 어떤 라이브러리에 있는지 확인
    for symbol in "Os::ConsoleInterface::getDelegate" "Os::CpuInterface::getDelegate" "Fw::stringFormat" "Fw::ActiveComponentBase" "Os::Mutex" "Os::Queue"; do
        echo "=== 심볼 '$symbol' 검색 결과 ==="
        for lib_file in ${LIB_DIR}/*.a; do # 변수명 수정: lib -> lib_file
            if nm -C "$lib_file" 2>/dev/null | grep -q "$symbol"; then # nm 에러 무시 및 변수명 수정
                echo "발견: $lib_file"
            fi
        done
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