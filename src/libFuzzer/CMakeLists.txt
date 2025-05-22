cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

# 컴파일 옵션 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 퍼저 빌드 설정
set(FUZZER_FLAGS "-fsanitize=fuzzer,address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZER_FLAGS}")

# 서브 디렉토리 추가
add_subdirectory(FuzzTester)

# 헤더 파일 디렉토리 추가
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}                    # libFuzzer 프로젝트 루트
    ${CMAKE_SOURCE_DIR}                            # libFuzzer 포함 경로
    ${CMAKE_SOURCE_DIR}/../fprime                   # fprime 소스 및 헤더 루트 (Fw/Types 등)
    ${CMAKE_SOURCE_DIR}/../fprime/config            # fprime 설정 헤더 (FpConfig.hpp 등)
    ${CMAKE_SOURCE_DIR}/../fprime/build-fprime-automatic-native-ut  # fprime UT 자동 생성 헤더
    ${CMAKE_SOURCE_DIR}/../fprime/cmake/platform/types # PlatformTypes.h
)

# 소스 파일 목록
set(SOURCE_FILES
    libFuzzer_main.cpp
)

# 실행 파일 생성
add_executable(cmd_fuzzer ${SOURCE_FILES})

# 의존성 설정
target_link_libraries(cmd_fuzzer
    CmdDispatcherFuzzTester
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

# 설치 대상 설정
install(TARGETS cmd_fuzzer DESTINATION bin) 