#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
// #include <cstdio> // fprintf를 위해 추가했었으나, 이제 사용하지 않으므로 주석 처리 또는 삭제 가능

// 특수한 입력 패턴을 생성하는 함수들 (현재는 LLVMFuzzerTestOneInput에서 직접 사용되지 않음)
Fw::ComBuffer createValidCommandBuffer(FwOpcodeType opcode, U32 cmdSeq) {
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    buff.serialize(opcode);
    buff.serialize(cmdSeq);
    return buff;
}

Fw::ComBuffer createInvalidSizeCommandBuffer(const uint8_t* data, size_t size) {
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    size_t copySize = std::min(size, static_cast<size_t>(sizeof(FwOpcodeType)-1));
    if (copySize > 0) {
        buff.serialize(data, copySize);
    }
    return buff;
}

// libFuzzer의 메인 입력 처리 함수
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {

    // Fuzzing 대상 컴포넌트인 Svc::CommandDispatcherImpl 객체를 생성합니다.
    // "CmdDispImpl"은 컴포넌트 인스턴스 이름입니다.
    Svc::CommandDispatcherImpl impl("CmdDispImpl");
    // CommandDispatcherImpl 컴포넌트를 초기화합니다.
    // 첫 번째 인자는 명령어 큐의 깊이, 두 번째 인자는 인스턴스 ID (보통 0)입니다.
    impl.init(10, 0); 

    // Fuzzing을 위한 테스트 하네스 Svc::CmdDispatcherFuzzTester 객체를 생성합니다.
    // 이 객체는 CommandDispatcherImpl 컴포넌트와 상호작용합니다.
    Svc::CmdDispatcherFuzzTester tester(impl);
    // Fuzz 테스터를 초기화합니다.
    tester.init();
    // Fuzz 테스터와 CommandDispatcherImpl 컴포넌트 간의 포트를 연결합니다.
    tester.connectPorts();

    // CommandDispatcherImpl 컴포넌트에 내장된 기본 명령어들을 등록합니다.
    // (예: NoOp, Health Ping 등)
    impl.regCommands();


    // Fuzzer로부터 받은 입력 데이터(Size)가 너무 작으면 (1바이트 미만) 처리를 건너뜁니다.
    if (Size < 1) {
        return 0; 
    }

    // F' 프레임워크에서 사용하는 명령어 버퍼(Fw::ComBuffer)를 생성합니다.
    Fw::ComBuffer buff;
    // 버퍼의 시작 부분에 이 데이터가 명령어 패킷임을 나타내는 디스크립터를 직렬화합니다.
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    // 실제 퍼저 입력 데이터를 직렬화할 수 있는 최대 크기를 계산합니다.
    const size_t cap = buff.getBuffCapacity() - sizeof(FwPacketDescriptorType);
    // 퍼저 입력(Data)을 버퍼 용량에 맞게 잘라서(len) 직렬화합니다.
    const size_t len = (Size > cap ? cap : Size);
    buff.serialize(Data, len);

    // 매 입력마다 Fuzz 테스터의 내부 상태를 초기화합니다.
    // 이를 통해 각 퍼즈 입력이 독립적으로 테스트되도록 보장합니다.
    tester.resetState();

/*

    // 준비된 명령어 버퍼(buff)를 Fuzz 테스터를 통해 CommandDispatcherImpl로 전송합니다.
    // 0은 포트 번호, context는 명령어 컨텍스트 (여기서는 0)입니다.
    tester.public_invoke_to_seqCmdBuff(0, buff, 0); 

    // Fuzz 테스터를 통해 CommandDispatcherImpl의 메시지 큐를 처리하도록 합니다.
    // 이 과정에서 실제로 명령어가 디스패치되고 실행됩니다.
    tester.public_doDispatchLoop();

    // Fuzz 테스터로부터 명령어 처리 결과를 가져옵니다.
    const auto& result = tester.getFuzzResult();

    // 만약 Fuzz 테스터가 명령어 처리 중 오류를 감지했다면, 관련 정보를 출력할 수 있습니다.
    // (현재는 에러 로그 출력이 주석 처리되어 있습니다)
    if (result.hasError) {
        // fprintf(stderr, "[FuzzError] Input size: %zu (clamped to %zu), LastOpcode: 0x%X, LastResponse: %d\n",
        //         Size, len, result.lastOpcode, result.lastResponse.e);
        // fflush(stderr);
        // Fuzzer가 에러를 감지하도록 하기 위해, 필요시 여기서 abort() 또는 FW_ASSERT(false) 호출
        // 예: if (result.lastResponse != Fw::CmdResponse::OK) FW_ASSERT(0, result.lastOpcode, result.lastResponse.e);
    }
*/
    return 0;
}
