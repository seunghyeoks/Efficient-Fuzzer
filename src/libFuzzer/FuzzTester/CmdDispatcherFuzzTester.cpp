#include "CmdDispatcherFuzzTester.hpp"
#include <cstdio>
#include <cstring>

namespace Svc {

    void CmdDispatcherFuzzTester::init(NATIVE_INT_TYPE instance) {
        CommandDispatcherTesterBase::init();
    }

    CmdDispatcherFuzzTester::CmdDispatcherFuzzTester(Svc::CommandDispatcherImpl& inst) :
        CommandDispatcherTesterBase("testerbase",100),
        m_impl(inst) {
            this->m_fuzzResult = FuzzResult();
    }

    // 소멸자
    CmdDispatcherFuzzTester::~CmdDispatcherFuzzTester() {
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



    // 상태 초기화
    void CmdDispatcherFuzzTester::resetState() {
        // 이벤트 및 텔레메트리 이력 초기화
        this->clearHistory();
        this->clearEvents();
        this->clearTlm();
        
        // 퍼징 결과 초기화
        this->m_fuzzResult = FuzzResult();
        this->m_fuzzResult.opCodeReregistered = false;
    }

    // 명령어 처리기 등록
    void CmdDispatcherFuzzTester::registerCommands(U32 num, FwOpcodeType startOpCode) {
        for (U32 i = 0; i < num; i++) { // 타입 일치
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
        // this->m_impl.doDispatch(); // 주석 처리 또는 public 래퍼 함수 사용
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

    // 이벤트 핸들러 - OpCode 재등록
    void CmdDispatcherFuzzTester::logIn_DIAGNOSTIC_OpCodeReregistered(
        U32 Opcode,
        I32 port
    ) {
        CommandDispatcherTesterBase::logIn_DIAGNOSTIC_OpCodeReregistered(Opcode, port);
        m_fuzzResult.opCodeReregistered = true;
        // This event is diagnostic, so it might not always be an error for the fuzzer.
        // Depending on the fuzzing strategy, you might want to set m_fuzzResult.hasError = true here.
    }

    void CmdDispatcherFuzzTester::public_doDispatchLoop() {
        // this->m_impl.doDispatch(); // 메시지 큐를 처리하여 명령 디스패치 수행 - 첫 호출은 루프에 통합
        // 추가 디스패치가 남아 있을 수 있으므로 반복 처리
        
        const int MAX_DISPATCH_COUNT = 5; // 예: 최대 5번 디스패치 시도
        int dispatch_attempts = 0;
        
        // 메시지 큐를 처리하여 명령 디스패치 수행
        // 제한된 횟수만큼만 doDispatch를 호출하여 무한 블로킹 방지 시도
        while (dispatch_attempts < MAX_DISPATCH_COUNT && 
               this->m_impl.doDispatch() == Fw::QueuedComponentBase::MSG_DISPATCH_OK) {
            dispatch_attempts++;
            // 루프가 돌 때마다 로그를 남기면 좋겠지만, 현재 printf가 안되므로 카운트만 합니다.
        }
        // 만약 dispatch_attempts가 MAX_DISPATCH_COUNT에 도달했다면,
        // 여전히 처리할 메시지가 남아있을 수 있으나 강제 종료한 것입니다.
        // 그렇지 않다면, 큐가 비었거나 (BLOCKING 상태 진입 전) MSG_DISPATCH_OK 외의 상태가 반환된 것입니다.
    }

    Fw::ComBuffer CmdDispatcherFuzzTester::createFuzzedCommandBuffer(
        const uint8_t* data, 
        size_t size
    ) {
        Fw::ComBuffer buff;
        
        // 항상 명령어 패킷 타입은 유지 (기본 구조)
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        
        // 데이터가 너무 작으면 기본값 사용
        if (size < 2) {
            // 기본 opcode 사용
            buff.serialize(static_cast<FwOpcodeType>(0x1234));
            return buff;
        }
        
        // 첫 2바이트로 opcode 생성
        FwOpcodeType opcode = static_cast<FwOpcodeType>(data[0]) | 
                             (static_cast<FwOpcodeType>(data[1]) << 8);
        buff.serialize(opcode);
        
        // 나머지 데이터는 명령어 인자로 사용
        // 타입 일치시키기 위해 static_cast 사용
        const size_t remainingSize = size - 2;
        const size_t buffSpace = static_cast<size_t>(buff.getBuffCapacity() - buff.getBuffLength());
        const size_t argSize = (remainingSize < buffSpace) ? remainingSize : buffSpace;
        
        if (argSize > 0) {
            buff.serialize(&data[2], argSize);
        }
        
        return buff;
    }

    /*
    Fw::ComBuffer CmdDispatcherFuzzTester::createFuzzedCommandBufferWithStrategy(
        const uint8_t* data, 
        size_t size,
        FuzzStrategy strategy
    ) {
        // 여러 전략 구현: 유효한 명령어, 잘못된 opcode, 잘못된 형식 등
        switch(strategy) {
            case STRATEGY_VALID:
                // 유효한 명령어 생성
                break;
            case STRATEGY_INVALID_OPCODE:
                // 잘못된 opcode 생성
                break;
            // ...
        }
    }
    */

} // namespace Svc
