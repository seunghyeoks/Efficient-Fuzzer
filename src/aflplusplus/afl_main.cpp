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

    // 입력 파일에서 queueDepth와 instance 값을 추출
    NATIVE_INT_TYPE queueDepth = 10;
    NATIVE_INT_TYPE instance = 0;
    size_t offset = 0;
    if (data.size() >= 2) {
        queueDepth = 4 + (data[0] % 32); // 1~32 범위
        instance = data[1];
        offset = 2;
    }
    
    // FuzzTester 초기화 및 포트 연결
    Svc::CmdDispatcherFuzzTester tester;
    tester.initWithFuzzParams(queueDepth, instance);
    tester.connectPorts();
    // 데이터로 테스트 수행 (앞의 2바이트는 파라미터로 사용했으니 제외)
    tester.tryTest(data.data() + offset, data.size() - offset);

    return 0;
} 