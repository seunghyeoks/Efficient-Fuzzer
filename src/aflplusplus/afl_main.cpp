#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "../FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    const std::string input_file = argv[1];
    std::ifstream ifs(input_file, std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open input file: " << input_file << std::endl;
        return 1;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());

    NATIVE_INT_TYPE queueDepth = 10;
    NATIVE_INT_TYPE instance = 0;
    if (Size >= 2) {
        queueDepth = 4 + (Data[0] % 28); // 4~32 범위
        instance = Data[1];
    }
    // Fuzzing을 위한 테스트 하네스 Svc::CmdDispatcherFuzzTester 객체를 생성합니다.
    Svc::CmdDispatcherFuzzTester tester;
    // Fuzz 테스터를 퍼저 입력 기반 파라미터로 초기화합니다.
    tester.initWithFuzzParams(queueDepth, instance);
    // Fuzz 테스터와 CommandDispatcherImpl 컴포넌트 간의 포트를 연결합니다.
    tester.connectPorts();
    // CommandDispatcherImpl 컴포넌트에 내장된 기본 명령어들을 등록합니다.
    tester.tryTest(Data, Size);

    return 0;
} 