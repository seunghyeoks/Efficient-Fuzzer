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
)

# FuzzTester 라이브러리 생성
add_library(CmdDispatcherFuzzTester
    FuzzTester/CmdDispatcherFuzzTester.cpp
)
# FuzzTester include 및 링크
target_include_directories(CmdDispatcherFuzzTester PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    ${FPRIME_INC_LIST}
)
target_link_libraries(CmdDispatcherFuzzTester PUBLIC
    Svc_CmdDispatcher
    Fw_Log
    Fw_Comp
    Fw_Tlm
    Fw_Com
    Fw_Time
    Fw_Port
    Fw_Types
    Fw_Cfg
    Fw_Cmd
    Svc_Ping
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