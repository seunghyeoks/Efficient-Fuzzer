#include "harness/CmdDispatcherHarness.hpp"

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