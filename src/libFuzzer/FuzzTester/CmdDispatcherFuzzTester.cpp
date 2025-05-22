#include "CmdDispatcherFuzzTester.hpp"
#include <cstdio>
#include <cstring>

namespace Svc {

// 생성자
CmdDispatcherFuzzTester::CmdDispatcherFuzzTester(const char* compName) : 
    CommandDispatcherTesterBase(compName, 100),
    m_impl("CmdDispatcherImpl")
{
    // 초기 상태 설정
    this->m_fuzzResult = FuzzResult();
}

// 소멸자
CmdDispatcherFuzzTester::~CmdDispatcherFuzzTester() {
    // 필요한 정리 작업 수행
}

// 초기화 메소드
void CmdDispatcherFuzzTester::init(U32 cmdTimeout, U32 cmdDispatcherNum) {
    // 구현체 초기화
    this->m_impl.init(cmdTimeout, cmdDispatcherNum);
    
    // 포트 연결
    this->connectPorts();
}

// 포트 연결 설정
void CmdDispatcherFuzzTester::connectPorts() {
    // CommandDispatcherTester.cpp의 connectPorts 함수 참조
    
    // 명령어 포트
    this->connect_to_compCmdStat(0, this->m_impl.get_compCmdStat_InputPort(0));
    this->connect_to_seqCmdBuff(0, this->m_impl.get_seqCmdBuff_InputPort(0));
    this->connect_to_compCmdReg(0, this->m_impl.get_compCmdReg_InputPort(0));

    // 출력 포트 연결
    this->m_impl.set_compCmdSend_OutputPort(0, this->get_from_compCmdSend(0));
    this->m_impl.set_seqCmdStatus_OutputPort(0, this->get_from_seqCmdStatus(0));
    
    // 로컬 디스패처 명령어 등록
    this->m_impl.set_CmdReg_OutputPort(0, this->m_impl.get_compCmdReg_InputPort(1));
    this->m_impl.set_CmdStatus_OutputPort(0, this->m_impl.get_compCmdStat_InputPort(0));

    this->m_impl.set_compCmdSend_OutputPort(1, this->m_impl.get_CmdDisp_InputPort(0));

    // 텔레메트리, 시간, 로그 포트 연결
    this->m_impl.set_Tlm_OutputPort(0, this->get_from_Tlm(0));
    this->m_impl.set_Time_OutputPort(0, this->get_from_Time(0));
    this->m_impl.set_Log_OutputPort(0, this->get_from_Log(0));
    this->m_impl.set_LogText_OutputPort(0, this->get_from_LogText(0));
    
    // 핑 포트 연결
    this->connect_to_pingIn(0, this->m_impl.get_pingIn_InputPort(0));
    this->m_impl.set_pingOut_OutputPort(0, this->get_from_pingOut(0));
}

// 퍼저 입력을 전달하는 메소드
CmdDispatcherFuzzTester::FuzzResult CmdDispatcherFuzzTester::dispatchFuzzedCommand(
    const Fw::ComBuffer& buff, 
    U32 context
) {
    // 상태 초기화
    this->resetState();
    
    // 명령어 버퍼 전송
    this->invoke_to_seqCmdBuff(0, const_cast<Fw::ComBuffer&>(buff), context);
    
    // 명령어 디스패치 수행
    this->m_impl.doDispatch();
    
    // 결과 반환
    return m_fuzzResult;
}

// 상태 초기화
void CmdDispatcherFuzzTester::resetState() {
    // 이벤트 및 텔레메트리 이력 초기화
    this->clearHistory();
    this->clearEvents();
    this->clearTlm();
    
    // 퍼징 결과 초기화
    this->m_fuzzResult = FuzzResult();
}

// 명령어 처리기 등록
void CmdDispatcherFuzzTester::registerCommands(U32 num, FwOpcodeType startOpCode) {
    for (FwIndexType i = 0; i < num; i++) {
        FwOpcodeType opCode = startOpCode + i;
        this->invoke_to_compCmdReg(0, opCode);
    }
}

// CommandDispatcherImpl에 직접 접근
CommandDispatcherImpl& CmdDispatcherFuzzTester::getImpl() {
    return this->m_impl;
}

// 핑 테스트 메소드
void CmdDispatcherFuzzTester::sendPing(U32 key) {
    this->invoke_to_pingIn(0, key);
    this->m_impl.doDispatch();
}

// TesterBase 핸들러 오버라이드 - 명령어 응답
void CmdDispatcherFuzzTester::from_seqCmdStatus_handler(
    FwIndexType portNum,
    FwOpcodeType opCode,
    U32 cmdSeq,
    const Fw::CmdResponse& response
) {
    // 부모 클래스 핸들러 호출
    CommandDispatcherTesterBase::from_seqCmdStatus_handler(portNum, opCode, cmdSeq, response);
    
    // 응답 분석 및 기록
    m_fuzzResult.lastOpcode = opCode;
    m_fuzzResult.lastResponse = response;
    m_fuzzResult.allResponses.push_back(response);
    
    if (response != Fw::CmdResponse::OK) {
        m_fuzzResult.hasError = true;
    }
}

// 텔레메트리 핸들러 - 명령 디스패치 카운트
void CmdDispatcherFuzzTester::tlmInput_CommandsDispatched(
    const Fw::Time& timeTag,
    const U32 val
) {
    CommandDispatcherTesterBase::tlmInput_CommandsDispatched(timeTag, val);
    m_fuzzResult.dispatchCount = val;
}

// 텔레메트리 핸들러 - 명령 오류 카운트
void CmdDispatcherFuzzTester::tlmInput_CommandErrors(
    const Fw::Time& timeTag,
    const U32 val
) {
    CommandDispatcherTesterBase::tlmInput_CommandErrors(timeTag, val);
    m_fuzzResult.errorCount = val;
}

// 이벤트 핸들러 - 비정상 명령
void CmdDispatcherFuzzTester::logIn_WARNING_HI_MalformedCommand(
    Fw::DeserialStatus Status
) {
    CommandDispatcherTesterBase::logIn_WARNING_HI_MalformedCommand(Status);
    m_fuzzResult.hasError = true;
}

// 이벤트 핸들러 - 잘못된 명령
void CmdDispatcherFuzzTester::logIn_WARNING_HI_InvalidCommand(
    U32 Opcode
) {
    CommandDispatcherTesterBase::logIn_WARNING_HI_InvalidCommand(Opcode);
    m_fuzzResult.hasError = true;
}

// 이벤트 핸들러 - 너무 많은 명령
void CmdDispatcherFuzzTester::logIn_WARNING_HI_TooManyCommands(
    U32 Opcode
) {
    CommandDispatcherTesterBase::logIn_WARNING_HI_TooManyCommands(Opcode);
    m_fuzzResult.hasError = true;
}

// 이벤트 핸들러 - 명령 코드 오류
void CmdDispatcherFuzzTester::logIn_COMMAND_OpCodeError(
    U32 Opcode,
    Fw::CmdResponse error
) {
    CommandDispatcherTesterBase::logIn_COMMAND_OpCodeError(Opcode, error);
    m_fuzzResult.hasError = true;
}

} // namespace Svc
