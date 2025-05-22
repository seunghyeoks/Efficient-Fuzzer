#include <Fw/Types/Assert.hpp>
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>

// 간단한 테스트 핸들러 클래스 (Google Test 없이)
class FuzzTester {
public:
    FuzzTester() : m_impl("CmdDispImpl") {
        // 초기화
        m_impl.init(10, 0);
        
        // 포트 연결 - ImplTester 없이 직접 구현
        // 출력 핸들러 설정
        m_impl.set_compCmdSend_OutputPort(0, 
            [this](FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer &args) {
                // 출력 처리 (필요시)
            });
        
        m_impl.set_seqCmdStatus_OutputPort(0,
            [this](FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdResponse &response) {
                // 출력 처리 (필요시)
            });
            
        // 다른 필요한 포트들도 연결...
        m_impl.set_Tlm_OutputPort(0, [](const Fw::Time& time, U32 id, Fw::TlmBuffer& val) {});
        m_impl.set_Time_OutputPort(0, [](Fw::Time &time) { time = Fw::Time(); });
        m_impl.set_Log_OutputPort(0, [](const Fw::Time& time, U32 id, Fw::LogSeverity severity, Fw::LogBuffer& val) {});
        m_impl.set_LogText_OutputPort(0, [](const Fw::Time& time, U32 id, Fw::LogSeverity severity, const Fw::TextLogString& text) {});
        
        // 기본 명령어 등록
        m_impl.regCommands();
    }
    
    // 명령 버퍼 주입 메소드
    void sendCommand(const Fw::ComBuffer& buff) {
        // seqCmdBuff_handler 직접 호출
        m_impl.seqCmdBuff_handler(0, buff, 0);
        m_impl.doDispatch();
    }
    
private:
    Svc::CommandDispatcherImpl m_impl;
};

// libFuzzer 엔트리 포인트
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    static FuzzTester tester;
    
    if (Size > sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType)) {
        Fw::ComBuffer buff;
        // 명령 패킷 헤더 추가
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        
        // Data에서 OpCode 추출
        FwOpcodeType opCode;
        memcpy(&opCode, Data, sizeof(FwOpcodeType));
        buff.serialize(opCode);
        
        // 나머지 데이터 추가
        const size_t argSize = Size - sizeof(FwOpcodeType);
        buff.serialize(&Data[sizeof(FwOpcodeType)], argSize > 1024 ? 1024 : argSize);
        
        // 명령 버퍼 전송
        tester.sendCommand(buff);
    }
    
    return 0;
}