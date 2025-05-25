#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <fstream> // 파일 출력을 위해 추가
#include <string>  // std::string 사용을 위해 추가
#include <sstream> // 문자열 스트림 사용을 위해 추가
#include <cstdlib>

namespace {
    // Fuzzer 전용 assert 훅
    class FuzzAssertHook : public Fw::AssertHook {
    public:
        void reportAssert(
            FILE_NAME_ARG file,
            NATIVE_UINT_TYPE lineNo,
            NATIVE_UINT_TYPE numArgs,
            FwAssertArgType arg1,
            FwAssertArgType arg2,
            FwAssertArgType arg3,
            FwAssertArgType arg4,
            FwAssertArgType arg5,
            FwAssertArgType arg6
        ) override {
            // 기본 assert 메시지 출력
            Fw::AssertHook::reportAssert(file, lineNo, numArgs,
                                         arg1, arg2, arg3,
                                         arg4, arg5, arg6);
            // TODO: 필요 시 전역 플래그 설정 등 추가 로직 작성
        }
        void doAssert() override {
            // abort 대신 단순 리턴하여 퍼징 지속
        }
    };
    static FuzzAssertHook g_fuzzAssertHook;
    __attribute__((constructor)) static void registerFuzzAssertHook() {
        g_fuzzAssertHook.registerHook();
    }
} // anonymous namespace

// libFuzzer의 메인 입력 처리 함수
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    std::cout << "[FUZZER_LOG] LLVMFuzzerTestOneInput START - Size: " << Size << std::endl;

    // Fuzzing 대상 컴포넌트인 Svc::CommandDispatcherImpl 객체를 생성합니다.
    // "CmdDispImpl"은 컴포넌트 인스턴스 이름입니다.
    Svc::CommandDispatcherImpl impl("CmdDispImpl");
    // CommandDispatcherImpl 컴포넌트를 초기화합니다.
    // 첫 번째 인자는 명령어 큐의 깊이, 두 번째 인자는 인스턴스 ID (보통 0)입니다.
    impl.init(10, 0); 

    // Fuzzing을 위한 테스트 하네스 Svc::CmdDispatcherFuzzTester 객체를 생성합니다.
    // 이 객체는 CommandDispatcherImpl 컴포넌트와 상호작용합니다.
    Svc::CmdDispatcherFuzzTester tester(impl);
    // Fuzz 테스터를 초기화합니다.
    tester.init();
    // Fuzz 테스터와 CommandDispatcherImpl 컴포넌트 간의 포트를 연결합니다.
    tester.connectPorts();

    // CommandDispatcherImpl 컴포넌트에 내장된 기본 명령어들을 등록합니다.
    // (예: NoOp, Health Ping 등)
    impl.regCommands();


    // Fuzzer로부터 받은 입력 데이터(Size)가 너무 작으면 (1바이트 미만) 처리를 건너뜁니다.
    if (Size < 1) {
        std::cout << "[FUZZER_LOG] Input size < 1, returning." << std::endl;
        return 0; 
    }

    /* 1. 첫 바이트를 사용해 전략 선택 (선택 사항)
    FuzzStrategy strategy = Size > 0 ? 
        static_cast<FuzzStrategy>(Data[0] % NUM_STRATEGIES) : 
        STRATEGY_DEFAULT;
    Fw::ComBuffer buff = tester.createFuzzedCommandBufferWithStrategy(Data, Size, strategy);
    */ 
    
    // 2. 퍼저 입력으로부터 바로 명령어 버퍼 생성
    Fw::ComBuffer buff = tester.createFuzzedCommandBuffer(Data, Size);
    
    // context 값 추출 (입력 데이터에서 8~11번째 바이트 사용, 없으면 0)
    U32 context = 0;
    if (Size >= 12) {
        context = static_cast<U32>(Data[8]) |
                  (static_cast<U32>(Data[9]) << 8) |
                  (static_cast<U32>(Data[10]) << 16) |
                  (static_cast<U32>(Data[11]) << 24);
    }

    std::cout << "[FUZZER_LOG] Context: " << context << std::endl;
    // 버퍼 내용의 일부를 확인하기 위한 로그 (Opcode 등)
    // 주의: buff.getBuffAddr()는 유효한 경우에만 접근해야 합니다.
    // FwPacketDescriptorType의 크기와 FwOpcodeType의 크기를 고려하여 Opcode 위치를 계산합니다.
    // getBuffLength()가 충분히 큰지 확인 후 접근합니다.
    if (buff.getBuffLength() >= (sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType))) {
        // Fw::ComPacket::FW_PACKET_COMMAND 이후의 Opcode 값을 가정합니다.
        // 실제 Opcode 위치는 패킷 구조에 따라 다를 수 있습니다.
        // 여기서는 단순 예시로 패킷 디스크립터 바로 다음에 Opcode가 온다고 가정합니다.
        // Fw::ComBuffer는 직렬화된 데이터를 담고 있으므로, deserialize를 통해 Opcode를 얻는 것이 더 정확합니다.
        // 여기서는 간단히 바이트 배열 접근으로 표시합니다.
        // FwOpcodeType extractedOpcode;
        // Fw::SerializeStatus deserStatus = buff.deserialize(extractedOpcode); // 패킷 디스크립터 이후에
        // if (deserStatus == Fw::FW_SERIALIZE_OK) {
        //    std::cout << "[FUZZER_LOG] Buffer (potential Opcode): 0x" << std::hex << extractedOpcode << std::dec << std::endl;
        // }
    }
    std::cout << "[FUZZER_LOG] Buffer length: " << buff.getBuffLength() << std::endl;


    std::cout << "[FUZZER_LOG] Invoking seqCmdBuff..." << std::endl;
    // 생성된 명령어 버퍼 전송
    tester.public_invoke_to_seqCmdBuff(0, buff, context);

    std::cout << "[FUZZER_LOG] Calling public_doDispatchLoop()..." << std::endl;
    tester.public_doDispatchLoop();
    std::cout << "[FUZZER_LOG] public_doDispatchLoop() returned." << std::endl;

    // Fuzz 테스터로부터 명령어 처리 결과를 가져옵니다.
    const auto& result = tester.getFuzzResult();
    std::cout << "[FUZZER_LOG] Fuzz result - hasError: " << (result.hasError ? "true" : "false")
              << ", lastOpcode: 0x" << std::hex << result.lastOpcode << std::dec
              << ", lastResponse: " << result.lastResponse.e << std::endl;


    // 만약 Fuzz 테스터가 명령어 처리 중 오류를 감지했다면
    if (result.hasError) {
        // 파일 이름을 위한 간단한 카운터 (실제 사용 시에는 더 견고한 방법 권장)
        static int error_file_counter = 0;
        std::ostringstream filename_stream;
        // findings 디렉토리 아래에 저장 (entrypoint 스크립트의 -artifact_prefix와 유사한 경로)
        filename_stream << "findings/logical_error_" << error_file_counter++ << "_op" << result.lastOpcode << "_resp" << result.lastResponse.e << ".dat";
        std::string error_filename = filename_stream.str();

        std::ofstream outfile(error_filename, std::ios::binary);
        if (outfile.is_open()) {
            // 원본 입력 데이터 저장
            outfile.write(reinterpret_cast<const char*>(Data), Size);
            // 추가적인 에러 정보도 파일에 기록 가능 (예: 텍스트 형태로)
            outfile << "\n--- Error Info ---\n";
            outfile << "Input Size (Original): " << Size << "\n";
            // outfile << "Input Size (Clamped): " << Size << "\n"; // Clamped size 정보가 있다면 추가
            outfile << "Last Opcode: 0x" << std::hex << result.lastOpcode << std::dec << "\n";
            outfile << "Last Response: " << result.lastResponse.e << "\n";
            outfile.close();
        }
        std::cout << "[FUZZER_LOG] Logical error detected and saved to " << error_filename << std::endl;

        tester.resetState();

        // Fuzzer가 에러를 감지하도록 하기 위해, 필요시 여기서 abort() 또는 FW_ASSERT(false) 호출
        // 예를 들어, 특정 논리적 에러도 크래시처럼 취급하고 싶다면 FW_ASSERT(false)를 사용할 수 있습니다.
        // FW_ASSERT(0, result.lastOpcode, result.lastResponse.e);
    }

    std::cout << "[FUZZER_LOG] LLVMFuzzerTestOneInput END" << std::endl;
    return 0;
}
