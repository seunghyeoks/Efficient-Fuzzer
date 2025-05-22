#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <build-fprime-automatic-native-ut/F-Prime/Svc/CmdDispatcher/CommandDispatcherTesterBase.hpp>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>

// 퍼징 전용 테스터 클래스 (GTest 없이)
class FuzzTester : public Svc::CommandDispatcherTesterBase {
public:
    // 추적을 위한 구조체
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
    
    FuzzTester() : CommandDispatcherTesterBase("FuzzTester", 100) {
        this->initComponents();
        this->connectPorts();
        
        // 명령어 처리기 등록 - CmdDispatcher가 실제 환경처럼 동작하게 함
        for (FwIndexType i = 0; i < 10; i++) {
            this->invoke_to_compCmdReg(0, i + 100); // 100~109 OpCode 등록
        }
    }
    
    // CommandDispatcherImpl에 직접 접근할 필요가 있을 때
    Svc::CommandDispatcherImpl& getImpl() {
        return this->component;
    }
    
    // 초기화 메소드: 각 퍼징 사이클 전 호출
    void resetState() {
        this->clearHistory();
        this->clearEvents();
        this->clearTlm();
        this->m_fuzzResult = FuzzResult();
    }
    
    // 명령 전송을 위한 간편한 메소드
    FuzzResult sendFuzzedCommand(const Fw::ComBuffer& buff, U32 context = 0) {
        this->resetState();
        this->invoke_to_seqCmdBuff(0, const_cast<Fw::ComBuffer&>(buff), context);
        this->component.doDispatch();
        return m_fuzzResult;
    }
    
    // 핑 테스트 메소드
    void sendPing(U32 key) {
        this->invoke_to_pingIn(0, key);
        this->component.doDispatch();
    }
    
    // 명령어 직접 전송 메소드 (seqCmdBuff 우회)
    void sendDirectCommand(FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer& args) {
        Fw::ComBuffer buff;
        // 명령어 패킷 생성
        buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
        buff.serialize(opCode);
        buff.serialize(cmdSeq);
        Fw::SerializeStatus stat = buff.serialize(args);
        FW_ASSERT(stat == Fw::FW_SERIALIZE_OK);
        
        this->invoke_to_seqCmdBuff(0, buff, 0);
        this->component.doDispatch();
    }
    
    // TesterBase 오버라이드: 명령어 응답 핸들러
    void from_seqCmdStatus_handler(
        FwIndexType portNum,
        FwOpcodeType opCode,
        U32 cmdSeq,
        const Fw::CmdResponse& response
    ) override {
        // 부모 클래스 핸들러 호출
        CommandDispatcherTesterBase::from_seqCmdStatus_handler(portNum, opCode, cmdSeq, response);
        
        // 응답 분석 및 기록
        m_fuzzResult.lastOpcode = opCode;
        m_fuzzResult.lastResponse = response;
        m_fuzzResult.allResponses.push_back(response);
        
        if (response != Fw::CmdResponse::OK) {
            m_fuzzResult.hasError = true;
            m_fuzzResult.errorCount++;
        }
    }
    
    // 텔레메트리 핸들러 오버라이드
    void tlmInput_CommandsDispatched(
        const Fw::Time& timeTag,
        const U32 val
    ) override {
        CommandDispatcherTesterBase::tlmInput_CommandsDispatched(timeTag, val);
        m_fuzzResult.dispatchCount = val;
    }
    
    void tlmInput_CommandErrors(
        const Fw::Time& timeTag,
        const U32 val
    ) override {
        CommandDispatcherTesterBase::tlmInput_CommandErrors(timeTag, val);
        m_fuzzResult.errorCount = val;
    }
    
    // 이벤트 핸들러 오버라이드
    void logIn_WARNING_HI_MalformedCommand(
        Fw::DeserialStatus Status
    ) override {
        CommandDispatcherTesterBase::logIn_WARNING_HI_MalformedCommand(Status);
        m_fuzzResult.hasError = true;
    }
    
    void logIn_WARNING_HI_InvalidCommand(
        U32 Opcode
    ) override {
        CommandDispatcherTesterBase::logIn_WARNING_HI_InvalidCommand(Opcode);
        m_fuzzResult.hasError = true;
    }
    
    void logIn_WARNING_HI_TooManyCommands(
        U32 Opcode
    ) override {
        CommandDispatcherTesterBase::logIn_WARNING_HI_TooManyCommands(Opcode);
        m_fuzzResult.hasError = true;
    }
    
    void logIn_COMMAND_OpCodeError(
        U32 Opcode,
        Fw::CmdResponse error
    ) override {
        CommandDispatcherTesterBase::logIn_COMMAND_OpCodeError(Opcode, error);
        m_fuzzResult.hasError = true;
    }
    
private:
    FuzzResult m_fuzzResult;
};

// 퍼징 상태 관리
static FuzzTester* tester = nullptr;

// 특수한 입력 패턴을 생성하는 함수들
Fw::ComBuffer createValidCommandBuffer(FwOpcodeType opcode, U32 cmdSeq) {
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    buff.serialize(opcode);
    buff.serialize(cmdSeq);
    return buff;
}

Fw::ComBuffer createInvalidSizeCommandBuffer(const uint8_t* data, size_t size) {
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    // 최소 필요 크기보다 작게 직렬화
    size_t copySize = std::min(size, static_cast<size_t>(sizeof(FwOpcodeType)-1));
    if (copySize > 0) {
        buff.serialize(data, copySize);
    }
    return buff;
}

// libFuzzer 엔트리 포인트
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    // 첫 호출 시 테스터 초기화
    if (tester == nullptr) {
        tester = new FuzzTester();
    }
    
    // 최소 크기 검사
    if (Size < 2) {
        return 0;
    }
    
    // 입력 데이터로부터 테스트 케이스 유형 결정 (첫 바이트 사용)
    uint8_t testType = Data[0] % 4;
    
    // 다양한 테스트 케이스 실행
    switch (testType) {
        case 0: {
            // 정상적인 명령어 퍼징
            if (Size > sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType)) {
                Fw::ComBuffer buff;
                buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                
                // OpCode 추출
                FwOpcodeType opCode = 0;
                if (Size > 1) {
                    memcpy(&opCode, Data + 1, std::min(sizeof(FwOpcodeType), Size - 1));
                }
                buff.serialize(opCode);
                
                // 데이터 추가
                if (Size > 1 + sizeof(FwOpcodeType)) {
                    size_t dataSize = std::min(Size - 1 - sizeof(FwOpcodeType), static_cast<size_t>(FW_COM_BUFFER_MAX_SIZE/2));
                    buff.serialize(&Data[1 + sizeof(FwOpcodeType)], dataSize);
                }
                
                // 명령 전송
                tester->sendFuzzedCommand(buff, 0);
            }
            break;
        }
        case 1: {
            // 비정상 패킷 유형 퍼징
            if (Size > 1) {
                Fw::ComBuffer buff;
                // 비명령어 패킷 유형 사용
                FwPacketDescriptorType packetType = Data[1] % 3; // 0, 1, 2 중 하나
                if (packetType == Fw::ComPacket::FW_PACKET_COMMAND) {
                    packetType = Fw::ComPacket::FW_PACKET_UNKNOWN;
                }
                buff.serialize(packetType);
                
                // 나머지 데이터 추가
                if (Size > 2) {
                    size_t dataSize = std::min(Size - 2, static_cast<size_t>(FW_COM_BUFFER_MAX_SIZE/2));
                    buff.serialize(&Data[2], dataSize);
                }
                
                tester->sendFuzzedCommand(buff, 0);
            }
            break;
        }
        case 2: {
            // 핑 포트 퍼징
            if (Size > 1) {
                U32 pingKey = 0;
                memcpy(&pingKey, &Data[1], std::min(sizeof(U32), Size - 1));
                tester->sendPing(pingKey);
            }
            break;
        }
        case 3: {
            // 비정상 크기 버퍼 퍼징
            if (Size > 1) {
                Fw::ComBuffer buff = createInvalidSizeCommandBuffer(&Data[1], Size - 1);
                tester->sendFuzzedCommand(buff, 0);
            }
            break;
        }
    }
    
    return 0;
}

// 메인 함수 - 독립 실행을 위한 코드 (libFuzzer와 별개)
int main(int argc, char **argv) {
    // 테스트용 데이터 생성
    uint8_t testData[100];
    for (int i = 0; i < 100; i++) {
        testData[i] = i % 256;
    }
    
    // 수동으로 퍼저 호출
    LLVMFuzzerTestOneInput(testData, 100);
    
    std::cout << "Manual fuzzing test completed." << std::endl;
    return 0;
}