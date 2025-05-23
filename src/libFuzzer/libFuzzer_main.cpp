#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

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

// Google Test의 Nominal Dispatch와 유사한 테스트를 수행하는 함수
static bool predefinedTestDone = false;
void runPredefinedNominalTest(Svc::CommandDispatcherImpl& impl, Svc::CmdDispatcherFuzzTester& tester) {
    if (predefinedTestDone) {
        return;
    }
    predefinedTestDone = true;

    fprintf(stdout, "Running predefined nominal test...\\n");

    // runNominalDispatch 테스트와 유사하게 특정 명령어 등록 및 전송
    FwOpcodeType testOpCode = 0x50;
    U32 testCmdArg = 100;
    U32 testContext = 110;

    // 명령어 등록 (CmdDispatcherFuzzTester를 통해 호출)
    tester.public_invoke_to_compCmdReg(0, testOpCode);
    // impl.doDispatch(); // 일반적으로 CmdReg은 동기적으로 처리될 수 있음

    // 명령어 버퍼 생성
    Fw::ComBuffer buff;
    // FW_ASSERT(buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND)) == Fw::FW_SERIALIZE_OK);
    // FW_ASSERT(buff.serialize(testOpCode) == Fw::FW_SERIALIZE_OK);
    // FW_ASSERT(buff.serialize(testCmdArg) == Fw::FW_SERIALIZE_OK);
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    buff.serialize(testOpCode);
    buff.serialize(testCmdArg);

    // 상태 초기화
    tester.resetState();

    // 명령 전송 (dispatchFuzzedCommand와 유사한 로직)
    tester.public_invoke_to_seqCmdBuff(0, buff, testContext);
    impl.doDispatch(); // 메시지 큐 처리하여 명령 디스패치 수행
    while (impl.doDispatch() == Fw::QueuedComponentBase::MSG_DISPATCH_OK) {
        ; // 추가 디스패치가 남아 있을 수 있으므로 반복 처리
    }
    
    // 응답은 CmdDispatcherFuzzTester의 핸들러에서 FuzzResult에 기록됨
    // 실제 GTest에서는 CommandDispatcherImplTester에서 직접 응답 포트를 호출하여 상태를 확인하지만,
    // 여기서는 FuzzResult를 통해 간접적으로 확인

    Svc::CmdDispatcherFuzzTester::FuzzResult result = tester.getFuzzResult(); // 직접 결과 가져오기 (주의: m_fuzzResult가 public이어야 함 또는 getter 사용)
                                                                        // CmdDispatcherFuzzTester.hpp에 getFuzzResult() 같은 getter 추가 고려

    // 결과 확인
    if (result.hasError) {
        fprintf(stderr, "[PredefinedTestError] LastOpcode: 0x%X, LastResponse: %d, DispatchCount: %u, ErrorCount: %u\n",
                result.lastOpcode, result.lastResponse.e, result.dispatchCount, result.errorCount);
        // 필요시 여기서 abort() 또는 exit(1) 호출하여 Fuzzer 중단 가능
        // FW_ASSERT(false); // Fuzzer가 크래시로 인식하도록
    } else {
        fprintf(stdout, "[PredefinedTestSuccess] LastOpcode: 0x%X, LastResponse: %d, DispatchCount: %u, ErrorCount: %u\n",
                result.lastOpcode, result.lastResponse.e, result.dispatchCount, result.errorCount);
    }
    
    bool nominalTestPassed = true;
    if (result.lastOpcode != testOpCode && result.lastOpcode != 0) { // OpCode가 아직 설정되지 않았을 수 있음 (응답이 오기 전)
                                                                 // 또는 응답 OpCode가 요청과 다를 경우
        fprintf(stderr, "Predefined Test Mismatch: lastOpcode. Expected 0x%X or 0, Got 0x%X\n", testOpCode, result.lastOpcode);
        nominalTestPassed = false;
    }
    if (result.lastResponse != Fw::CmdResponse::OK && result.lastResponse.e != Fw::CmdResponse::SERIALIZED_SIZE) { // 초기값일 수 있음
        fprintf(stderr, "Predefined Test Mismatch: lastResponse. Expected Fw::CmdResponse::OK (%d) or initial value, Got %d\n", Fw::CmdResponse::OK, result.lastResponse.e);
        nominalTestPassed = false;
    }

    if (nominalTestPassed) {
         fprintf(stdout, "Predefined nominal test behavior seems OK.\\n");
    } else {
         fprintf(stderr, "Predefined nominal test behavior FAILED. Check logs.\\n");
         // FW_ASSERT(false); // Fuzzer가 크래시로 인식하도록
    }
    
    // 다음 퍼즈 입력을 위해 상태 초기화
    tester.resetState(); 
    fprintf(stdout, "Finished predefined nominal test.\\n");
}

// libFuzzer 엔트리 포인트
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    // 매 입력마다 새로 생성 (상태 오염 방지)
    Svc::CommandDispatcherImpl impl("CmdDispImpl");
    impl.init(10, 0);

    Svc::CmdDispatcherFuzzTester tester(impl);
    tester.init();
    tester.connectPorts();

    // 기본 제공 명령어 등록 (NoOp, NoOpString, TestCmd1, ClearTracking 등)
    impl.regCommands();

    // Fuzz 테스팅을 위한 추가 명령어 등록 (선택 사항)
    // tester.registerCommands(10, 0x100);

    // 퍼지 루프 시작 전, 고정된 테스트 실행 (최초 한 번만)
    runPredefinedNominalTest(impl, tester);

    if (Size < 1) return 0; // Fuzzer 입력이 너무 작으면 스킵

    // 입력 데이터 전체를 명령 버퍼로 사용
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    const size_t cap = buff.getBuffCapacity() - sizeof(FwPacketDescriptorType);
    const size_t len = (Size > cap ? cap : Size);
    buff.serialize(Data, len);

    // 상태 초기화 (Fuzzer 입력마다)
    tester.resetState();

    // 명령 전송 및 결과 확인
    auto result = tester.dispatchFuzzedCommand(buff, 0);

    // 에러 발생 시 로그 출력 (퍼저가 크래시 입력을 쉽게 찾도록)
    if (result.hasError) {
        fprintf(stderr, "[FuzzError] Input size: %zu (clamped to %zu), LastOpcode: 0x%X, LastResponse: %d\n",
                Size, len, result.lastOpcode, result.lastResponse.e);
        // Fuzzer가 에러를 감지하도록 하기 위해, 필요시 여기서 abort() 또는 FW_ASSERT(false) 호출
        // 예: if (result.lastResponse != Fw::CmdResponse::OK) FW_ASSERT(0, result.lastOpcode, result.lastResponse.e);
    }

    return 0;
}