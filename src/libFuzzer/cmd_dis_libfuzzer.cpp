#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cassert>
#include "harness/CmdDispatcherHarness.hpp"
#include <Fw/Types/Assert.hpp>

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
    signal(SIGABRT, handle_abort);
    // 다른 치명적인 신호도 처리
    signal(SIGSEGV, handle_abort);
    signal(SIGBUS, handle_abort);
    signal(SIGILL, handle_abort);
    signal(SIGFPE, handle_abort);
}

// libFuzzer 퍼징 진입점
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    static CmdDispatcherHarness harness("FuzzHarness");
    static bool initialized = false;

    // 첫 호출 시 하네스 초기화
    if (!initialized) {
        harness.initialize(100, 100);
        initialized = true;
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