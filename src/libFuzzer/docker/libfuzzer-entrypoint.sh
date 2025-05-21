#!/usr/bin/env bash
# Ensure a clean state by attempting to convert line endings, then proceed.
if command -v dos2unix > /dev/null 2>&1; then
    dos2unix "$0" 
elif command -v sed > /dev/null 2>&1; then
    sed -i 's/\r$//' "$0"
fi
# 오류가 발생해도 스크립트가 계속 실행되도록 set -e 제거

# fprime 큐 구현을 generic으로 변경
export OS_QUEUE_PLATFORM=generic
echo "=== 환경 설정 ==="
echo "OS_QUEUE_PLATFORM=${OS_QUEUE_PLATFORM}"

# 디버깅을 위한 /dev/mqueue 마운트 확인
if [ -d "/dev/mqueue" ]; then
    echo "✅ /dev/mqueue가 마운트되었습니다."
    ls -la /dev/mqueue
else
    echo "❌ /dev/mqueue가 마운트되지 않았습니다. 마운트 시도 중..."
    mkdir -p /dev/mqueue
    mount -t mqueue none /dev/mqueue
    if [ $? -eq 0 ]; then
        echo "✅ /dev/mqueue 마운트 성공"
        ls -la /dev/mqueue
    else
        echo "❌ /dev/mqueue 마운트 실패. privileged 모드가 필요할 수 있습니다."
    fi
fi

cd /workspace/Efficient-Fuzzer

# 파일 존재 확인
echo "=== 소스 파일 확인 ==="
if [ ! -f "/workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp" ]; then
    echo "❌ 오류: src/libFuzzer/cmd_dis_libfuzzer.cpp 파일이 존재하지 않습니다."
    echo "브랜치 Week13/temp이 올바르게 클론되었는지 확인하세요."
    echo "현재 디렉토리 구조:"
    find /workspace/Efficient-Fuzzer -type f -name "*.cpp" | sort
    exit 1
fi

if [ ! -f "/workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp" ]; then
    echo "❌ 오류: src/harness/CmdDispatcherHarness.cpp 파일이 존재하지 않습니다."
    echo "브랜치 Week13/temp이 올바르게 클론되었는지 확인하세요."
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
# Python을 사용하여 패키지 존재 확인 (파이프 사용하지 않음)
if python3 -c "import sys, importlib.util; sys.exit(0 if importlib.util.find_spec('fprime') is not None else 1)" 2>/dev/null; then
    echo "✅ fprime 패키지가 이미 설치되어 있습니다."
elif command -v fprime-fpp-to-xml > /dev/null 2>&1; then
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

# 모든 fprime 정적 라이브러리 링크 준비 (Stub 라이브러리 제외)
echo "=== 전체 라이브러리 목록 생성 (Stub 제외) ==="
declare -a ALL_LIBS=()
for lib in "${LIB_DIR}"/*.a; do
    base=$(basename "$lib")
    # Stub 라이브러리는 제외
    if [[ ! "$base" =~ _Stub\.a$ ]]; then
        ALL_LIBS+=("$lib")
    fi
done

# stringFormat 구현을 위한 Fw Types 소스
SNPRINTF_SRC=(/workspace/Efficient-Fuzzer/src/fprime/Fw/Types/snprintf_format.cpp)

# 명시적으로 필요한 시스템 라이브러리 지정
SYS_LIBS="-lpthread -ldl -lrt -lm -lstdc++ -lutil"

# 컴파일러 및 링커 플래그 설정
CXXFLAGS="-g -O1 -fsanitize=fuzzer,address -fsanitize-recover=all -std=c++14 -fvisibility=default -DOS_QUEUE_PLATFORM=generic"
LDFLAGS="-Wl,-z,defs -Wl,--no-as-needed"

# stub 코드 소스 추가
STUB_SOURCES=(/workspace/Efficient-Fuzzer/src/fprime/Os/Stub/*.cpp)

# 컴파일하기 전에 OS_QUEUE_PLATFORM 다시 확인
echo "=== 컴파일 전 환경 확인 ==="
echo "OS_QUEUE_PLATFORM=${OS_QUEUE_PLATFORM}"

# libFuzzer 컴파일 및 링크 (stub 코드 및 stringFormat 소스 포함)
echo "=== libFuzzer 컴파일 및 링크 ==="
clang++ ${CXXFLAGS} \
    ${INCLUDE_DIRS} \
    ${FPRIME_INCLUDES} \
    /workspace/Efficient-Fuzzer/src/libFuzzer/cmd_dis_libfuzzer.cpp \
    /workspace/Efficient-Fuzzer/src/harness/CmdDispatcherHarness.cpp \
    "${STUB_SOURCES[@]}" \
    "${SNPRINTF_SRC[@]}" \
    ${LDFLAGS} -Wl,--whole-archive ${ALL_LIBS[@]} -Wl,--no-whole-archive ${SYS_LIBS} \
    -o cmd_dispatcher_fuzzer

if [ $? -ne 0 ]; then
    echo "❌ 컴파일 또는 링크 실패. stub 코드를 포함했음에도 문제가 발생합니다."
    exit 1
fi

echo "✅ 컴파일 성공!"







# 코퍼스 디렉토리 생성
mkdir -p corpus

# 초기 코퍼스 생성
echo "=== 초기 코퍼스 생성 ==="
# 기본 테스트를 0회 실행하여 코퍼스만 생성
./cmd_dispatcher_fuzzer -runs=0 corpus

# libFuzzer를 백그라운드로 실행
echo "=== libFuzzer 실행 시작 ==="
mkdir -p findings
mkdir -p corpus_backup # 코퍼스 백업 디렉토리 추가

# 더 많은 디버깅 정보 출력 및 어서트 처리 설정
export ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:print_scariness=1:handle_abort=1:handle_sigill=1:handle_sigfpe=1:abort_on_error=0:start_deactivated=0:strict_string_checks=0"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0"

./cmd_dispatcher_fuzzer -max_len=1024 \
                       -artifact_prefix=findings/ \
                       -ignore_crashes=1 \
                       -ignore_timeouts=1 \
                       -ignore_ooms=1 \
                       -timeout=1 \
                       -error_exitcode=0 \
                       -rss_limit_mb=4096 \
                       -print_final_stats=1 \
                       -fork=1 \
                       -runs=-1 \
                       corpus &
FUZZER_PID=$!

echo "libFuzzer가 PID $FUZZER_PID로 백그라운드에서 실행 중입니다."
echo "결과는 findings/ 디렉토리에 저장됩니다."

# 정기적으로 상태 확인 및 백업하는 스크립트도 실행
(
while true; do
    if kill -0 $FUZZER_PID 2>/dev/null; then
        echo "$(date) - Fuzzer가 계속 실행 중입니다(PID: $FUZZER_PID)"
        
        # corpus 파일을 주기적으로 백업
        if [ -d "corpus" ] && [ "$(ls -A corpus 2>/dev/null)" ]; then
            echo "코퍼스 백업 중..."
            cp -r corpus/* corpus_backup/ 2>/dev/null || true
        fi
        
        # findings 확인
        if [ -d "findings" ] && [ "$(ls -A findings 2>/dev/null)" ]; then
            echo "발견된 이슈:"
            ls -la findings/
        fi
    else
        echo "$(date) - Fuzzer가 종료되었습니다. 재시작 시도..."
        
        # 퍼저 재시작
        ./cmd_dispatcher_fuzzer -max_len=1024 \
                              -artifact_prefix=findings/ \
                              -ignore_crashes=1 \
                              -ignore_timeouts=1 \
                              -ignore_ooms=1 \
                              -timeout=1 \
                              -error_exitcode=0 \
                              -rss_limit_mb=4096 \
                              -print_final_stats=1 \
                              -fork=1 \
                              -runs=-1 \
                              corpus &
        FUZZER_PID=$!
        echo "Fuzzer가 PID $FUZZER_PID로 재시작되었습니다."
    fi
    
    # 60초마다 확인
    sleep 60
done
) &
STATUS_CHECKER_PID=$!

# 컨테이너 유지를 위한 트랩 설정 (Ctrl+C가 컨테이너를 종료하지 않도록)
trap "echo 'Received SIGINT, but container will keep running. Fuzzer may have terminated.'" SIGINT
trap "echo 'Received SIGTERM, but container will keep running. Fuzzer may have terminated.'" SIGTERM

# 무한 대기
echo "=== 컨테이너 유지 모드 ==="
echo "컨테이너를 유지하기 위해 대기 중입니다. (Ctrl+C를 눌러도 종료되지 않습니다)"
echo "docker exec로 접속하여 결과를 확인하세요."
echo "종료하려면 docker stop 명령을 사용하세요."

while true; do
    sleep 3600 & wait $!
done 
