cmake_minimum_required(VERSION 3.13)
project(FPrimeCmdDispatcherFuzzer)

# 컴파일 옵션 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fsanitize=fuzzer,address")

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

# 모든 .a 파일 자동 링크 (자동 링크 코드 제거)
# 아래에 명시적으로 라이브러리 추가
# file(GLOB FPRIME_STATIC_LIBS
# "${FPRIME_SRC_ROOT}/build-fprime-automatic-native-ut/lib/Linux/*.a"
# "${FPRIME_SRC_ROOT}/build-fprime-automatic-native/lib/Linux/*.a"
# )
# message(STATUS "Linking all fprime static libs: ${FPRIME_STATIC_LIBS}")

# ut_list.txt에서 FPrime 라이브러리 목록 읽기
set(UT_LIST_FILE "${CMAKE_SOURCE_DIR}/ut_list.txt")
if(NOT EXISTS ${UT_LIST_FILE})
    message(FATAL_ERROR "ut_list.txt not found at ${UT_LIST_FILE}")
endif()
file(STRINGS ${UT_LIST_FILE} UT_RAW_LIBRARIES)

set(OS_LIB_PATH "")
set(FPRIME_LINK_LIBS "")

foreach(lib_path ${UT_RAW_LIBRARIES})
    if(lib_path MATCHES ".*/libOs\\\\.a$") # Escaped dot for regex
        set(OS_LIB_PATH ${lib_path})
    elseif(NOT (lib_path MATCHES ".*libgtest.*" OR lib_path MATCHES ".*libsnprintf-format\\\\.a$")) # Escaped dot
        list(APPEND FPRIME_LINK_LIBS ${lib_path})
    endif()
endforeach()

if(NOT OS_LIB_PATH)
    message(WARNING "libOs.a not found in ut_list.txt. Linker errors for Os::*Interface::getDelegate might persist if OS_LIB_PATH is not correctly identified from ut_list.txt or manually set.")
else()
    message(STATUS "Found OS_LIB_PATH: ${OS_LIB_PATH}")
endif()

list(REMOVE_DUPLICATES FPRIME_LINK_LIBS)
message(STATUS "Linking FPrime libraries (from ut_list.txt, excluding gtest, libOs.a, libsnprintf-format.a): ${FPRIME_LINK_LIBS}")

# CmdDispatcherFuzzTester 라이브러리 링크 수정 (Fprime 의존성은 cmd_fuzzer에서 처리)
target_link_libraries(CmdDispatcherFuzzTester PUBLIC
    gcov
    snprintf-format
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

# OS 인터페이스 문제 해결을 위한 링킹 순서 중요: libOs.a를 먼저 링크
target_link_libraries(cmd_fuzzer PRIVATE
    CmdDispatcherFuzzTester
    -Wl,--start-group
    ${OS_LIB_PATH}          # ut_list.txt에서 찾은 libOs.a 경로 사용
    ${FPRIME_LINK_LIBS}    # 나머지 FPrime 라이브러리 (gtest, libsnprintf-format.a 제외)
    -Wl,--end-group
    gcov
    snprintf-format
)

# 설치 설정
install(TARGETS cmd_fuzzer DESTINATION bin)
