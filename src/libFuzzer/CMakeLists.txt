cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -Wall -Wextra -fsanitize=address -Wno-unused-parameter -Wno-inconsistent-missing-override")

# fprime 경로 설정
set(FPRIME_SRC_ROOT "${CMAKE_SOURCE_DIR}/../fprime")
set(FPRIME_UT_ROOT "${FPRIME_SRC_ROOT}/build-fprime-automatic-native-ut")
set(FPRIME_INC_LIST
    "${FPRIME_UT_ROOT}"
    "${FPRIME_UT_ROOT}/F-Prime"
    "${FPRIME_UT_ROOT}/config"
    "${FPRIME_SRC_ROOT}/config"
    "${FPRIME_SRC_ROOT}/Os/Generic"
    "${FPRIME_SRC_ROOT}/Os/Linux"
    "${FPRIME_SRC_ROOT}/Fw/Types"
    "${FPRIME_SRC_ROOT}"
    "${FPRIME_SRC_ROOT}/cmake/platform/types"
    "${FPRIME_SRC_ROOT}/
)

set(FPRIME_UT_LIB_DIR "${FPRIME_UT_ROOT}/lib/Linux")

# 링크 라이브러리 목록
set(FPRIME_LINK_LIBS_PATHS
    ${FPRIME_UT_LIB_DIR}/libFw_Cfg.a
    ${FPRIME_UT_LIB_DIR}/libFw_Cmd.a
    ${FPRIME_UT_LIB_DIR}/libFw_Com.a
    ${FPRIME_UT_LIB_DIR}/libFw_Comp.a
    ${FPRIME_UT_LIB_DIR}/libFw_CompQueued.a
    ${FPRIME_UT_LIB_DIR}/libFw_Log.a
    ${FPRIME_UT_LIB_DIR}/libFw_Logger.a
    ${FPRIME_UT_LIB_DIR}/libFw_Obj.a
    ${FPRIME_UT_LIB_DIR}/libFw_Port.a
    ${FPRIME_UT_LIB_DIR}/libFw_Prm.a
    ${FPRIME_UT_LIB_DIR}/libFw_Time.a
    ${FPRIME_UT_LIB_DIR}/libFw_Tlm.a
    ${FPRIME_UT_LIB_DIR}/libFw_Types.a
    ${FPRIME_UT_LIB_DIR}/libOs.a
    ${FPRIME_UT_LIB_DIR}/libOs_Console_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_Console_Posix.a
    ${FPRIME_UT_LIB_DIR}/libOs_Cpu_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_Cpu_Linux.a
    ${FPRIME_UT_LIB_DIR}/libOs_File_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_File_Posix.a
    ${FPRIME_UT_LIB_DIR}/libOs_Generic_PriorityQueue.a
    ${FPRIME_UT_LIB_DIR}/libOs_Generic_Types.a
    ${FPRIME_UT_LIB_DIR}/libOs_Memory_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_Memory_Linux.a
    ${FPRIME_UT_LIB_DIR}/libOs_Mutex_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_Mutex_Posix.a
    ${FPRIME_UT_LIB_DIR}/libOs_Posix_Shared.a
    ${FPRIME_UT_LIB_DIR}/libOs_Queue_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_RawTime_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_RawTime_Posix.a
    ${FPRIME_UT_LIB_DIR}/libOs_Task_Common.a
    ${FPRIME_UT_LIB_DIR}/libOs_Task_Posix.a
    ${FPRIME_UT_LIB_DIR}/libSvc_CmdDispatcher.a
    ${FPRIME_UT_LIB_DIR}/libSvc_Ping.a
    ${FPRIME_UT_LIB_DIR}/libUtils_Hash.a
    ${FPRIME_UT_LIB_DIR}/libconfig.a
    ${FPRIME_UT_LIB_DIR}/libsnprintf-format.a
)

# CmdDispatcher 하네스 소스 파일 정의
file(GLOB FPRIME_CMD_DISPATCHER_HARNESS_SRCS
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher/*.cpp"
)
list(REMOVE_ITEM FPRIME_CMD_DISPATCHER_HARNESS_SRCS
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher/CommandDispatcherGTestBase.cpp"
)

message(STATUS "Including fprime harness sources: ${FPRIME_CMD_DISPATCHER_HARNESS_SRCS}")

# 하네스 라이브러리
add_library(CmdDispatcherFuzzTester
    FuzzTester/CmdDispatcherFuzzTester.cpp
    ${FPRIME_CMD_DISPATCHER_HARNESS_SRCS}
)
target_include_directories(CmdDispatcherFuzzTester PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher"
    ${FPRIME_INC_LIST}
)
target_compile_definitions(CmdDispatcherFuzzTester PRIVATE
    BUILD_UT
    PROTECTED=public
    PRIVATE=public
    STATIC=
)

# fuzzer 메인 실행 파일
add_executable(cmd_fuzzer libFuzzer_main.cpp)
target_compile_options(cmd_fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(cmd_fuzzer PRIVATE -fsanitize=fuzzer)
target_include_directories(cmd_fuzzer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/FuzzTester"
    "${FPRIME_SRC_ROOT}/Fw/Types"
    ${FPRIME_INC_LIST}
)
target_link_libraries(cmd_fuzzer PRIVATE
    -Wl,--start-group
    CmdDispatcherFuzzTester
    ${FPRIME_LINK_LIBS_PATHS}
    -Wl,--end-group
    gcov
)

install(TARGETS cmd_fuzzer DESTINATION bin)