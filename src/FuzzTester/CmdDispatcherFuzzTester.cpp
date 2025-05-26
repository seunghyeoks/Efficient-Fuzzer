#include "CmdDispatcherFuzzTester.hpp"
#include <cstdio>
#include <cstring>

namespace Svc {

    void CmdDispatcherFuzzTester::init(NATIVE_INT_TYPE instance) {
        CommandDispatcherTesterBase::init();
    }

    // 생성자: 내부 m_impl을 직접 생성
    CmdDispatcherFuzzTester::CmdDispatcherFuzzTester()
    : CommandDispatcherTesterBase("testerbase", 100), m_impl("CmdDispImpl") {
        this->m_fuzzResult = FuzzResult();
    }

    // 소멸자
    CmdDispatcherFuzzTester::~CmdDispatcherFuzzTester() {
    }

    // 퍼저의 랜덤 값을 받아서 초기화에 활용
    void CmdDispatcherFuzzTester::initWithFuzzParams(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance) {
        CommandDispatcherTesterBase::init();
        m_impl.init(queueDepth, instance);
    }

    void CmdDispatcherFuzzTester::from_compCmdSend_handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer &args) {
        // 명령을 받으면 바로 OK 응답 반환
        // this->invoke_to_compCmdStat(0, opCode, cmdSeq, Fw::CmdResponse::OK);
        // (원한다면 내부 변수 세팅도 추가)
        this->m_cmdSendOpCode = opCode;
        this->m_cmdSendCmdSeq = cmdSeq;
        this->m_cmdSendArgs = args;
        this->m_cmdSendRcvd = true;
    }

    void CmdDispatcherFuzzTester::from_seqCmdStatus_handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdResponse &response) {
        this->m_seqStatusRcvd = true;
        this->m_seqStatusOpCode = opCode;
        this->m_seqStatusCmdSeq = cmdSeq;
        this->m_seqStatusCmdResponse = response;
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

    // CommandDispatcherImpl에 직접 접근
    CommandDispatcherImpl& CmdDispatcherFuzzTester::getImpl() {
        return this->m_impl;
    }

    // 핑 테스트 메소드
    void CmdDispatcherFuzzTester::sendPing(U32 key) {
        this->invoke_to_pingIn(0, key);
        // this->m_impl.doDispatch(); // 주석 처리 또는 public 래퍼 함수 사용
    }

/*
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
*/
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

    // Fuzzer를 위한 일반화된 테스트 실행 메소드 구현: 상태 초기화, 명령 주입, 디스패치 수행 후 결과 반환
    Svc::CmdDispatcherFuzzTester::FuzzResult CmdDispatcherFuzzTester::tryTest(
        const uint8_t* data,
        size_t size
    ) {
        this->resetState();
        this->m_impl.regCommands();

        // 1. 임의의 opcode 등록
        FwOpcodeType opcode = (size >= 6)
            ? (static_cast<FwOpcodeType>(data[2]) |
               (static_cast<FwOpcodeType>(data[3]) << 8) |
               (static_cast<FwOpcodeType>(data[4]) << 16) |
               (static_cast<FwOpcodeType>(data[5]) << 24))
            : 0x1234;
        this->invoke_to_compCmdReg(0, opcode);

        // 2. 명령 실행 요청 (ComBuffer에 opcode/인자/컨텍스트)
        Fw::ComBuffer buff;
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        buff.serialize(opcode);
        // 남은 바이트를 인자로 직렬화 (최대 32바이트)
        // context 값
        U32 context = (size >= 10)
            ? (static_cast<U32>(data[6]) |
               (static_cast<U32>(data[7]) << 8) |
               (static_cast<U32>(data[8]) << 16) |
               (static_cast<U32>(data[9]) << 24))
            : 0;

        if (size > 11) {
            size_t arg_len = (size - 11 > 64) ? 64 : (size - 11);
            buff.serialize(reinterpret_cast<const U8*>(data + 11), arg_len);
        }
        this->invoke_to_seqCmdBuff(0, buff, context);
        this->m_impl.doDispatch();

        // 3. 명령 실행 결과 응답 (임의의 응답코드)
        U32 cmdSeq = this->m_cmdSendCmdSeq; // 실제 전달된 시퀀스 사용
        Fw::CmdResponse resp = Fw::CmdResponse::OK;
        if (size > 10) {
            switch (data[10] % 4) {
                case 0: resp = Fw::CmdResponse::OK; break;
                case 1: resp = Fw::CmdResponse::EXECUTION_ERROR; break;
                case 2: resp = Fw::CmdResponse::INVALID_OPCODE; break;
                case 3: resp = Fw::CmdResponse::VALIDATION_ERROR; break;
            }
        }
        this->invoke_to_compCmdStat(0, opcode, cmdSeq, resp);
        this->m_impl.doDispatch();

        return this->getFuzzResult();
    }

} // namespace Svc
