cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

set(CMAKE_CXX_STANDARD 11)
# protobuf-mutator는 C++17 이상을 권장하지만, F' 호환성을 위해 우선 11 유지, 필요시 상향
# set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -Wall -Wextra -fsanitize=address -Wno-unused-parameter -Wno-inconsistent-missing-override")
# libprotobuf-mutator 사용 시 C++17 활성화 (선택적, F' 코드와 호환성 확인 필요)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Protobuf 설정
find_package(Protobuf REQUIRED)
message(STATUS "Found Protobuf: ${Protobuf_LIBRARIES}")
# libprotobuf-mutator 설정
find_package(libprotobuf-mutator CONFIG REQUIRED)
message(STATUS "Found libprotobuf-mutator: ${libprotobuf-mutator_LIBRARIES}")

# Proto 파일 경로
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmd_dispatcher_fuzz_input.proto")
# 생성될 C++ 파일 경로
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/cmd_dispatcher_fuzz_input.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/cmd_dispatcher_fuzz_input.pb.h")

# Protobuf 컴파일 규칙
add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE} --experimental_allow_proto3_optional --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating C++ sources from ${PROTO_FILE}"
)

# 생성된 Protobuf 소스를 포함할 라이브러리 (선택적, 직접 실행 파일에 추가 가능)
add_library(fuzz_proto_lib OBJECT ${PROTO_SRCS})
target_compile_options(fuzz_proto_lib INTERFACE -Wno-zero-as-null-pointer-constant -Wno-shorten-64-to-32 -Wno-unused-parameter -Wno-unused-variable)
target_include_directories(fuzz_proto_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

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

# Custom compiled F' sources to resolve specific linking issues

# For Fw::stringFormat
add_library(custom_snprintf_format STATIC
    ${FPRIME_SRC_ROOT}/Fw/Types/snprintf_format.cpp
)
target_include_directories(custom_snprintf_format PUBLIC ${FPRIME_INC_LIST})
# snprintf_format.cpp는 일반적으로 특별한 컴파일 정의가 많이 필요하지 않지만,
# F' 전역 설정을 따르기 위해 BUILD_UT 등을 고려할 수 있습니다.
target_compile_definitions(custom_snprintf_format PRIVATE BUILD_UT)


# For Os::QueueInterface::getDelegate (assuming DefaultPriorityQueue provides it)
# and other Os delegate functions for Posix
add_library(custom_OsImplementations STATIC
    ${FPRIME_SRC_ROOT}/Os/Generic/DefaultPriorityQueue.cpp
    ${FPRIME_SRC_ROOT}/Os/Posix/DefaultMutex.cpp # For Mutex and ConditionVariable delegates
    ${FPRIME_SRC_ROOT}/Os/Posix/DefaultTask.cpp  # For Task delegate
)
target_include_directories(custom_OsImplementations PUBLIC ${FPRIME_INC_LIST})
target_compile_definitions(custom_OsImplementations PRIVATE
    BUILD_UT
    PROTECTED=public
    PRIVATE=public
    STATIC=
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
    ../FuzzTester/CmdDispatcherFuzzTester.cpp
    ${FPRIME_CMD_DISPATCHER_HARNESS_SRCS}
)
target_include_directories(CmdDispatcherFuzzTester PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../FuzzTester"
    "${FPRIME_UT_ROOT}/F-Prime/Svc/CmdDispatcher"
    ${FPRIME_INC_LIST}
)
target_compile_definitions(CmdDispatcherFuzzTester PRIVATE
    BUILD_UT
    PROTECTED=public
    PRIVATE=public
    STATIC=
)

# CmdDispatcherFuzzTester 라이브러리가 사용하는 F' 라이브러리들을 링크합니다.
# target_link_libraries(CmdDispatcherFuzzTester PRIVATE
#     -Wl,--start-group
#     ${FPRIME_LINK_LIBS_PATHS}
#     -Wl,--end-group
# ) # 이 부분을 주석 처리 또는 삭제합니다.


if(EXISTS "/usr/local/lib/libprotobuf-mutator-libfuzzer.a")
    message(STATUS "libprotobuf-mutator-libfuzzer.a exists!")
    execute_process(
        COMMAND nm /usr/local/lib/libprotobuf-mutator-libfuzzer.a
        COMMAND grep CustomProtoMutator
        RESULT_VARIABLE NM_RESULT
        OUTPUT_VARIABLE NM_OUTPUT
        ERROR_VARIABLE NM_ERROR
    )
    message(STATUS "nm output: ${NM_OUTPUT}")
else()
    message(FATAL_ERROR "libprotobuf-mutator-libfuzzer.a NOT FOUND!")
endif()


# fuzzer 메인 실행 파일
find_library(LIBPROTOBUF_MUTATOR NAMES protobuf-mutator HINTS /usr/local/lib /usr/lib)
if(NOT LIBPROTOBUF_MUTATOR)
    message(FATAL_ERROR "libprotobuf-mutator not found!")
else()
    message(STATUS "Found libprotobuf-mutator: ${LIBPROTOBUF_MUTATOR}")
endif()

add_executable(proto_fuzzer protobuf_mutator_main.cpp)
target_compile_options(proto_fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(proto_fuzzer PRIVATE -fsanitize=fuzzer)

target_include_directories(proto_fuzzer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../FuzzTester"
    "${FPRIME_SRC_ROOT}/Fw/Types"
    ${FPRIME_INC_LIST}
    ${CMAKE_CURRENT_BINARY_DIR}
    /usr/local/include/libprotobuf-mutator
    /usr/local/include/libprotobuf-mutator/src
)

target_link_libraries(proto_fuzzer PRIVATE
    -Wl,--start-group
    custom_snprintf_format
    custom_OsImplementations
    ${FPRIME_UT_LIB_DIR}/libsnprintf-format.a
    ${FPRIME_UT_LIB_DIR}/libOs_Mutex_Posix.a
    ${FPRIME_UT_LIB_DIR}/libOs_Task_Posix.a
    CmdDispatcherFuzzTester
    fuzz_proto_lib
    ${FPRIME_LINK_LIBS_PATHS}
    -Wl,--end-group
    gcov
    ${Protobuf_LIBRARIES}
    ${LIBPROTOBUF_MUTATOR}
    /usr/local/lib/libprotobuf-mutator.a
    /usr/local/lib/libprotobuf-mutator-libfuzzer.a
)

install(TARGETS proto_fuzzer DESTINATION bin)