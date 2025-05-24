cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

# 컴파일 옵션 설정
set(CMAKE_CXX_STANDARD 11)
# 전역 CXX_FLAGS: -fsanitize=fuzzer 제거, -Wno-inconsistent-missing-override 추가
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -Wall -Wextra -fsanitize=address -Wno-unused-parameter -Wno-inconsistent-missing-override")

# fprime UT harness 전용 매크로 정의 (protected/private 접근 허용)
add_definitions(
    -DBUILD_UT
    -DPROTECTED=public
    -DPRIVATE=public
    -DSTATIC=
)

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

# FuzzTester 라이브러리 생성 (퍼저 하네스 + fprime auto-generated UT harness)
file(GLOB FPRIME_CMD_DISPATCHER_HARNESS_SRCS
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher/*.cpp"
)
list(REMOVE_ITEM FPRIME_CMD_DISPATCHER_HARNESS_SRCS
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher/CommandDispatcherGTestBase.cpp"
)
message(STATUS "Including fprime harness sources: ${FPRIME_CMD_DISPATCHER_HARNESS_SRCS}")
add_library(CmdDispatcherFuzzTester
    FuzzTester/CmdDispatcherFuzzTester.cpp
    ${FPRIME_CMD_DISPATCHER_HARNESS_SRCS}
)
# FuzzTester include 및 링크
target_include_directories(CmdDispatcherFuzzTester PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher"
    ${FPRIME_INC_LIST}
)

# fprime UT harness 매크로 오버라이드: PROTECTED, PRIVATE 공개
target_compile_definitions(CmdDispatcherFuzzTester PRIVATE
    BUILD_UT
    PROTECTED=public
    PRIVATE=public
    STATIC=
)

# snprintf_format 구현 추가
add_library(snprintf-format ${FPRIME_SRC_ROOT}/Fw/Types/snprintf_format.cpp)
target_include_directories(snprintf-format PUBLIC ${FPRIME_INC_LIST})

# Fprime 라이브러리 경로 명시적 설정
# FPRIME_UT_ROOT 는 이미 "${FPRIME_SRC_ROOT}/build-fprime-automatic-native-ut" 로 설정되어 있음

set(LIB_OS_A_PATH ${FPRIME_UT_ROOT}/lib/Linux/libOs.a)

set(FPRIME_LINK_LIBS_PATHS
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Memory_Linux.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Console_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_File_Posix.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Types.a
    ${FPRIME_UT_ROOT}/lib/Linux/libSvc_CmdDispatcher.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Generic_Types.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Console_Posix.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_CompQueued.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Mutex_Posix.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Cmd.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_RawTime_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Com.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Tlm.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Port.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Prm.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Generic_PriorityQueue.a
    ${FPRIME_UT_ROOT}/lib/Linux/libSvc_Ping.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Buffer.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Posix_Shared.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Comp.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Cfg.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Obj.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_File_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Memory_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Queue_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Log.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_RawTime_Posix.a
    ${FPRIME_UT_ROOT}/lib/Linux/libconfig.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Time.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Mutex_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Task_Common.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Task_Posix.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Cpu_Linux.a
    ${FPRIME_UT_ROOT}/lib/Linux/libUtils_Hash.a
    ${FPRIME_UT_ROOT}/lib/Linux/libFw_Logger.a
    ${FPRIME_UT_ROOT}/lib/Linux/libOs_Cpu_Common.a
)
message(STATUS "FPrime Lib OS Path: ${LIB_OS_A_PATH}")
message(STATUS "FPrime Link Libraries: ${FPRIME_LINK_LIBS_PATHS}")

# CmdDispatcherFuzzTester 라이브러리 링크 수정 (Fprime 의존성은 cmd_fuzzer에서 처리)
target_link_libraries(CmdDispatcherFuzzTester PUBLIC
    gcov
    snprintf-format
)

# Fprime OS Stub 구현 라이브러리 생성
file(GLOB FPRIME_OS_STUB_SRCS
    "${FPRIME_SRC_ROOT}/Os/Stub/*.cpp"
)
add_library(OsStubImplementation ${FPRIME_OS_STUB_SRCS})
target_include_directories(OsStubImplementation PUBLIC
    "${FPRIME_SRC_ROOT}/Os/Stub"
    ${FPRIME_INC_LIST}
)
target_compile_definitions(OsStubImplementation PRIVATE
    BUILD_UT
    PROTECTED=public
    PRIVATE=public
    STATIC=
)

# 실행 파일 생성
add_executable(cmd_fuzzer
    libFuzzer_main.cpp
)
# cmd_fuzzer에만 -fsanitize=fuzzer 옵션 추가
target_compile_options(cmd_fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(cmd_fuzzer PRIVATE -fsanitize=fuzzer)

# cmd_fuzzer include 및 링크
target_include_directories(cmd_fuzzer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    ${FPRIME_INC_LIST}
)

# OS 인터페이스 문제 해결을 위한 링킹 순서 중요: libOs.a를 먼저 링크
target_link_libraries(cmd_fuzzer PRIVATE
    CmdDispatcherFuzzTester
    -Wl,--start-group
    ${FPRIME_NATIVE_LIB_DIR}/libOs.a                 # POSIX 통합 라이브러리
    ${FPRIME_NATIVE_LIB_DIR}/libOs_Generic_PriorityQueue.a
    ${FPRIME_NATIVE_LIB_DIR}/libOs_Posix_Mutex.a
    ${FPRIME_NATIVE_LIB_DIR}/libOs_Posix_Task.a
    ${FPRIME_NATIVE_LIB_DIR}/libSvc_CmdDispatcher.a
    ${FPRIME_LINK_LIBS_PATHS}
    -Wl,--end-group
    gcov
)

# TestNominal 실행 파일 생성
add_executable(TestNominal
    FuzzTester/TestNominal.cpp
)
# TestNominal include 및 링크
target_include_directories(TestNominal PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    ${FPRIME_INC_LIST}
)
# TestNominal은 fuzzer sanitize 옵션 없이 빌드 (기존 설정 유지)
set_target_properties(TestNominal PROPERTIES CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -Wall -Wextra -fsanitize=address")

target_link_libraries(TestNominal PRIVATE
    CmdDispatcherFuzzTester
    -Wl,--start-group
    ${FPRIME_NATIVE_LIB_DIR}/libOs.a                 # POSIX 통합 라이브러리
    ${FPRIME_NATIVE_LIB_DIR}/libOs_Generic_PriorityQueue.a
    ${FPRIME_NATIVE_LIB_DIR}/libOs_Posix_Mutex.a
    ${FPRIME_NATIVE_LIB_DIR}/libOs_Posix_Task.a
    ${FPRIME_NATIVE_LIB_DIR}/libSvc_CmdDispatcher.a
    ${FPRIME_LINK_LIBS_PATHS}
    -Wl,--end-group
    gcov
)

# 설치 설정
install(TARGETS TestNominal DESTINATION bin)
install(TARGETS cmd_fuzzer DESTINATION bin)

