#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cassert>
#include "harness/CmdDispatcherHarness.hpp"
#include <Fw/Types/Assert.hpp>
#include <unistd.h> // getenv 함수 사용을 위해 추가

// 전역 변수로 longjmp를 위한 환경 설정
thread_local jmp_buf s_jumpBuffer;
thread_local bool s_inFuzzer = false;

void handle_abort(int sig) {
    fprintf(stderr, "[fuzz] caught abort/assert (signal %d), continue fuzzing\n", sig);
    if (s_inFuzzer) {
        // longjmp를 사용하여 퍼징 함수로 복귀
        longjmp(s_jumpBuffer, 1);
    }
}

/**
 * F-Prime의 Assert 훅을 커스텀하게 구현한 클래스
 * assert가 발생해도 프로세스가 종료되지 않고 퍼징을 계속하도록 함
 */
class FuzzAssertHook : public Fw::AssertHook {
public:
    FuzzAssertHook() {
        // 훅 등록
        registerHook();
    }

    ~FuzzAssertHook() {
        // 훅 등록 해제
        deregisterHook();
    }

    // assert 메시지 출력 오버라이드
    void printAssert(const CHAR* msg) override {
        fprintf(stderr, "[fuzz-assert] %s\n", msg);
    }

    // assert 동작 오버라이드
    void doAssert() override {
        if (s_inFuzzer) {
            // 퍼징 중인 경우 longjmp로 복귀
            longjmp(s_jumpBuffer, 1);
        } else {
            // 퍼징 중이 아닌 경우는 기본 동작
            std::abort(); // assert(0) 대신 std::abort() 사용
        }
    }
};

// 전역 Assert 훅 인스턴스
static FuzzAssertHook g_fuzzAssertHook;

__attribute__((constructor))
void setup_signal_handler() {
    // 환경 변수 확인 및 설정
    const char* queuePlatform = getenv("OS_QUEUE_PLATFORM");
    fprintf(stderr, "[fuzz] OS_QUEUE_PLATFORM=%s\n", queuePlatform ? queuePlatform : "not set");
    
    // 명시적으로 환경 변수 설정 (이미 설정되어 있더라도 덮어씀)
    setenv("OS_QUEUE_PLATFORM", "posix", 1);
    fprintf(stderr, "[fuzz] OS_QUEUE_PLATFORM 환경 변수를 'posix'로 설정\n");
    
    signal(SIGABRT, handle_abort);
    // 다른 치명적인 신호도 처리
    signal(SIGSEGV, handle_abort);
    signal(SIGBUS, handle_abort);
    signal(SIGILL, handle_abort);
    signal(SIGFPE, handle_abort);
}

// libFuzzer 퍼징 진입점
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    // OS_QUEUE_PLATFORM 확인
    const char* queuePlatform = getenv("OS_QUEUE_PLATFORM");
    if (!queuePlatform || strcmp(queuePlatform, "posix") != 0) {
        fprintf(stderr, "[fuzz] 경고: OS_QUEUE_PLATFORM이 'posix'가 아닙니다: %s\n", 
                queuePlatform ? queuePlatform : "not set");
        setenv("OS_QUEUE_PLATFORM", "posix", 1);
    }
    
    // 정적 하네스 인스턴스 - 매번 재생성하지 않고 재사용
    static CmdDispatcherHarness harness("FuzzHarness");
    static bool isInitialized = false;
    static TestCommandComponent testComp("TestComponent");
    
    // 최초 1회만 초기화 수행
    if (!isInitialized) {
        // 초기화 전 환경 변수 다시 확인
        fprintf(stderr, "[fuzz] 초기화 직전 OS_QUEUE_PLATFORM=%s\n", 
                getenv("OS_QUEUE_PLATFORM") ? getenv("OS_QUEUE_PLATFORM") : "not set");
        
        // 하네스 초기화 (명령 큐 크기와 최대 등록수 줄임)
        harness.initialize(5, 5);
        testComp.init();
        harness.registerTestComponent(0, &testComp);
        
        // 몇 가지 기본 명령 등록
        FwOpcodeType validOpcodes[] = {0x1001, 0x1002, 0x1003, 0x1004};
        for (auto opcode : validOpcodes) {
            harness.registerCommand(opcode, 0);
            testComp.setCommandResponse(opcode, Fw::CmdResponse(Fw::CmdResponse::OK));
        }
        
        isInitialized = true;
        fprintf(stderr, "[fuzz] Harness initialized\n");
        fprintf(stderr, "[fuzz] queueDepth=5, maxRegistrations=5\n");

        // === 정상 입력 테스트 코드 수정 ===
        fprintf(stderr, "[fuzz] 정상 입력 테스트 시작\n");
        // 0x1001 명령어, 인자 없음
        Fw::ComBuffer normalBuffer = harness.createCommandBuffer(0x1001);
        int result = harness.processFuzzedInput(normalBuffer.getBuffAddr(), normalBuffer.getBuffLength());
        fprintf(stderr, "[fuzz] 정상 입력 테스트 결과: %d\n", result);
        fprintf(stderr, "[fuzz] 정상 입력 테스트 종료\n");
        // ================================
    }
    
    // setjmp를 사용하여 assert 발생 시 이 지점으로 돌아올 수 있게 함
    s_inFuzzer = true;
    if (setjmp(s_jumpBuffer) == 0) {
        // 정상 실행 흐름
        return harness.processFuzzedInput(Data, Size);
    } else {
        // assert로 인해 longjmp로 복귀한 경우
        fprintf(stderr, "[fuzz] recovered from assert, continuing with next input\n");
        s_inFuzzer = false;
        return 0;
    }
}