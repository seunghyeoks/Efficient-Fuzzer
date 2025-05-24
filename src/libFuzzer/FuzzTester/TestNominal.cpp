#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <Fw/Types/Assert.hpp>
#include <cstdio> // fprintf 사용을 위해 추가

int main(int argc, char* argv[]) {
    // Os::Log::registerLogger()는 프로젝트 설정에 따라 다를 수 있으므로,
    // 여기서는 간단한 printf 로거를 사용하거나, FPrime의 로깅 메커니즘을 따릅니다.
    // FPrime GTest 환경에서는 보통 TestLog::TestLogger가 사용됩니다.
    // 여기서는 Fuzz 테스팅 환경과 유사하게 stderr에 직접 출력하거나,
    // 간단한 로거를 사용합니다.

    fprintf(stderr, "Starting TestNominal...\n");

    Svc::CommandDispatcherImpl impl("CmdDisp");
    impl.init(10, 0); // instance = 10, base id = 0

    Svc::CmdDispatcherFuzzTester tester(impl);
    tester.init();
    tester.connectPorts();

    // F' GDS 에서 기본으로 등록되는 명령어들을 여기서도 등록
    impl.regCommands();
    fprintf(stderr, "Built-in commands registered.\n");

    // 테스트할 명령어 정의
    FwOpcodeType testOpCode = 0x50; // 사용자 정의 OpCode
    U32 testCmdArg = 100;          // 명령어 인자
    U32 testContext = 110;         // 명령어 Context

    // 1. 명령어 등록 테스트
    fprintf(stderr, "Registering OpCode 0x%X...\n", testOpCode);
    tester.public_invoke_to_compCmdReg(0, testOpCode); // port 0에 명령어 등록
    // impl.doDispatch(); // CmdReg은 동기적으로 처리될 수 있음 (필요시 호출)

    // 등록 확인 (내부 상태 확인은 CmdDispatcherImplTester의 GTest 매크로를 사용해야 함)
    // 여기서는 이벤트 로그를 통해 간접적으로 확인하거나, FuzzResult의 상태 변화를 볼 수 있음.
    // 예: tester.getFuzzResult().opCodeRegistered (만약 해당 필드가 FuzzResult에 있다면)

    // 2. 명령어 전송 및 응답 테스트
    fprintf(stderr, "Dispatching command with OpCode 0x%X...\n", testOpCode);
    Fw::ComBuffer buff;
    // 패킷 타입, OpCode, 인자 순으로 직렬화
    FW_ASSERT(buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND)) == Fw::FW_SERIALIZE_OK);
    FW_ASSERT(buff.serialize(testOpCode) == Fw::FW_SERIALIZE_OK);
    FW_ASSERT(buff.serialize(testCmdArg) == Fw::FW_SERIALIZE_OK);

    // 상태 초기화 후 명령어 전송
    tester.resetState();
    tester.public_invoke_to_seqCmdBuff(0, buff, testContext);

    // 메시지 큐를 처리하여 명령 디스패치 수행
    impl.doDispatch();
    // 추가 디스패치가 남아 있을 수 있으므로 반복 처리
    while (impl.doDispatch() == Fw::QueuedComponentBase::MSG_DISPATCH_OK) {
        ;
    }

    // 결과 확인
    const Svc::CmdDispatcherFuzzTester::FuzzResult& result = tester.getFuzzResult();

    bool nominalTestPassed = true;
    fprintf(stderr, "Test Results:\n");
    fprintf(stderr, "  Has Error: %s\n", result.hasError ? "true" : "false");
    fprintf(stderr, "  Last OpCode: 0x%X (Expected: 0x%X)\n", result.lastOpcode, testOpCode);
    fprintf(stderr, "  Last Response: %d (Expected: %d - OK)\n", result.lastResponse.e, Fw::CmdResponse::OK);
    fprintf(stderr, "  Dispatch Count: %u\n", result.dispatchCount);
    fprintf(stderr, "  Error Count: %u\n", result.errorCount);
    fprintf(stderr, "  OpCode Reregistered: %s\n", result.opCodeReregistered ? "true" : "false");

    if (result.hasError) {
        fprintf(stderr, "Nominal Test FAILED: An error was reported.\n");
        nominalTestPassed = false;
    }
    if (result.lastOpcode != testOpCode) {
        fprintf(stderr, "Nominal Test FAILED: Last OpCode mismatch.\n");
        nominalTestPassed = false;
    }
    // CmdDispatcher의 기본 핸들러는 CMD_OK를 반환. 특정 핸들러가 연결되지 않은 경우에도.
    if (result.lastResponse != Fw::CmdResponse::OK) {
         // 만약 핸들러가 없어서 CmdDispatcher자체가 에러를 반환한다면 INVALID_OPCODE가 아닌 다른 상태(예: EXECUTION_ERROR)일 수 있다.
         // SvcCmdDispatcher의 기본 동작은 compCmdSend 포트로 보내고, 해당 포트에 연결된 컴포넌트가 응답을 보내야 한다.
         // TestHandler (CommandDispatcherTesterBase의 일부)는 기본적으로 OK를 반환해야 한다.
         // 그러나 여기서는 CmdDispatcherFuzzTester의 from_seqCmdStatus_handler가 호출되므로 그 로직을 따라야한다.
         // CmdDispatcherFuzzTester는 받은 응답을 그대로 기록한다.
         // 우리가 compCmdStat를 통해 compCmdSend로 전달된 명령에 대한 응답을 보내주지 않았으므로,
         // CmdDispatcher는 seqCmdStatus를 통해 응답을 보내지 않을 수 있다.
         // CmdDispatcher자체가 응답을 생성하는 경우는 NO_OP등의 빌트인 명령어들이다.
         // 지금은 사용자 정의 opcode 0x50을 사용하고, compCmdSend로 보내진 후 응답을 기다린다.
         // 이 테스트에서는 compCmdStat을 호출하는 부분이 없으므로, seqCmdStatus_handler가 호출되지 않을 가능성이 높다.
         // 그 경우 lastResponse는 초기값(OK)으로 남아있을 수 있다.
         // 하지만, 만약 CmdDispatcher가 응답을 보내는 로직이 있다면, 그 응답을 확인해야 한다.
         // GTest의 CommandDispatcherImplTester::runNominalDispatch를 보면, compCmdStat를 호출하여 응답을 시뮬레이션한다.
         // 여기서는 그 부분이 없으므로, CmdDispatcher의 응답 타임아웃 또는 기본 응답(만약 있다면)을 확인해야 한다.
         // 일단은 OK로 기대값을 설정하고, 실제 실행 결과를 보고 수정한다.
        fprintf(stderr, "Nominal Test FAILED: Last Response mismatch. (Note: This might depend on whether a command handler sends a status back via compCmdStat)\n");
        nominalTestPassed = false;
    }


    // NO_OP 명령어 테스트 (빌트인 명령어)
    fprintf(stderr, "\nTesting NO_OP command...\n");
    Fw::ComBuffer noOpBuff;
    FwOpcodeType noOpCode = Svc::CommandDispatcherImpl::OPCODE_CMD_NO_OP;
    FW_ASSERT(noOpBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND)) == Fw::FW_SERIALIZE_OK);
    FW_ASSERT(noOpBuff.serialize(noOpCode) == Fw::FW_SERIALIZE_OK);

    tester.resetState();
    tester.public_invoke_to_seqCmdBuff(0, noOpBuff, 0); // context = 0
    impl.doDispatch();
    while (impl.doDispatch() == Fw::QueuedComponentBase::MSG_DISPATCH_OK) {;}

    const Svc::CmdDispatcherFuzzTester::FuzzResult& noOpResult = tester.getFuzzResult();
    fprintf(stderr, "NO_OP Test Results:\n");
    fprintf(stderr, "  Has Error: %s\n", noOpResult.hasError ? "true" : "false");
    fprintf(stderr, "  Last OpCode: 0x%X (Expected: 0x%X)\n", noOpResult.lastOpcode, noOpCode);
    fprintf(stderr, "  Last Response: %d (Expected: %d - OK)\n", noOpResult.lastResponse.e, Fw::CmdResponse::OK);

    if (noOpResult.hasError || noOpResult.lastOpcode != noOpCode || noOpResult.lastResponse != Fw::CmdResponse::OK) {
        fprintf(stderr, "NO_OP Test FAILED.\n");
        nominalTestPassed = false;
    }


    if (nominalTestPassed) {
        fprintf(stderr, "\nAll nominal tests PASSED.\n");
        return 0;
    } else {
        fprintf(stderr, "\nOne or more nominal tests FAILED.\n");
        return 1;
    }
}
