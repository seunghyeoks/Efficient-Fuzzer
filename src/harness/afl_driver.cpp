#include "CmdDispatcherHarness.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    static CmdDispatcherHarness harness;
    static bool initialized = false;
    
    if (!initialized) {
        harness.initialize();
        initialized = true;
    }
    
    return harness.processFuzzedInput(data, size);
}
