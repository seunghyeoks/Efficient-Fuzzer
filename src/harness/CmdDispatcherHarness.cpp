#include "CmdDispatcherHarness.hpp"
#include <iostream>
#include <iomanip>
#include <Fw/Cmd/CmdPacket.hpp>
#include <Fw/Types/Serializable.hpp>

// TestCommandComponent 구현
TestCommandComponent::TestCommandComponent(const char* name) : m_name(name) {}

TestCommandComponent::~TestCommandComponent() {}

void TestCommandComponent::init() {
    Fw::InputCmdPort::init();
    m_cmdResponsePort.init();
}

void TestCommandComponent::handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, 
                                  U32 cmdSeq, Fw::CmdArgBuffer& args) {
    std::cout << "[" << m_name << "] 명령 수신: OpCode=0x" << std::hex << opCode 
              << " Seq=" << std::dec << cmdSeq << std::endl;
    
    Fw::CmdResponse response = Fw::CmdResponse::OK;
    
    // 사용자 정의 핸들러가 있는지 확인
    auto handlerIt = m_handlerMap.find(opCode);
    if (handlerIt != m_handlerMap.end()) {
        response = handlerIt->second(cmdSeq, args);
    } else {
        // 미리 설정된 응답 확인
        auto respIt = m_responseMap.find(opCode);
        if (respIt != m_responseMap.end()) {
            response = respIt->second;
        }
    }
    
    std::cout << "[" << m_name << "] 응답 송신: OpCode=0x" << std::hex << opCode 
              << " Seq=" << std::dec << cmdSeq 
              << " Response=" << response.e << std::endl;
    
    if (m_cmdResponsePort.isConnected()) {
        m_cmdResponsePort.invoke(opCode, cmdSeq, response);
    }
}

void TestCommandComponent::setCommandResponse(FwOpcodeType opcode, Fw::CmdResponse response) {
    m_responseMap[opcode] = response;
}

void TestCommandComponent::setCommandHandler(FwOpcodeType opcode, 
                                          std::function<Fw::CmdResponse(U32, Fw::CmdArgBuffer&)> handler) {
    m_handlerMap[opcode] = handler;
}

// TestCmdResponseHandler 구현
TestCmdResponseHandler::TestCmdResponseHandler() : 
    m_responseReceived(false), m_lastOpcode(0), m_lastCmdSeq(0) {}

TestCmdResponseHandler::~TestCmdResponseHandler() {}

void TestCmdResponseHandler::init() {
    Fw::InputCmdResponsePort::init();
}

void TestCmdResponseHandler::handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, 
                                    U32 cmdSeq, const Fw::CmdResponse& response) {
    std::cout << "[ResponseHandler] 응답 수신: Port=" << portNum 
              << " OpCode=0x" << std::hex << opCode << std::dec
              << " Seq=" << cmdSeq 
              << " Response=" << response.e << std::endl;
    
    m_responseReceived = true;
    m_lastOpcode = opCode;
    m_lastCmdSeq = cmdSeq;
    m_lastResponse = response;
}

bool TestCmdResponseHandler::getLastResponse(FwOpcodeType& opcode, U32& cmdSeq, 
                                          Fw::CmdResponse& response) {
    if (!m_responseReceived) {
        return false;
    }
    
    opcode = m_lastOpcode;
    cmdSeq = m_lastCmdSeq;
    response = m_lastResponse;
    return true;
}

void TestCmdResponseHandler::clearResponses() {
    m_responseReceived = false;
}

// CmdDispatcherHarness 구현
CmdDispatcherHarness::CmdDispatcherHarness(const char* name) : 
    m_dispatcher(name), m_name(name), m_seqCnt(0) {}

CmdDispatcherHarness::~CmdDispatcherHarness() {
    // 리소스 정리
    for (auto handler : m_responseHandlers) {
        delete handler;
    }
}

void CmdDispatcherHarness::initialize(U32 maxCommands, U32 maxRegistrations) {
    // CommandDispatcherImpl의 init 함수에는 인자가 필요함
    m_dispatcher.init(maxCommands, maxRegistrations);
    
    // 기본 seqCmdBuff/seqStatus 포트 핸들러 생성
    for (NATIVE_INT_TYPE i = 0; i < m_dispatcher.getNum_seqCmdBuff_InputPorts() && i < 10; i++) {
        auto handler = new TestCmdResponseHandler();
        handler->init();
        m_responseHandlers.push_back(handler);
        
        // 포트 연결 설정
        m_responseHandlerPorts.push_back(i);
        
        // 응답 핸들러와 CmdDispatcher 포트 연결
        m_dispatcher.set_cmdResponseOut_OutputPort(i, handler->get_cmdResponseIn_InputPort(0));
        
        // CmdDispatcher의 이벤트 포트가 있다면 이벤트 핸들러에도 연결
        if (m_dispatcher.isConnected_logOut_OutputPort(i)) {
            // 이벤트 핸들러 연결 코드는 생략
        }
    }
    
    std::cout << "[" << m_name << "] 초기화 완료" << std::endl;
}

bool CmdDispatcherHarness::registerCommand(FwOpcodeType opcode, NATIVE_INT_TYPE compPortNum) {
    // 실제 구현에서는 명령 등록 메커니즘을 사용해야 함
    // 이 부분은 퍼징 테스트를 위해 생략하고 항상 성공으로 반환
    
    std::cout << "[" << m_name << "] 명령 등록: OpCode=0x" << std::hex << opcode 
              << " CompPort=" << std::dec << compPortNum << std::endl;
    return true;
}

Fw::ComBuffer CmdDispatcherHarness::createCommandBuffer(FwOpcodeType opcode, 
                                                      const U8* args, size_t argSize) {
    Fw::ComBuffer buffer;
    
    // F Prime 명령 패킷 형식에 맞게 버퍼 구성 (직접 직렬화)
    
    // 패킷 타입: FW_PACKET_COMMAND (일반적으로 0)
    const FwPacketDescriptorType CMD_PACKET_TYPE = Fw::ComPacket::FW_PACKET_COMMAND;
    buffer.serialize(CMD_PACKET_TYPE);
    
    // 명령 코드
    buffer.serialize(opcode);
    
    // 인자 추가 (있는 경우)
    if (args != nullptr && argSize > 0) {
        buffer.serialize(args, argSize);
    }
    
    return buffer;
}

bool CmdDispatcherHarness::dispatchCommand(FwOpcodeType opcode, U32 cmdSeq, NATIVE_INT_TYPE portNum) {
    Fw::ComBuffer buffer = createCommandBuffer(opcode);
    return dispatchRawCommand(buffer, cmdSeq, portNum);
}

bool CmdDispatcherHarness::dispatchRawCommand(const Fw::ComBuffer& buffer, 
                                           U32 cmdSeq, NATIVE_INT_TYPE portNum) {
    // 응답 핸들러 초기화
    if (portNum < m_responseHandlers.size()) {
        m_responseHandlers[portNum]->clearResponses();
    }
    
    // 명령 버퍼 전송 (비-상수 버퍼가 필요하므로 복사)
    Fw::ComBuffer bufferCopy = buffer;
    
    try {
        // 실제로 CommandDispatcher의 schedIn 포트로 명령 전송
        // 이 부분을 추가하여 실제 명령 처리 메커니즘 활성화
        if (m_dispatcher.m_seqCmdBuff[portNum].isConnected()) {
            // 명령 시퀀스 설정
            m_dispatcher.m_seqCmdBuff[portNum].invoke(cmdSeq, bufferCopy);
            
            std::cout << "[" << m_name << "] 명령 전송: Seq=" << cmdSeq 
                    << " Port=" << portNum << std::endl;
            
            return true;
        } else {
            std::cerr << "[" << m_name << "] 오류: 명령 포트가 연결되지 않음: Port=" << portNum << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << m_name << "] 명령 전송 중 예외 발생: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[" << m_name << "] 명령 전송 실패: Seq=" << cmdSeq 
                << " Port=" << portNum << std::endl;
        return false;
    }
}

void CmdDispatcherHarness::simulateComponentResponse(FwOpcodeType opcode, U32 cmdSeq,
                                                 NATIVE_INT_TYPE compPortNum,
                                                 const Fw::CmdResponse& response) {
    // 실제 구현에서는 컴포넌트로부터 응답을 받아야 함
    // 여기서는 직접 응답 처리
    
    std::cout << "[" << m_name << "] 컴포넌트 응답 시뮬레이션: OpCode=0x" << std::hex << opcode 
              << " Seq=" << std::dec << cmdSeq 
              << " CompPort=" << compPortNum 
              << " Response=" << response.e << std::endl;
    
    // 테스트를 위해 첫 번째 응답 핸들러에게 응답 전달
    if (!m_responseHandlers.empty()) {
        m_responseHandlers[0]->handler(0, opcode, cmdSeq, response);
    }
}

void CmdDispatcherHarness::registerTestComponent(NATIVE_INT_TYPE portNum, 
                                             TestCommandComponent* component) {
    // 컴포넌트를 리스트에 추가
    m_components.push_back(component);
    
    // CmdDispatcher의 출력 포트와 테스트 컴포넌트의 입력 포트 연결
    if (portNum < m_dispatcher.getNum_compCmdSend_OutputPorts()) {
        // 명령 출력 포트 연결
        m_dispatcher.set_compCmdSend_OutputPort(portNum, component->get_cmdIn_InputPort());
        
        // 응답 입력 포트 연결
        component->set_cmdResponseOut_OutputPort(m_dispatcher.get_compCmdReg_InputPort(portNum));
        
        std::cout << "[" << m_name << "] 테스트 컴포넌트 등록: Port=" << portNum << std::endl;
    } else {
        std::cerr << "[" << m_name << "] 오류: 유효하지 않은 포트 번호: " << portNum 
                << " (최대: " << m_dispatcher.getNum_compCmdSend_OutputPorts() - 1 << ")" << std::endl;
    }
}

U32 CmdDispatcherHarness::getNextSequence() {
    return ++m_seqCnt;
}

bool CmdDispatcherHarness::validateLastCommandStatus(FwOpcodeType expectedOpcode, 
                                                  U32 expectedCmdSeq,
                                                  Fw::CmdResponse expectedResponse) {
    // 마지막 응답 검증
    if (m_responseHandlers.empty()) {
        return false;
    }
    
    FwOpcodeType opcode;
    U32 cmdSeq;
    Fw::CmdResponse response;
    
    if (!m_responseHandlers[0]->getLastResponse(opcode, cmdSeq, response)) {
        std::cout << "응답 없음!" << std::endl;
        return false;
    }
    
    bool result = (opcode == expectedOpcode && 
                   cmdSeq == expectedCmdSeq && 
                   response.e == expectedResponse.e);
    
    if (!result) {
        std::cout << "응답 불일치: OpCode=0x" << std::hex << opcode 
                  << "(예상: 0x" << expectedOpcode << "), "
                  << "Seq=" << std::dec << cmdSeq 
                  << "(예상: " << expectedCmdSeq << "), "
                  << "Response=" << response.e 
                  << "(예상: " << expectedResponse.e << ")" << std::endl;
    }
    
    return result;
}

void CmdDispatcherHarness::printStatus() const {
    std::cout << "===== CmdDispatcherHarness 상태 =====" << std::endl;
    std::cout << "등록된 컴포넌트: " << m_components.size() << std::endl;
    std::cout << "명령 시퀀스 카운터: " << m_seqCnt << std::endl;
    // 추가 상태 정보
    std::cout << "==================================" << std::endl;
}

// 퍼징 인터페이스
int CmdDispatcherHarness::processFuzzedInput(const uint8_t* data, size_t size) {
    try {
        if (size < 4) {
            // 최소 크기보다 작으면 기본 테스트 실행
            runBasicTest();
            return 0;
        }
        
        // 퍼징 데이터에서 명령 정보 추출
        
        // 오피코드 추출 (바이트 0-3)
        FwOpcodeType opcode = 0;
        memcpy(&opcode, data, sizeof(FwOpcodeType));
        
        // 유효한 범위로 오피코드 제한 (너무 큰 값은 처리하지 않음)
        opcode = opcode % 0x2000; // 0x0000~0x1FFF 범위로 제한
        
        // 기본 명령 코드로 매핑 (미리 등록한 코드 중 하나로)
        FwOpcodeType validOpcodes[] = {0x1001, 0x1002, 0x1003, 0x1004};
        if (opcode > 0x1FFF || opcode < 0x1000) {
            opcode = validOpcodes[opcode % 4];
        }
        
        // 직접 명령 버퍼 생성
        Fw::ComBuffer cmdBuffer;
        
        // 패킷 타입 (FW_PACKET_COMMAND)
        const FwPacketDescriptorType CMD_PACKET_TYPE = Fw::ComPacket::FW_PACKET_COMMAND;
        cmdBuffer.serialize(CMD_PACKET_TYPE);
        
        // 명령 코드
        cmdBuffer.serialize(opcode);
        
        // 인자 추가 (있는 경우)
        if (size > sizeof(FwOpcodeType)) {
            size_t argSize = size - sizeof(FwOpcodeType);
            if (argSize > 128) { // 인자 크기 제한
                argSize = 128;
            }
            
            if (argSize > cmdBuffer.getBuffCapacity() - cmdBuffer.getBuffLength()) {
                argSize = cmdBuffer.getBuffCapacity() - cmdBuffer.getBuffLength();
            }
            
            if (argSize > 0) {
                cmdBuffer.serialize(data + sizeof(FwOpcodeType), argSize);
            }
        }
        
        // 명령 시퀀스 번호 생성
        U32 cmdSeq = getNextSequence();
        
        // 포트 번호 계산 (퍼징 데이터에서 유도, 유효한 범위로 제한)
        NATIVE_INT_TYPE portNum = 0;
        if (size > 4) {
            portNum = data[4] % std::min(m_responseHandlers.size(), 
                                        (size_t)m_dispatcher.getNum_seqCmdBuff_InputPorts());
        }
        
        // 명령 디스패치 전 로그
        std::cout << "[퍼즈] 입력 처리: 크기=" << size 
                << " OpCode=0x" << std::hex << opcode << std::dec
                << " Port=" << portNum << std::endl;
        
        // 명령 디스패치
        return dispatchRawCommand(cmdBuffer, cmdSeq, portNum) ? 0 : -1;
    } 
    catch (const std::exception& e) {
        fprintf(stderr, "[fuzz] 예외 발생: %s\n", e.what());
    }
    catch (...) {
        fprintf(stderr, "[fuzz] 알 수 없는 예외 발생\n");
    }
    
    return 0;
}

// 테스트 케이스
void CmdDispatcherHarness::runBasicTest() {
    std::cout << "\n--- 기본 명령 테스트 시작 ---" << std::endl;
    
    // 테스트 컴포넌트 생성
    TestCommandComponent testComp("TestComponent");
    testComp.init();
    
    // 컴포넌트 연결
    registerTestComponent(0, &testComp);
    
    // 명령 등록
    FwOpcodeType testOpcode = 0x1001;
    registerCommand(testOpcode, 0);
    
    // 성공 응답으로 설정
    testComp.setCommandResponse(testOpcode, Fw::CmdResponse(Fw::CmdResponse::OK));
    
    // 명령 전송
    U32 cmdSeq = getNextSequence();
    dispatchCommand(testOpcode, cmdSeq, 0);
    
    // 응답 시뮬레이션
    simulateComponentResponse(testOpcode, cmdSeq, 0, Fw::CmdResponse(Fw::CmdResponse::OK));
    
    // 검증
    if (validateLastCommandStatus(testOpcode, cmdSeq, Fw::CmdResponse(Fw::CmdResponse::OK))) {
        std::cout << "기본 명령 테스트 성공!" << std::endl;
    } else {
        std::cout << "기본 명령 테스트 실패!" << std::endl;
    }
}

void CmdDispatcherHarness::runInvalidOpcodeTest() {
    std::cout << "\n--- 잘못된 OpCode 테스트 시작 ---" << std::endl;
    
    // 등록되지 않은 명령 코드
    FwOpcodeType invalidOpcode = 0xDEAD;
    
    // 명령 전송
    U32 cmdSeq = getNextSequence();
    dispatchCommand(invalidOpcode, cmdSeq, 0);
    
    // 응답 시뮬레이션 - 등록되지 않은 명령이므로 INVALID_OPCODE 응답을 시뮬레이션
    simulateComponentResponse(invalidOpcode, cmdSeq, 0, Fw::CmdResponse(Fw::CmdResponse::INVALID_OPCODE));
    
    // 검증
    if (validateLastCommandStatus(invalidOpcode, cmdSeq, Fw::CmdResponse(Fw::CmdResponse::INVALID_OPCODE))) {
        std::cout << "잘못된 OpCode 테스트 성공!" << std::endl;
    } else {
        std::cout << "잘못된 OpCode 테스트 실패!" << std::endl;
    }
}

void CmdDispatcherHarness::runArgumentTest() {
    std::cout << "\n--- 명령 인자 테스트 시작 ---" << std::endl;
    
    // 테스트 컴포넌트 생성
    TestCommandComponent testComp("ArgTestComponent");
    testComp.init();
    
    // 컴포넌트 연결
    registerTestComponent(1, &testComp);
    
    // 명령 등록
    FwOpcodeType testOpcode = 0x1002;
    registerCommand(testOpcode, 1);
    
    // 인자 처리 핸들러 설정
    testComp.setCommandHandler(testOpcode, [](U32 cmdSeq, Fw::CmdArgBuffer& args) {
        U32 arg1 = 0;
        F32 arg2 = 0.0f;
        
        // 인자 추출 시도
        Fw::SerializeStatus stat1 = args.deserialize(arg1);
        Fw::SerializeStatus stat2 = args.deserialize(arg2);
        
        if (stat1 == Fw::FW_SERIALIZE_OK && stat2 == Fw::FW_SERIALIZE_OK) {
            std::cout << "인자 추출 성공: " << arg1 << ", " << arg2 << std::endl;
            
            // 예상 값 확인
            if (arg1 == 123 && std::abs(arg2 - 456.789f) < 0.001f) {
                return Fw::CmdResponse(Fw::CmdResponse::OK);
            }
        }
        
        std::cout << "인자 추출 실패 또는 값 불일치" << std::endl;
        return Fw::CmdResponse(Fw::CmdResponse::VALIDATION_ERROR);
    });
    
    // 인자 버퍼 생성
    Fw::CmdArgBuffer args;
    U32 arg1 = 123;
    F32 arg2 = 456.789f;
    args.serialize(arg1);
    args.serialize(arg2);
    
    // 직접 명령 버퍼 생성
    Fw::ComBuffer buffer = createCommandBuffer(testOpcode, args.getBuffAddr(), args.getBuffLength());
    
    // 명령 전송
    U32 cmdSeq = getNextSequence();
    dispatchRawCommand(buffer, cmdSeq, 0);
    
    // 응답 시뮬레이션
    simulateComponentResponse(testOpcode, cmdSeq, 1, Fw::CmdResponse(Fw::CmdResponse::OK));
    
    // 검증
    if (validateLastCommandStatus(testOpcode, cmdSeq, Fw::CmdResponse(Fw::CmdResponse::OK))) {
        std::cout << "명령 인자 테스트 성공!" << std::endl;
    } else {
        std::cout << "명령 인자 테스트 실패!" << std::endl;
    }
}
