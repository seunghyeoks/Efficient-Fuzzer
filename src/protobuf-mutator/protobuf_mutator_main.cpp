#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "../FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <fstream> // 파일 출력을 위해 추가
#include <string>  // std::string 사용을 위해 추가
#include <sstream> // 문자열 스트림 사용을 위해 추가
#include <cstdlib>
#include "cmd_dispatcher_fuzz_input.pb.h" // protobuf 헤더

// libprotobuf-mutator용 진입점
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"

DEFINE_PROTO_FUZZER(const Svc::CmdDispatcherFuzzInput& fuzz_input) {
    // protobuf 메시지를 바이트 배열로 변환
    std::string serialized;
    fuzz_input.SerializeToString(&serialized);
    const uint8_t* Data = reinterpret_cast<const uint8_t*>(serialized.data());
    size_t Size = serialized.size();

    NATIVE_INT_TYPE queueDepth = 10;
    NATIVE_INT_TYPE instance = 0;
    if (Size >= 2) {
        queueDepth = 4 + (Data[0] % 28); // 4~32 범위
        instance = Data[1];
    }
    Svc::CmdDispatcherFuzzTester tester;
    tester.initWithFuzzParams(queueDepth, instance);
    tester.connectPorts();
    tester.tryTest(Data, Size);
}
