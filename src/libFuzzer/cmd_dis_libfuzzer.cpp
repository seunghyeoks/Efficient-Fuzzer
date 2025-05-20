#include "../harness/CmdDispatcherHarness.hpp"

// 전역 하네스 인스턴스
static CmdDispatcherHarness g_harness("FuzzDispatcher");
static bool g_initialized = false;

// libFuzzer 진입점
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // 첫 호출 시 초기화 (한 번만 수행)
    if (!g_initialized) {
        g_harness.initialize(100, 100);
        
        // 테스트 컴포넌트 생성 및 등록
        static TestCommandComponent testComp("FuzzTestComponent");
        testComp.init();
        g_harness.registerTestComponent(0, &testComp);
        
        // 몇 가지 명령 등록 (퍼징에 필요한 기본 명령)
        g_harness.registerCommand(0x1001, 0);
        g_harness.registerCommand(0x1002, 0);
        g_harness.registerCommand(0x1003, 0);
        
        g_initialized = true;
    }
    
    // 퍼징 데이터 처리
    return g_harness.processFuzzedInput(data, size);
}