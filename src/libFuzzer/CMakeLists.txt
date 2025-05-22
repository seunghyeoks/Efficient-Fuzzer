cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

# 컴파일 옵션 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fsanitize=fuzzer,address")

# fprime include 설정
set(FPRIME_SRC_ROOT "${CMAKE_SOURCE_DIR}/../fprime")
set(FPRIME_UT_ROOT "${FPRIME_SRC_ROOT}/build-fprime-automatic-native-ut")
set(FPRIME_INC_LIST
    "${FPRIME_SRC_ROOT}"
    "${FPRIME_SRC_ROOT}/config"
    "${FPRIME_SRC_ROOT}/cmake/platform/types"
    "${FPRIME_UT_ROOT}"
    "${FPRIME_UT_ROOT}/F-Prime"
    "${FPRIME_UT_ROOT}/config"  # UT config 헤더 (FppConstantsAc.hpp 등)
)

# 글로벌 include 설정: fprime 주요 헤더 경로
include_directories(
    ${FPRIME_INC_LIST}
)

# fprime 라이브러리 경로 명시적으로 추가
link_directories(/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/lib/Linux)

# fprime 라이브러리 경로 자동 탐색
file(GLOB_RECURSE FPRIME_LIB_DIRS
    "${FPRIME_SRC_ROOT}/build*/lib"
    "${FPRIME_SRC_ROOT}/build*/F-Prime/lib"
    "${FPRIME_SRC_ROOT}/build*/Svc/CmdDispatcher"
    "${FPRIME_SRC_ROOT}/build*/Fw/*"
)
list(REMOVE_DUPLICATES FPRIME_LIB_DIRS)
foreach(libdir ${FPRIME_LIB_DIRS})
    if(IS_DIRECTORY ${libdir})
        link_directories(${libdir})
    endif()
endforeach()

# FuzzTester 라이브러리 생성
add_library(CmdDispatcherFuzzTester
    FuzzTester/CmdDispatcherFuzzTester.cpp
)
# FuzzTester include 및 링크
target_include_directories(CmdDispatcherFuzzTester PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    ${FPRIME_INC_LIST}
)

# 모든 .a 파일 자동 링크 (자동 링크 코드 제거)
# 아래에 명시적으로 라이브러리 추가

target_link_libraries(CmdDispatcherFuzzTester PUBLIC
    Os_Memory_Linux
    Os_Console_Common
    Os_File_Posix
    Fw_Types
    Svc_CmdDispatcher
    Os_Generic_Types
    Os_Console_Posix
    Fw_CompQueued
    Os_Mutex_Posix
    Fw_Cmd
    Os_RawTime_Common
    Fw_Com
    Fw_Tlm
    Fw_Port
    Fw_Prm
    Os_Generic_PriorityQueue
    Svc_Ping
    Fw_Buffer
    Os_Posix_Shared
    Fw_Comp
    Fw_Cfg
    Fw_Obj
    Os_File_Common
    snprintf-format
    Os_Memory_Common
    Os_Queue_Common
    Fw_Log
    Os_RawTime_Posix
    config
    Fw_Time
    Os_Mutex_Common
    Os_Task_Common
    Os_Task_Posix
    Os_Cpu_Linux
    Os
    Utils_Hash
    Fw_Logger
    Os_Cpu_Common
    gcov
)

# 실행 파일 생성
add_executable(cmd_fuzzer
    libFuzzer_main.cpp
)
# cmd_fuzzer include 및 링크
target_include_directories(cmd_fuzzer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    ${FPRIME_INC_LIST}
)
target_link_libraries(cmd_fuzzer PRIVATE
    CmdDispatcherFuzzTester
)

# 설치 설정
install(TARGETS cmd_fuzzer DESTINATION bin) 