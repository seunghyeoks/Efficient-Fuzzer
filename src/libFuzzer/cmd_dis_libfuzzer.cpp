#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "harness/CmdDispatcherHarness.hpp"

void handle_abort(int sig) {
    fprintf(stderr, "[fuzz] caught abort/assert (signal %d), continue fuzzing\n", sig);
    // crash로 기록만 하고, 프로세스는 죽이지 않음
}

__attribute__((constructor))
void setup_signal_handler() {
    signal(SIGABRT, handle_abort);
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

    // 퍼징 입력 처리
    return harness.processFuzzedInput(Data, Size);
}