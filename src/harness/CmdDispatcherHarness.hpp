// CmdDispatcherHarness.hpp
#pragma once

#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Cmd/CmdResponseEnumAc.hpp>
#include <map>
#include <vector>
#include <string>
#include <functional>

/**
 * @class TestCommandDispatcherImpl
 * @brief CommandDispatcherImpl의 테스트 특화 확장 클래스
 * 
 * 이 클래스는 CommandDispatcherImpl을 확장하여 
 * private 핸들러에 접근할 수 있도록 합니다.
 */
class TestCommandDispatcherImpl : public Svc::CommandDispatcherImpl {
public:
    /**
     * @brief 생성자
     * @param name 디스패처 이름
     */
    TestCommandDispatcherImpl(const char* name) : 
        Svc::CommandDispatcherImpl(name) {}
    
    /**
     * @brief 명령 등록 핸들러 접근
     */
    void compCmdReg_handlerAccess(NATIVE_INT_TYPE portNum, FwOpcodeType opCode) {
        compCmdReg_handler(portNum, opCode);
    }
    
    /**
     * @brief 명령 버퍼 핸들러 접근
     */
    void seqCmdBuff_handlerAccess(NATIVE_INT_TYPE portNum, Fw::ComBuffer &data, U32 context) {
        seqCmdBuff_handler(portNum, data, context);
    }
    
    /**
     * @brief 명령 응답 핸들러 접근
     */
    void compCmdStat_handlerAccess(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, 
                                  U32 cmdSeq, const Fw::CmdResponse &response) {
        compCmdStat_handler(portNum, opCode, cmdSeq, response);
    }
};

/**
 * @class TestCommandComponent
 * @brief F' 컴포넌트 동작을 시뮬레이션하는 클래스
 * 
 * CmdDispatcher가 보내는 명령을 수신하고 응답하는 가상의 컴포넌트입니다.
 * 실제 F' 컴포넌트처럼 명령을 수신하고 설정된 응답이나 사용자 정의 핸들러를 
 * 통해 응답을 생성합니다.
 */
class TestCommandComponent : public Fw::InputCmdPort {
public:
    /**
     * @brief 생성자
     * @param name 컴포넌트 식별을 위한 이름
     */
    TestCommandComponent(const char* name);
    
    /**
     * @brief 소멸자
     */
    ~TestCommandComponent();
    
    /**
     * @brief 초기화 함수
     * 내부 포트를 초기화합니다.
     */
    void init();
    
    /**
     * @brief 명령 수신 핸들러 (오버라이드)
     * @param portNum 수신 포트 번호
     * @param opCode 명령 코드
     * @param cmdSeq 명령 시퀀스 번호
     * @param args 명령 인자 버퍼
     * 
     * CmdDispatcher로부터 명령을 수신하고 미리 설정된 응답이나
     * 사용자 정의 핸들러를 통해 응답을 반환합니다.
     */
    void handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer& args);
    
    /**
     * @brief 특정 명령에 대한 응답 설정
     * @param opcode 명령 코드
     * @param response 반환할 응답 코드
     * 
     * 지정된 명령 코드에 대해 반환할 응답을 미리 설정합니다.
     */
    void setCommandResponse(FwOpcodeType opcode, Fw::CmdResponse response);
    
    /**
     * @brief 사용자 정의 명령 핸들러 설정
     * @param opcode 명령 코드
     * @param handler 명령 처리 및 응답 생성을 위한 함수
     * 
     * 특정 명령을 처리할 사용자 정의 함수를 등록합니다.
     * 이 함수는 명령 시퀀스와 인자를 받아 적절한 응답을 반환해야 합니다.
     */
    void setCommandHandler(FwOpcodeType opcode, 
                          std::function<Fw::CmdResponse(U32, Fw::CmdArgBuffer&)> handler);
    
    /** CmdDispatcher와의 연결을 위해 접근 가능하게 함 */
    Fw::OutputCmdResponsePort m_cmdResponsePort;
    
private:
    /** 명령 코드별 사전 설정된 응답 매핑 */
    std::map<FwOpcodeType, Fw::CmdResponse> m_responseMap;
    
    /** 명령 코드별 사용자 정의 핸들러 매핑 */
    std::map<FwOpcodeType, std::function<Fw::CmdResponse(U32, Fw::CmdArgBuffer&)>> m_handlerMap;
    
    /** 컴포넌트 이름 */
    const char* m_name;
};

/**
 * @class TestCmdResponseHandler
 * @brief CmdDispatcher의 명령 응답을 수신하는 클래스
 * 
 * CmdDispatcher가 보내는 명령 상태 응답을 수신하고 저장하여
 * 테스트 및 검증에 사용할 수 있게 합니다.
 */
class TestCmdResponseHandler : public Fw::InputCmdResponsePort {
public:
    /**
     * @brief 생성자
     */
    TestCmdResponseHandler();
    
    /**
     * @brief 소멸자
     */
    ~TestCmdResponseHandler();
    
    /**
     * @brief 초기화 함수
     * 내부 포트를 초기화합니다.
     */
    void init();
    
    /**
     * @brief 응답 수신 핸들러 (오버라이드)
     * @param portNum 수신 포트 번호
     * @param opCode 명령 코드
     * @param cmdSeq 명령 시퀀스 번호
     * @param response 명령 처리 결과 응답
     * 
     * CmdDispatcher로부터 명령 처리 결과를 수신하고 내부 상태에 저장합니다.
     */
    void handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, 
                const Fw::CmdResponse& response);
    
    /**
     * @brief 마지막으로 수신한 응답 조회
     * @param opcode [출력] 명령 코드
     * @param cmdSeq [출력] 명령 시퀀스 번호
     * @param response [출력] 명령 응답 코드
     * @return 응답이 수신되었으면 true, 아니면 false
     * 
     * 마지막으로 수신한 응답 정보를 반환합니다.
     */
    bool getLastResponse(FwOpcodeType& opcode, U32& cmdSeq, Fw::CmdResponse& response);
    
    /**
     * @brief 수신 응답 초기화
     * 
     * 수신 상태와 마지막 응답 정보를 초기화합니다.
     */
    void clearResponses();
    
private:
    /** 응답 수신 여부 */
    bool m_responseReceived;
    
    /** 마지막으로 수신한 명령 코드 */
    FwOpcodeType m_lastOpcode;
    
    /** 마지막으로 수신한 명령 시퀀스 번호 */
    U32 m_lastCmdSeq;
    
    /** 마지막으로 수신한 응답 코드 */
    Fw::CmdResponse m_lastResponse;
};

/**
 * @class CmdDispatcherHarness
 * @brief F' CmdDispatcher 테스트 및 퍼징을 위한 핵심 하네스 클래스
 * 
 * CmdDispatcher를 테스트하고 퍼징하기 위한 환경을 제공합니다.
 * 명령 등록, 디스패치, 응답 처리 및 퍼징 인터페이스를 구현합니다.
 */
class CmdDispatcherHarness {
public:
    /**
     * @brief 생성자
     * @param name 하네스 이름
     */
    CmdDispatcherHarness(const char* name = "CmdDispatcherHarness");
    
    /**
     * @brief 소멸자
     */
    ~CmdDispatcherHarness();
    
    /**
     * @brief 하네스 초기화
     * @param maxCommands 최대 명령 수
     * @param maxRegistrations 최대 등록 수
     * 
     * CmdDispatcher와 관련 포트를 초기화합니다.
     */
    void initialize(U32 maxCommands = 100, U32 maxRegistrations = 100);
    
    /**
     * @brief 명령 등록
     * @param opcode 등록할 명령 코드
     * @param compPortNum 명령을 처리할 컴포넌트 포트 번호
     * @return 등록 성공 여부
     * 
     * 특정 명령 코드를 지정된 컴포넌트 포트에 등록합니다.
     */
    bool registerCommand(FwOpcodeType opcode, NATIVE_INT_TYPE compPortNum);
    
    /**
     * @brief 명령 버퍼 생성
     * @param opcode 명령 코드
     * @param args 명령 인자 데이터
     * @param argSize 인자 데이터 크기
     * @return 생성된 명령 버퍼
     * 
     * F' 명령 프로토콜에 맞는 명령 버퍼를 생성합니다.
     */
    Fw::ComBuffer createCommandBuffer(FwOpcodeType opcode, const U8* args = nullptr, size_t argSize = 0);
    
    /**
     * @brief 명령 디스패치
     * @param opcode 명령 코드
     * @param cmdSeq 명령 시퀀스 번호
     * @param portNum 소스 포트 번호
     * @return 디스패치 성공 여부
     * 
     * 지정된 명령 코드로 새 명령 버퍼를 생성하여 디스패치합니다.
     */
    bool dispatchCommand(FwOpcodeType opcode, U32 cmdSeq, NATIVE_INT_TYPE portNum = 0);
    
    /**
     * @brief 원시 명령 버퍼 디스패치
     * @param buffer 명령 버퍼
     * @param cmdSeq 명령 시퀀스 번호
     * @param portNum 소스 포트 번호
     * @return 디스패치 성공 여부
     * 
     * 미리 생성된 명령 버퍼를 CmdDispatcher에 전달합니다.
     */
    bool dispatchRawCommand(const Fw::ComBuffer& buffer, U32 cmdSeq, NATIVE_INT_TYPE portNum = 0);
    
    /**
     * @brief 컴포넌트 응답 시뮬레이션
     * @param opcode 명령 코드
     * @param cmdSeq 명령 시퀀스 번호
     * @param compPortNum 컴포넌트 포트 번호
     * @param response 명령 응답 코드
     * 
     * 컴포넌트가 명령에 응답하는 것을 시뮬레이션합니다.
     */
    void simulateComponentResponse(FwOpcodeType opcode, U32 cmdSeq, 
                                 NATIVE_INT_TYPE compPortNum, 
                                 const Fw::CmdResponse& response);
    
    /**
     * @brief 테스트 컴포넌트 등록
     * @param portNum 컴포넌트 포트 번호
     * @param component 등록할 테스트 컴포넌트
     * 
     * 테스트 컴포넌트를 CmdDispatcher에 연결합니다.
     */
    void registerTestComponent(NATIVE_INT_TYPE portNum, TestCommandComponent* component);
    
    /**
     * @brief 퍼징 입력 처리 인터페이스
     * @param data 퍼저가 생성한 바이너리 데이터
     * @param size 데이터 크기
     * @return 처리 결과 코드 (0은 성공)
     * 
     * 퍼저가 생성한 바이너리 데이터를 F' 명령으로 변환하고 처리합니다.
     * AFL++, libFuzzer 등의 퍼저와 연동하기 위한 핵심 인터페이스입니다.
     */
    int processFuzzedInput(const uint8_t* data, size_t size);
    
    /**
     * @brief 마지막 명령 상태 검증
     * @param expectedOpcode 예상 명령 코드
     * @param expectedCmdSeq 예상 명령 시퀀스 번호
     * @param expectedResponse 예상 응답 코드
     * @return 검증 성공 여부
     * 
     * 마지막으로 수신한 명령 응답이 예상값과 일치하는지 검증합니다.
     */
    bool validateLastCommandStatus(FwOpcodeType expectedOpcode, U32 expectedCmdSeq,
                                  Fw::CmdResponse expectedResponse);
    
    /**
     * @brief 하네스 상태 출력
     * 
     * 현재 하네스의 상태 정보를 콘솔에 출력합니다.
     */
    void printStatus() const;
    
    /**
     * @brief 기본 명령 처리 테스트
     * 
     * 정상적인 명령 등록, 디스패치, 응답 처리를 테스트합니다.
     * 하네스 검증 및 초기 코퍼스 생성에 유용합니다.
     */
    void runBasicTest();
    
    /**
     * @brief 잘못된 명령 코드 테스트
     * 
     * 등록되지 않은 명령 코드로 디스패치 했을 때 
     * CmdDispatcher의 동작을 테스트합니다.
     */
    void runInvalidOpcodeTest();
    
    /**
     * @brief 명령 인자 처리 테스트
     * 
     * 인자를 포함한 명령 처리 과정을 테스트합니다.
     */
    void runArgumentTest();
    
private:
    /** CmdDispatcher 인스턴스 */
    TestCommandDispatcherImpl m_dispatcher;
    
    /** 하네스 이름 */
    const char* m_name;
    
    /** 등록된 테스트 컴포넌트 목록 */
    std::vector<TestCommandComponent*> m_components;
    
    /** 응답 핸들러 목록 */
    std::vector<TestCmdResponseHandler*> m_responseHandlers;
    
    /** 응답 핸들러 포트 번호 */
    std::vector<NATIVE_INT_TYPE> m_responseHandlerPorts;
    
    /** 명령 시퀀스 카운터 */
    U32 m_seqCnt;
    
    /**
     * @brief 다음 시퀀스 번호 생성
     * @return 새 시퀀스 번호
     * 
     * 내부 카운터를 증가시켜 새 명령 시퀀스 번호를 생성합니다.
     */
    U32 getNextSequence();
};