#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cstdio> // fprintf를 위해 추가

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

    fprintf(stderr, "LLVMFuzzerTestOneInput_START\n"); fflush(stderr);

    Svc::CommandDispatcherImpl impl("CmdDispImpl");
    fprintf(stderr, "LLVMFuzzerTestOneInput_IMPL_CREATED\n"); fflush(stderr); 
    impl.init(10, 0);
    fprintf(stderr, "LLVMFuzzerTestOneInput_IMPL_INITED\n"); fflush(stderr); 

/*

    Svc::CmdDispatcherFuzzTester tester(impl);
    fprintf(stderr, "LLVMFuzzerTestOneInput_TESTER_CREATED\n"); fflush(stderr); 
    tester.init();
    fprintf(stderr, "LLVMFuzzerTestOneInput_TESTER_INITED\n"); fflush(stderr); 
    tester.connectPorts();
    fprintf(stderr, "LLVMFuzzerTestOneInput_PORTS_CONNECTED\n"); fflush(stderr); 

    impl.regCommands();
    fprintf(stderr, "LLVMFuzzerTestOneInput_CMDS_REGISTERED\n"); fflush(stderr); 

    if (Size < 1) {
        fprintf(stderr, "LLVMFuzzerTestOneInput_SKIP_SMALL_INPUT\n"); fflush(stderr);
        return 0; 
    }

    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    const size_t cap = buff.getBuffCapacity() - sizeof(FwPacketDescriptorType);
    const size_t len = (Size > cap ? cap : Size);
    buff.serialize(Data, len);
    fprintf(stderr, "LLVMFuzzerTestOneInput_BUFFER_SERIALIZED\n"); fflush(stderr);

    tester.resetState();
    fprintf(stderr, "LLVMFuzzerTestOneInput_STATE_RESET\n"); fflush(stderr);

    tester.public_invoke_to_seqCmdBuff(0, buff, 0); 
    fprintf(stderr, "LLVMFuzzerTestOneInput_CMD_INVOKED\n"); fflush(stderr);

    tester.public_doDispatchLoop();
    fprintf(stderr, "LLVMFuzzerTestOneInput_DISPATCH_LOOP_DONE\n"); fflush(stderr);

    const auto& result = tester.getFuzzResult();

    if (result.hasError) {
        fprintf(stderr, "[FuzzError] Input size: %zu (clamped to %zu), LastOpcode: 0x%X, LastResponse: %d\n",
                Size, len, result.lastOpcode, result.lastResponse.e);
        fflush(stderr);
    }
    fprintf(stderr, "LLVMFuzzerTestOneInput_END\n"); fflush(stderr);
*/
    return 0;
}
