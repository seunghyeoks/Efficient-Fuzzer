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
link_directories(/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native/lib/Linux)

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

# 모든 .a 파일 자동 링크
file(GLOB FPRIME_ALL_LIBS
    "/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/lib/Linux/lib*.a"
    "/workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native/lib/Linux/lib*.a"
)
set(FPRIME_LIB_NAMES "")
foreach(libfile ${FPRIME_ALL_LIBS})
    get_filename_component(libname ${libfile} NAME_WE)
    string(REPLACE "lib" "" libname ${libname})
    list(APPEND FPRIME_LIB_NAMES ${libname})
endforeach()
target_link_libraries(CmdDispatcherFuzzTester PUBLIC
    Svc_CmdDispatcher_UT
    Svc_CmdDispatcher
    Fw_CompQueued
    Fw_Comp
    Fw_Tlm
    Fw_Com
    Fw_Time
    Fw_Port
    Fw_Types
    Fw_Cfg
    Fw_Cmd
    Fw_Log
    Svc_Ping
    Os
    Os_Mutex_Posix
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