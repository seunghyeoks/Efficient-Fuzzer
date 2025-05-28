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

// libFuzzer의 메인 입력 처리 함수
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    // queueDepth와 instance 값을 퍼저 입력에서 일부 랜덤하게 추출
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
