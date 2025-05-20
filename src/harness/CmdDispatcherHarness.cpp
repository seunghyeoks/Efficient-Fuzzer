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
    // CmdDispatcher 초기화
    m_dispatcher.init(maxCommands, maxRegistrations);
    
    // 기본 seqCmdBuff/seqStatus 포트 핸들러 생성
    for (NATIVE_INT_TYPE i = 0; i < 10; i++) { // 최대 10개 포트 사용
        auto handler = new TestCmdResponseHandler();
        handler->init();
        m_responseHandlers.push_back(handler);
        
        // 직접 연결 대신 포트 번호 기록
        m_responseHandlerPorts.push_back(i);
    }
    
    std::cout << "[" << m_name << "] 초기화 완료 (maxCmds=" << maxCommands 
              << ", maxRegs=" << maxRegistrations << ")" << std::endl;
}

bool CmdDispatcherHarness::registerCommand(FwOpcodeType opcode, NATIVE_INT_TYPE compPortNum) {
    // 명령 등록 - 확장된 접근 메서드 사용
    try {
        m_dispatcher.compCmdReg_handlerAccess(compPortNum, opcode);
        std::cout << "[" << m_name << "] 명령 등록: OpCode=0x" << std::hex << opcode 
                << " CompPort=" << std::dec << compPortNum << std::endl;
        return true;
    } catch (...) {
        std::cerr << "[" << m_name << "] 명령 등록 실패: OpCode=0x" << std::hex << opcode 
                << " CompPort=" << std::dec << compPortNum << std::endl;
        return false;
    }
}

Fw::ComBuffer CmdDispatcherHarness::createCommandBuffer(FwOpcodeType opcode, 
                                                      const U8* args, size_t argSize) {
    Fw::ComBuffer buffer;
    
    // F Prime 명령 패킷 형식에 맞게 버퍼 구성
    // 실제 구현은 시스템에 맞게 조정해야 할 수 있음
    
    // 패킷 타입: 명령 (FW_PACKET_COMMAND는 일반적으로 0)
    const FwPacketDescriptorType CMD_PACKET_TYPE = 0;
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
        // 확장된 접근 메서드 호출
        m_dispatcher.seqCmdBuff_handlerAccess(portNum, bufferCopy, cmdSeq);
        
        std::cout << "[" << m_name << "] 명령 전송: Seq=" << cmdSeq 
                << " Port=" << portNum << std::endl;
        return true;
    } catch (...) {
        std::cerr << "[" << m_name << "] 명령 전송 실패: Seq=" << cmdSeq 
                << " Port=" << portNum << std::endl;
        return false;
    }
}

void CmdDispatcherHarness::simulateComponentResponse(FwOpcodeType opcode, U32 cmdSeq,
                                                 NATIVE_INT_TYPE compPortNum,
                                                 const Fw::CmdResponse& response) {
    // 컴포넌트 응답 시뮬레이션
    try {
        // 확장된 접근 메서드 호출
        m_dispatcher.compCmdStat_handlerAccess(compPortNum, opcode, cmdSeq, response);
        
        std::cout << "[" << m_name << "] 컴포넌트 응답 시뮬레이션: OpCode=0x" << std::hex << opcode 
                << " Seq=" << std::dec << cmdSeq 
                << " CompPort=" << compPortNum 
                << " Response=" << response.e << std::endl;
    } catch (...) {
        std::cerr << "[" << m_name << "] 컴포넌트 응답 시뮬레이션 실패: OpCode=0x" << std::hex << opcode 
                << " Seq=" << std::dec << cmdSeq 
                << " CompPort=" << compPortNum << std::endl;
    }
}

void CmdDispatcherHarness::registerTestComponent(NATIVE_INT_TYPE portNum, 
                                               TestCommandComponent* component) {
    // 컴포넌트를 리스트에 추가
    m_components.push_back(component);
    
    // 포트 연결은 커스텀 메서드를 통해 구현
    // 실제 구현은 시스템에 맞게 조정해야 할 수 있음
    
    std::cout << "[" << m_name << "] 테스트 컴포넌트 등록: Port=" << portNum << std::endl;
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
    if (size < 5) {
        return 0; // 최소 크기 확인
    }
    
    // 퍼징 데이터에서 명령 정보 추출
    // 첫 바이트는 패킷 타입으로 무시 (F Prime은 0을 명령 패킷으로 사용)
    
    // 오피코드 추출 (바이트 1-4)
    FwOpcodeType opcode = 0;
    memcpy(&opcode, &data[1], sizeof(FwOpcodeType));
    
    // 명령 버퍼 생성
    Fw::ComBuffer cmdBuffer;
    
    // 패킷 유형 (0 = 명령)
    const FwPacketDescriptorType CMD_PACKET_TYPE = 0;
    cmdBuffer.serialize(CMD_PACKET_TYPE);
    
    // 명령 코드
    cmdBuffer.serialize(opcode);
    
    // 인자 추가 (있는 경우)
    if (size > 5) {
        size_t argSize = size - 5;
        if (argSize > cmdBuffer.getBuffCapacity() - cmdBuffer.getBuffLength()) {
            argSize = cmdBuffer.getBuffCapacity() - cmdBuffer.getBuffLength();
        }
        cmdBuffer.serialize(&data[5], argSize);
    }
    
    // 명령 시퀀스 번호 생성
    U32 cmdSeq = getNextSequence();
    
    // 포트 번호 계산 (퍼징 데이터에서 유도)
    NATIVE_INT_TYPE portNum = data[size-1] % 10; // 0-9 사이로 제한
    
    // 명령 디스패치
    dispatchRawCommand(cmdBuffer, cmdSeq, portNum);
    
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
    
    // 검증 - 등록되지 않은 OpCode에 대해 INVALID_OPCODE 응답 기대
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
    
    // 명령 버퍼 생성
    Fw::ComBuffer buffer = createCommandBuffer(testOpcode, args.getBuffAddr(), args.getBuffLength());
    
    // 명령 전송
    U32 cmdSeq = getNextSequence();
    dispatchRawCommand(buffer, cmdSeq, 0);
    
    // 검증
    if (validateLastCommandStatus(testOpcode, cmdSeq, Fw::CmdResponse(Fw::CmdResponse::OK))) {
        std::cout << "명령 인자 테스트 성공!" << std::endl;
    } else {
        std::cout << "명령 인자 테스트 실패!" << std::endl;
    }
}
