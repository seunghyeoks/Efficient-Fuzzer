#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

// 퍼징 상태 관리
static Svc::CmdDispatcherFuzzTester* tester = nullptr;

// 특수한 입력 패턴을 생성하는 함수들
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
    // 최소 필요 크기보다 작게 직렬화
    size_t copySize = std::min(size, static_cast<size_t>(sizeof(FwOpcodeType)-1));
    if (copySize > 0) {
        buff.serialize(data, copySize);
    }
    return buff;
}

// libFuzzer 엔트리 포인트
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size < 1) return 0;

    // 매 입력마다 새로 생성 (상태 오염 방지)
    Svc::CommandDispatcherImpl impl("CmdDispImpl");
    impl.init(10, 0);

    Svc::CmdDispatcherFuzzTester tester(impl);
    tester.init();
    tester.connectPorts();
    tester.registerCommands(10, 0x100);

    // 입력 데이터 전체를 명령 버퍼로 사용
    Fw::ComBuffer buff;
    if (Size > 0) {
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        buff.serialize(Data, Size);
    }

    // 상태 초기화
    tester.resetState();

    // 명령 전송 및 결과 확인
    auto result = tester.dispatchFuzzedCommand(buff, 0);

    // 에러 발생 시 로그 출력 (퍼저가 크래시 입력을 쉽게 찾도록)
    if (result.hasError) {
        fprintf(stderr, "[FuzzError] Input size: %zu, LastOpcode: 0x%X, LastResponse: %d\n", Size, result.lastOpcode, result.lastResponse.e);
    }

    return 0;
}