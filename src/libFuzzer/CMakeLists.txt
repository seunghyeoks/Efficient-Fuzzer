cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

# 컴파일 옵션 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 퍼저 빌드 설정
set(FUZZER_FLAGS "-fsanitize=fuzzer,address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZER_FLAGS}")

### FuzzTester 라이브러리 통합 빌드 ###
# FuzzTester 경로에서 헬퍼 소스 수집
set(TESTER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester/CmdDispatcherFuzzTester.cpp")
# FuzzTester 라이브러리 생성
add_library(CmdDispatcherFuzzTester ${TESTER_SRC})
# FuzzTester 라이브러리 의존성
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

### fprime 하위 모든 헤더 폴더 자동 include 설정 ###
# fprime 소스 루트 경로
set(FPRIME_SRC_ROOT "${CMAKE_SOURCE_DIR}/../fprime")
# 재귀적으로 디렉토리 탐색
file(GLOB_RECURSE FPRIME_FOLDERS
    RELATIVE "${FPRIME_SRC_ROOT}"
    "${FPRIME_SRC_ROOT}/*"
)
# include 경로 리스트 초기화 (libFuzzer 프로젝트 루트, fprime 루트, config, platform types 포함)
set(FPRIME_INCLUDE_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${FPRIME_SRC_ROOT}"
    "${FPRIME_SRC_ROOT}/config"  # FpConfig.hpp
    "${FPRIME_SRC_ROOT}/cmake/platform/types"  # PlatformTypes.h
)
# 디렉토리만 append
foreach(f ${FPRIME_FOLDERS})
  if(IS_DIRECTORY "${FPRIME_SRC_ROOT}/${f}")
    list(APPEND FPRIME_INCLUDE_PATHS "${FPRIME_SRC_ROOT}/${f}")
  endif()
endforeach()

# fprime UT 자동 생성 헤더 경로
set(FPRIME_UT_ROOT "${FPRIME_SRC_ROOT}/build-fprime-automatic-native-ut")
file(GLOB_RECURSE FPRIME_UT_FOLDERS
    RELATIVE "${FPRIME_UT_ROOT}"
    "${FPRIME_UT_ROOT}/*"
)
list(APPEND FPRIME_INCLUDE_PATHS "${FPRIME_UT_ROOT}")
foreach(f ${FPRIME_UT_FOLDERS})
  if(IS_DIRECTORY "${FPRIME_UT_ROOT}/${f}")
    list(APPEND FPRIME_INCLUDE_PATHS "${FPRIME_UT_ROOT}/${f}")
  endif()
endforeach()

# FuzzTester 헤더 포함
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    ${FPRIME_INCLUDE_PATHS}
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