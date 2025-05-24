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
    "${FPRIME_UT_ROOT}/config"
)

# 글로벌 include 설정
include_directories(
    ${FPRIME_INC_LIST}
)

# fprime 라이브러리 경로 설정: UT build root 사용
link_directories(${FPRIME_UT_ROOT}/lib/Linux)

# Os 라이브러리 경로
set(LIB_OS_A_PATH ${FPRIME_UT_ROOT}/lib/Linux/libOs.a)

# FPRIME 링크 라이브러리 목록 (UT)
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

# OSAL 구현체 소스 추가 (Generic PriorityQueue 및 DefaultPriorityQueue, Posix Mutex/Task)
target_sources(CmdDispatcherFuzzTester PRIVATE
    "${FPRIME_SRC_ROOT}/Os/Generic/DefaultPriorityQueue.cpp"
    "${FPRIME_SRC_ROOT}/Os/Generic/PriorityQueue.cpp"
    "${FPRIME_SRC_ROOT}/Os/Posix/Mutex.cpp"
    "${FPRIME_SRC_ROOT}/Os/Posix/Task.cpp"
)
# OSAL 구현체 헤더 include 경로 추가
target_include_directories(CmdDispatcherFuzzTester PUBLIC
    "${FPRIME_SRC_ROOT}/Os/Generic"
    "${FPRIME_SRC_ROOT}/Os/Posix"
)

# snprintf_format 구현 추가
add_library(snprintf-format ${FPRIME_SRC_ROOT}/Fw/Types/snprintf_format.cpp)
target_include_directories(snprintf-format PUBLIC ${FPRIME_INC_LIST})

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

# cmd_fuzzer 링크 수정: UT 라이브러리만 사용
target_link_libraries(cmd_fuzzer PRIVATE
    CmdDispatcherFuzzTester
    -Wl,--start-group
    ${LIB_OS_A_PATH}
    ${FPRIME_LINK_LIBS_PATHS}
    -Wl,--end-group
    gcov
    snprintf-format
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
# fuzzer sanitize 옵션 없이 빌드
set_target_properties(TestNominal PROPERTIES CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -Wall -Wextra -fsanitize=address")

# TestNominal 링크 수정: UT 라이브러리만 사용
target_link_libraries(TestNominal PRIVATE
    CmdDispatcherFuzzTester
    -Wl,--start-group
    ${LIB_OS_A_PATH}
    ${FPRIME_LINK_LIBS_PATHS}
    -Wl,--end-group
    gcov
    snprintf-format
)

# 설치 설정
install(TARGETS cmd_fuzzer TestNominal DESTINATION bin)

