#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "../FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <fstream> // 파일 출력을 위해 추가
#include <string>  // std::string 사용을 위해 추가
#include <sstream> // 문자열 스트림 사용을 위해 추가
#include <cstdlib>

/*
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
            // libFuzzer가 크래시로 인식하도록 trap 사용
            __builtin_trap();
        }
    };
    static FuzzAssertHook g_fuzzAssertHook;
    __attribute__((constructor)) static void registerFuzzAssertHook() {
        g_fuzzAssertHook.registerHook();
    }
} // anonymous namespace
*/

// libFuzzer의 메인 입력 처리 함수
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
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
    tester.tryTest(Data, Size);


    return 0;
}
