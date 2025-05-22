#include <Fw/Types/Assert.hpp>
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
// CommandDispatcherTesterBase.hpp가 있다면 포함
// #include <CommandDispatcherTesterBase.hpp>

// 퍼징 전용 테스터 클래스 (GTest 없이)
class FuzzTester {
public:
    FuzzTester() : m_impl("CmdDispImpl") {
        // 초기화
        m_impl.init(10, 0);
        
        // CommandDispatcherTester.cpp의 connectPorts 함수에서 영감을 받은 포트 연결 로직
        // 명령 입력 포트
        m_impl.set_compCmdSend_OutputPort(0, 
            [this](FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer &args) {
                // 명령 처리 결과를 수집할 수 있음
                m_lastOpCode = opCode;
                m_lastCmdSeq = cmdSeq;
            });
        
        m_impl.set_seqCmdStatus_OutputPort(0,
            [this](FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdResponse &response) {
                // 명령 상태 응답을 추적
                m_lastStatusOpCode = opCode;
                m_lastStatusCmdSeq = cmdSeq;
                m_lastStatusResponse = response;
            });
            
        // CmdReg 포트를 내부 compCmdReg 포트에 연결 (자체 등록용)
        m_impl.set_CmdReg_OutputPort(0, m_impl.get_compCmdReg_InputPort(1));
        m_impl.set_CmdStatus_OutputPort(0, m_impl.get_compCmdStat_InputPort(0));
        
        // 로깅/텔레메트리 포트도 연결
        m_impl.set_Tlm_OutputPort(0, [](const Fw::Time& time, U32 id, Fw::TlmBuffer& val) {});
        m_impl.set_Time_OutputPort(0, [](Fw::Time &time) { time = Fw::Time(); });
        m_impl.set_Log_OutputPort(0, [](const Fw::Time& time, U32 id, Fw::LogSeverity severity, Fw::LogBuffer& val) {});
        m_impl.set_LogText_OutputPort(0, [](const Fw::Time& time, U32 id, Fw::LogSeverity severity, const Fw::TextLogString& text) {});
        
        // 기본 명령어 등록
        m_impl.regCommands();
    }
    
    // 명령 버퍼 주입 메소드
    void sendCommand(const Fw::ComBuffer& buff, U32 context = 0) {
        // seqCmdBuff_handler 직접 호출
        m_impl.seqCmdBuff_handler(0, buff, context);
        m_impl.doDispatch();
    }
    
    // 상태 확인 메소드들 (ASSERT_* 대신 사용)
    bool wasCommandHandled() const {
        return m_lastOpCode != 0;
    }
    
    bool wasCommandSuccessful() const {
        return m_lastStatusResponse == Fw::CmdResponse::OK;
    }
    
private:
    Svc::CommandDispatcherImpl m_impl;
    
    // 테스트 상태 변수들 (ASSERT_* 대신 상태 추적용)
    FwOpcodeType m_lastOpCode = 0;
    U32 m_lastCmdSeq = 0;
    
    FwOpcodeType m_lastStatusOpCode = 0;
    U32 m_lastStatusCmdSeq = 0;
    Fw::CmdResponse m_lastStatusResponse = Fw::CmdResponse::EXECUTION_ERROR;
};

// libFuzzer 엔트리 포인트
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    static FuzzTester tester;
    
    if (Size > sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType)) {
        Fw::ComBuffer buff;
        // 명령 패킷 헤더 추가
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        
        // Data에서 OpCode 추출 (또는 랜덤 값 사용)
        FwOpcodeType opCode;
        memcpy(&opCode, Data, sizeof(FwOpcodeType));
        buff.serialize(opCode);
        
        // 나머지 데이터를 인자로 추가
        const size_t argSize = Size - sizeof(FwOpcodeType);
        if (argSize > 0) {
            buff.serialize(&Data[sizeof(FwOpcodeType)], argSize > 1024 ? 1024 : argSize);
        }
        
        // 명령 버퍼 전송
        U32 context = 0;
        if (Size >= sizeof(FwOpcodeType) + sizeof(U32)) {
            memcpy(&context, &Data[sizeof(FwOpcodeType)], sizeof(U32));
        }
        tester.sendCommand(buff, context);
        
        // 선택적: 결과 확인 및 추가 로직
        // if (tester.wasCommandHandled() && !tester.wasCommandSuccessful()) {
        //    // 특정 조건에서 추가 테스트 또는 로깅
        // }
    }
    
    return 0;
}