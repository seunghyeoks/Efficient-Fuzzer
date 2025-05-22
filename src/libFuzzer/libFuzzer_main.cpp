#include <Fw/Types/Assert.hpp>
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <build-fprime-automatic-native-ut/F-Prime/Svc/CmdDispatcher/CommandDispatcherTesterBase.hpp>

// 퍼징 전용 테스터 클래스 (GTest 없이)
class FuzzTester : public Svc::CommandDispatcherTesterBase {
public:
    FuzzTester() : CommandDispatcherTesterBase("FuzzTester", 100) {
        this->initComponents();
        this->connectPorts();
    }
    
    // CommandDispatcherImpl에 직접 접근할 필요가 있을 때
    Svc::CommandDispatcherImpl& getImpl() {
        return this->component;
    }
    
    // 명령 전송을 위한 간편한 메소드
    void sendFuzzedCommand(const Fw::ComBuffer& buff, U32 context = 0) {
        this->invoke_to_seqCmdBuff(0, buff, context);
        this->component.doDispatch();
    }
    
    // 기존 테스트 코드에서 포트 연결 로직 가져오기
    void connectPorts() {
        // TesterBase에서 상속받은 포트 연결 로직 활용
        // ...
    }
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
        tester.sendFuzzedCommand(buff, context);
        
        // 선택적: 결과 확인 및 추가 로직
        // if (tester.wasCommandHandled() && !tester.wasCommandSuccessful()) {
        //    // 특정 조건에서 추가 테스트 또는 로깅
        // }
    }
    
    return 0;
}