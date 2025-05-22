// CommandDispatcherTesterBase를 상속하여, Fuzz Testing 입력을 CmdDispatcher에 전달하는 harness 클래스

#ifndef CMD_DISPATCHER_FUZZ_TESTER_HPP
#define CMD_DISPATCHER_FUZZ_TESTER_HPP

#include <F-Prime/Svc/CmdDispatcher/CommandDispatcherTesterBase.hpp>
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <vector>

namespace Svc {

class CmdDispatcherFuzzTester : public CommandDispatcherTesterBase {
  public:
    // 퍼징 결과 추적 구조체
    struct FuzzResult {
        bool hasError;
        Fw::CmdResponse lastResponse;
        U32 lastOpcode;
        std::vector<Fw::CmdResponse> allResponses;
        U32 dispatchCount;
        U32 errorCount;
        
        FuzzResult() : hasError(false), lastResponse(Fw::CmdResponse::OK), 
                       lastOpcode(0), dispatchCount(0), errorCount(0) {}
    };

    // 생성자
    CmdDispatcherFuzzTester(const char* compName = "CmdDispatcherFuzzTester");
    
    // 소멸자
    virtual ~CmdDispatcherFuzzTester();
    
    // 초기화 메소드
    void init(U32 cmdTimeout = 10, U32 cmdDispatcherNum = 0);
    
    // 컴포넌트 포트 연결
    void connectPorts();
    
    // 퍼저 입력을 전달하는 메소드
    FuzzResult dispatchFuzzedCommand(const Fw::ComBuffer& buff, U32 context = 0);
    
    // 상태 초기화
    void resetState();
    
    // 명령어 처리기 등록
    void registerCommands(U32 num, FwOpcodeType startOpCode = 0x100);
    
    // CommandDispatcherImpl에 직접 접근
    CommandDispatcherImpl& getImpl();
    
    // 핑 테스트 메소드
    void sendPing(U32 key);
    
  protected:
    // TesterBase 핸들러 오버라이드
    void from_seqCmdStatus_handler(
        FwIndexType portNum,
        FwOpcodeType opCode,
        U32 cmdSeq,
        const Fw::CmdResponse& response
    ) override;
    
    // 텔레메트리 핸들러 오버라이드
    void tlmInput_CommandsDispatched(
        const Fw::Time& timeTag,
        const U32 val
    ) override;
    
    void tlmInput_CommandErrors(
        const Fw::Time& timeTag,
        const U32 val
    ) override;
    
    // 이벤트 핸들러 오버라이드
    void logIn_WARNING_HI_MalformedCommand(
        Fw::DeserialStatus Status
    ) override;
    
    void logIn_WARNING_HI_InvalidCommand(
        U32 Opcode
    ) override;
    
    void logIn_WARNING_HI_TooManyCommands(
        U32 Opcode
    ) override;
    
    void logIn_COMMAND_OpCodeError(
        U32 Opcode,
        Fw::CmdResponse error
    ) override;
    
  private:
    // 명령 디스패처 구현체
    CommandDispatcherImpl m_impl;
    
    // 퍼징 결과 저장
    FuzzResult m_fuzzResult;
};

} // namespace Svc

#endif // CMD_DISPATCHER_FUZZ_TESTER_HPP