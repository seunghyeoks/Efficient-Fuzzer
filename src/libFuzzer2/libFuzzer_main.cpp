#include "Tester.hpp"  // 자동 생성된 테스트 클래스
#include <Fw/Types/Assert.hpp>
#include <Svc/CmdDispatcher/test/ut/CommandDispatcherImplTester.hpp>
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    // CmdDispatcher 인스턴스 생성
    static Svc::CommandDispatcherImpl impl("CmdDispImpl");
    static Svc::CommandDispatcherImplTester tester(impl);
    static bool initialized = false;
    
    if (!initialized) {
        // 초기화 (테스트 메서드에서 사용하는 방식과 동일)
        impl.init(10, 0);
        tester.init();
        
        // 포트 연결
        // connectPorts(impl, tester); // 필요한 연결 함수 구현
        
        // 기본 명령어 등록
        impl.regCommands();
        
        initialized = true;
    }
    
    // 퍼저 입력을 명령 버퍼로 변환
    if (Size > sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType)) {
        Fw::ComBuffer buff;
        // 기존 CommandDispatcherImplTester 테스트처럼 버퍼 구성
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        
        // Data에서 OpCode 추출 또는 무작위 OpCode 사용
        FwOpcodeType opCode;
        memcpy(&opCode, Data, sizeof(FwOpcodeType));
        buff.serialize(opCode);
        
        // 나머지 데이터를 인자로 추가
        buff.serialize(&Data[sizeof(FwOpcodeType)], Size - sizeof(FwOpcodeType));
        
        // 명령 버퍼 전송 (테스트 케이스에서 사용하는 것과 동일한 방식)
        tester.invoke_to_seqCmdBuff(0, buff, 0); // 0은 context
        impl.doDispatch(); // 명령 처리
    }
    
    return 0;
}