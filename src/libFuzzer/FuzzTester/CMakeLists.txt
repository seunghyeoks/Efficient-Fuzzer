cmake_minimum_required(VERSION 3.13)
project(CmdDispatcherFuzzer)

# 컴파일 옵션 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 헤더 파일 디렉토리 추가
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}                    # FuzzTester 소스 경로
    ${CMAKE_SOURCE_DIR}                            # libFuzzer 상위 경로
    ${CMAKE_SOURCE_DIR}/../fprime/build-fprime-automatic-native-ut  # fprime UT 자동 생성 헤더
    ${CMAKE_SOURCE_DIR}/../fprime/config                         # fprime 설정 헤더 (FpConfig.hpp 등)
)

# 소스 파일 목록
set(SOURCE_FILES
    CmdDispatcherFuzzTester.cpp
)

# 라이브러리 생성
add_library(CmdDispatcherFuzzTester ${SOURCE_FILES})

# 의존성 설정
target_link_libraries(CmdDispatcherFuzzTester
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
install(TARGETS CmdDispatcherFuzzTester DESTINATION lib)
install(FILES CmdDispatcherFuzzTester.hpp DESTINATION include/libFuzzer/FuzzTester) 