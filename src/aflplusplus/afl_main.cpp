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

    // CommandDispatcherImpl 객체 생성 및 초기화
    Svc::CommandDispatcherImpl impl("CmdDispImpl");
    impl.init(10, 0);

    // FuzzTester 초기화 및 포트 연결
    Svc::CmdDispatcherFuzzTester tester(impl);
    tester.init();
    tester.connectPorts();

    // 데이터로 테스트 수행
    tester.tryTest(data.data(), data.size());

    return 0;
} 