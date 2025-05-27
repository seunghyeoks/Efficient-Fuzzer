#include "CmdDispatcherFuzzTester.hpp"
#include <cstdio>
#include <cstring>
#include <Fw/Com/ComPacket.hpp>      // For Fw::ComPacket
#include <Fw/Com/ComBuffer.hpp>      // For Fw::ComBuffer
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp> // For OPCODE_CMD_CLEAR_TRACKING

namespace Svc {

    void CmdDispatcherFuzzTester::init(NATIVE_INT_TYPE instance) {
        CommandDispatcherTesterBase::init();
    }

    // 생성자: 내부 m_impl을 직접 생성
    CmdDispatcherFuzzTester::CmdDispatcherFuzzTester()
    : CommandDispatcherTesterBase("testerbase", 100), m_impl("CmdDispImpl"), m_fuzzResult(), m_cmdSendRcvd(false), m_cmdSendOpCode(0), m_cmdSendCmdSeq(0), m_seqStatusRcvd(false), m_seqStatusOpCode(0), m_seqStatusCmdSeq(0) {
        // m_cmdSendArgs is default constructed
        // m_seqStatusCmdResponse is default constructed
    }

    // 소멸자
    CmdDispatcherFuzzTester::~CmdDispatcherFuzzTester() {
    }

    // 퍼저의 랜덤 값을 받아서 초기화에 활용
    void CmdDispatcherFuzzTester::initWithFuzzParams(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance) {
        CommandDispatcherTesterBase::init();
        m_impl.init(queueDepth, instance);
        this->connectPorts();
        this->resetState();
    }

    void CmdDispatcherFuzzTester::registerCommands() {
        this->m_impl.regCommands();
    }

    void CmdDispatcherFuzzTester::from_compCmdSend_handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer &args) {
        this->m_cmdSendOpCode = opCode;
        this->m_cmdSendCmdSeq = cmdSeq;
        this->m_cmdSendArgs = args; // shallow copy is okay for CmdArgBuffer if its lifetime is managed
        this->m_cmdSendRcvd = true;
        // Base class handler if it does anything useful:
        // CommandDispatcherTesterBase::from_compCmdSend_handler(portNum, opCode, cmdSeq, args);
    }

    void CmdDispatcherFuzzTester::from_seqCmdStatus_handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdResponse &response) {
        this->m_seqStatusRcvd = true;
        this->m_seqStatusOpCode = opCode;
        this->m_seqStatusCmdSeq = cmdSeq;
        this->m_seqStatusCmdResponse = response;
        
        // Store in FuzzResult as well
        m_fuzzResult.lastOpcode = opCode;
        m_fuzzResult.lastResponse = response;
        m_fuzzResult.allResponses.push_back(response);
        if (response != Fw::CmdResponse::OK) {
            m_fuzzResult.hasError = true;
        }
        // Base class handler:
        // CommandDispatcherTesterBase::from_seqCmdStatus_handler(portNum, opCode, cmdSeq, response);
    }

    void CmdDispatcherFuzzTester::from_pingOut_handler(const NATIVE_INT_TYPE portNum, U32 key) {
        // Handle ping response if necessary for fuzzing logic
        // CommandDispatcherTesterBase::from_pingOut_handler(portNum, key);
    }

    // 포트 연결 설정
    void CmdDispatcherFuzzTester::connectPorts() {
        // Connect to compCmdStat, seqCmdBuff, compCmdReg (inputs to CmdDispatcherImpl)
        this->connect_to_compCmdStat(0, this->m_impl.get_compCmdStat_InputPort(0));
        this->connect_to_seqCmdBuff(0, this->m_impl.get_seqCmdBuff_InputPort(0));
        this->connect_to_compCmdReg(0, this->m_impl.get_compCmdReg_InputPort(0));
        this->connect_to_pingIn(0, this->m_impl.get_pingIn_InputPort(0));

        // Connect from_compCmdSend, from_seqCmdStatus (outputs from CmdDispatcherImpl)
        this->m_impl.set_compCmdSend_OutputPort(0, this->get_from_compCmdSend(0));
        this->m_impl.set_seqCmdStatus_OutputPort(0, this->get_from_seqCmdStatus(0));
        
        // Connect internal command ports for CmdDispatcher's own commands (NO_OP etc.)
        // The second compCmdSend port (index 1) is usually for the component's own internal commands.
        // The CmdDisp input port (index 0) receives these.
        this->m_impl.set_compCmdSend_OutputPort(1, this->m_impl.get_CmdDisp_InputPort(0));

        // The CmdReg output port (index 0) sends registration to its own compCmdReg input port (index 1).
        // This was a bit confusing in the original, let's clarify:
        // For self-registration of its own commands (NO_OP, etc.), CmdDispatcherImpl usually calls its CmdReg port,
        // which should be connected to its *own* compCmdReg_Objects's input port, or it directly registers.
        // If CMD_REG port is used:
        // this->m_impl.set_CmdReg_OutputPort(0, this->m_impl.get_compCmdReg_InputPort(0)); // Port 0 is usually for external components
        // It's more typical that regCommands() calls invoke_to_compCmdReg directly or manipulates m_entryTable.
        // For the purpose of the fuzzer, if regCommands() handles internal registration, this might not be needed.
        // Let's assume regCommands() handles it. If it uses the CmdReg port to self-register on port 0 of compCmdReg:
        // this->m_impl.set_CmdReg_OutputPort(0, this->m_impl.get_compCmdReg_InputPort(0));

        // CmdStatus output port is for command responses from its own commands.
        // This should typically go to its own compCmdStat input port.
        this->m_impl.set_CmdStatus_OutputPort(0, this->m_impl.get_compCmdStat_InputPort(0));

        // Connect telemetry, time, log, and ping output ports from CmdDispatcherImpl
        this->m_impl.set_Tlm_OutputPort(0, this->get_from_Tlm(0));
        this->m_impl.set_Time_OutputPort(0, this->get_from_Time(0));
        this->m_impl.set_Log_OutputPort(0, this->get_from_Log(0));
        this->m_impl.set_LogText_OutputPort(0, this->get_from_LogText(0));
        this->m_impl.set_pingOut_OutputPort(0, this->get_from_pingOut(0));
    }

    // 상태 초기화
    void CmdDispatcherFuzzTester::resetState() {
        // Clear event history from the base tester
        this->clearHistory(); 
        
        // Reset fuzz result structure
        this->m_fuzzResult = FuzzResult(); // This also clears the new eventLog

        // Reset CmdDispatcherImpl internal state if possible/needed.
        // This might involve calling a specific reset command if available,
        // or re-initializing m_impl if that's the strategy.
        // For now, we assume m_impl is reset by re-running regCommands() and clearing its tables.
        // A more robust reset might involve clearing m_impl's tables directly or re-instantiating.
        // For simplicity, we'll rely on CLEAR_TRACKING and re-registering.
        
        // Reset local simulation state variables
        this->m_cmdSendRcvd = false;
        this->m_cmdSendOpCode = 0;
        this->m_cmdSendCmdSeq = 0;
        this->m_cmdSendArgs.resetSer(); // Clear buffer

        this->m_seqStatusRcvd = false;
        this->m_seqStatusOpCode = 0;
        this->m_seqStatusCmdSeq = 0;
        this->m_seqStatusCmdResponse = Fw::CmdResponse(); // Reset to default
    }

    // CommandDispatcherImpl에 직접 접근 (이제 private으로 변경 고려)
    // CommandDispatcherImpl& CmdDispatcherFuzzTester::getImpl() {
    //     return this->m_impl;
    // }

    // 텔레메트리 핸들러 - 명령 디스패치 카운트
    void CmdDispatcherFuzzTester::tlmInput_CommandsDispatched(
        const Fw::Time& timeTag,
        const U32 val
    ) {
        CommandDispatcherTesterBase::tlmInput_CommandsDispatched(timeTag, val);
        m_fuzzResult.dispatchCount = val;
        // EventInfo info("TLM_CommandsDispatched");
        // info.opcode = val; // Re-using opcode field for count
        // m_fuzzResult.eventLog.push_back(info);
    }

    // 텔레메트리 핸들러 - 명령 오류 카운트
    void CmdDispatcherFuzzTester::tlmInput_CommandErrors(
        const Fw::Time& timeTag,
        const U32 val
    ) {
        CommandDispatcherTesterBase::tlmInput_CommandErrors(timeTag, val);
        m_fuzzResult.errorCount = val;
        // EventInfo info("TLM_CommandErrors");
        // info.opcode = val; // Re-using opcode field for count
        // m_fuzzResult.eventLog.push_back(info);
    }

    // --- Event Handlers Modification ---

    void CmdDispatcherFuzzTester::logIn_DIAGNOSTIC_OpCodeRegistered(
        U32 Opcode,
        I32 port,
        I32 slot
    ) {
        CommandDispatcherTesterBase::logIn_DIAGNOSTIC_OpCodeRegistered(Opcode, port, slot);
        EventInfo info("OpCodeRegistered");
        info.opcode = Opcode;
        info.portOrSlot = port; // Storing port, slot could be in another field or combined
        // Example: info.slot = slot; (if EventInfo has a slot field)
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_DIAGNOSTIC_OpCodeReregistered(
        U32 Opcode,
        I32 port
    ) {
        CommandDispatcherTesterBase::logIn_DIAGNOSTIC_OpCodeReregistered(Opcode, port);
        m_fuzzResult.opCodeReregistered = true;
        EventInfo info("OpCodeReregistered");
        info.opcode = Opcode;
        info.portOrSlot = port;
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_COMMAND_OpCodeDispatched(
        U32 Opcode,
        I32 port
    ) {
        CommandDispatcherTesterBase::logIn_COMMAND_OpCodeDispatched(Opcode, port);
        EventInfo info("OpCodeDispatched");
        info.opcode = Opcode;
        info.portOrSlot = port;
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_COMMAND_OpCodeCompleted(
        U32 Opcode
    ) {
        CommandDispatcherTesterBase::logIn_COMMAND_OpCodeCompleted(Opcode);
        EventInfo info("OpCodeCompleted");
        info.opcode = Opcode;
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_COMMAND_OpCodeError(
        U32 Opcode,
        const Fw::CmdResponse& error // Make sure it's const Fw::CmdResponse&
    ) {
        CommandDispatcherTesterBase::logIn_COMMAND_OpCodeError(Opcode, error);
        m_fuzzResult.hasError = true;
        EventInfo info("OpCodeError");
        info.opcode = Opcode;
        info.response = error;
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_WARNING_HI_MalformedCommand(
        Fw::DeserialStatus Status
    ) {
        CommandDispatcherTesterBase::logIn_WARNING_HI_MalformedCommand(Status);
        m_fuzzResult.hasError = true;
        EventInfo info("MalformedCommand");
        info.desStatus = Status; // Store the DeserialStatus
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_WARNING_HI_InvalidCommand(
        U32 Opcode
    ) {
        CommandDispatcherTesterBase::logIn_WARNING_HI_InvalidCommand(Opcode);
        m_fuzzResult.hasError = true;
        EventInfo info("InvalidCommand");
        info.opcode = Opcode;
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_WARNING_HI_TooManyCommands(
        U32 Opcode
    ) {
        CommandDispatcherTesterBase::logIn_WARNING_HI_TooManyCommands(Opcode);
        m_fuzzResult.hasError = true;
        EventInfo info("TooManyCommands");
        info.opcode = Opcode;
        m_fuzzResult.eventLog.push_back(info);
    }
    
    void CmdDispatcherFuzzTester::logIn_ACTIVITY_HI_NoOpReceived() {
        CommandDispatcherTesterBase::logIn_ACTIVITY_HI_NoOpReceived();
        EventInfo info("NoOpReceived");
        m_fuzzResult.eventLog.push_back(info);
    }

    void CmdDispatcherFuzzTester::logIn_ACTIVITY_HI_NoOpStringReceived(const Fw::LogStringArg& message) {
        CommandDispatcherTesterBase::logIn_ACTIVITY_HI_NoOpStringReceived(message);
        EventInfo info("NoOpStringReceived");
        // info.eventStringArg = message; // If EventInfo is extended to hold Fw::LogStringArg
        m_fuzzResult.eventLog.push_back(info);
    }
    
    void CmdDispatcherFuzzTester::logIn_ACTIVITY_HI_TestCmd1Args(I32 arg1, F32 arg2, U8 arg3) {
        CommandDispatcherTesterBase::logIn_ACTIVITY_HI_TestCmd1Args(arg1, arg2, arg3);
        EventInfo info("TestCmd1Args");
        // Store args in EventInfo if needed for analysis
        // info.param_i32 = arg1; info.param_f32 = arg2; info.param_u8 = arg3;
        m_fuzzResult.eventLog.push_back(info);
    }

    // Helper to dispatch messages if component is active
    // NOTE: This is a simplified way. In a real F Prime component, message dispatch
    // is handled by the active component's thread and message queue (Os::Queue).
    // For a unit/fuzz test, `doDispatch()` on the Impl directly processes one message.
    // If `m_impl` is not derived from `QueuedComponentBase` or `ActiveComponentBase`
    // in a way that `doDispatch()` is meaningful without a full OS environment,
    // this might need adjustment or reliance on synchronous port calls triggering handlers.
    // Assuming `CommandDispatcherImpl` has a `doDispatch` that processes its queue.
    void FuzzDoDispatch(Svc::CommandDispatcherImpl& impl) {
        // Check if the component has a message queue and is active.
        // This is a conceptual check. The actual mechanism depends on F Prime's
        // testing framework or how `impl.doDispatch()` is supposed to work in isolation.
        // For fprime unit tests, `doDispatch()` typically pulls a message from the queue
        // and calls the handler.
        // If `m_impl.m_msgService` was a public member or had a getter:
        // if (impl.m_msgService != nullptr && impl.m_msgService->isStarted()) {
        // For simple sequential testing, calling doDispatch might be okay.
        // However, be cautious about state if it's meant for an active component.
        (void)impl.doDispatch(); // Call it, assuming it's safe in this test context
        // }
    }

    Svc::CmdDispatcherFuzzTester::FuzzResult CmdDispatcherFuzzTester::tryTest(
        const uint8_t* data,
        size_t size
    ) {
        this->resetState(); 
        this->registerCommands(); // Register NO_OP etc.

        // Ensure there's enough data for basic operations
        if (size < 6) { // Min: 4 bytes for opcode, 1 for scenario, 1 for response type
            m_fuzzResult.hasError = true;
            // EventInfo info("FuzzTest_InsufficientData");
            // m_fuzzResult.eventLog.push_back(info);
            return this->getFuzzResult();
        }
        
        // Data layout:
        // Bytes 0-3: Opcode for most scenarios
        // Byte 4: Test Scenario selector
        // Byte 5: Simulated CmdResponse type for NORMAL_COMMAND
        // Bytes 6-9: Context value for commands
        // Bytes 10+: Command arguments for NORMAL_COMMAND or other scenario-specific data

        FwOpcodeType fuzzed_opcode = 0;
        // Opcode (bytes 0-3)
        memcpy(&fuzzed_opcode, data, sizeof(FwOpcodeType));
        
        U32 context = 0; // Default context
        if (size >= 10) { // Context (bytes 6-9)
            memcpy(&context, data + 6, sizeof(U32));
        }

        // Test Scenario (byte 4)
        enum TestScenario {
            NORMAL_COMMAND = 0,
            REGISTER_COMMAND = 1,
            INVALID_OPCODE_SCENARIO = 2,
            MALFORMED_COMMAND_SCENARIO = 3,
            CLEAR_TRACKING_SCENARIO = 4,
            MAX_SCENARIOS // For modulo
        };
        TestScenario scenario = static_cast<TestScenario>(data[4] % MAX_SCENARIOS);
        
        // Simulated CmdResponse type (byte 5) for NORMAL_COMMAND scenario
        Fw::CmdResponse sim_response_type = Fw::CmdResponse::OK;
        if (size > 5) {
             switch (data[5] % 4) {
                case 0: sim_response_type = Fw::CmdResponse::OK; break;
                case 1: sim_response_type = Fw::CmdResponse::EXECUTION_ERROR; break;
                case 2: sim_response_type = Fw::CmdResponse::INVALID_OPCODE; break;
                case 3: sim_response_type = Fw::CmdResponse::VALIDATION_ERROR; break;
            }
        }
        
        // Command arguments start after context (e.g., from byte 10)
        const uint8_t* arg_data = nullptr;
        size_t arg_size = 0;
        if (size > 10) {
            arg_data = data + 10;
            arg_size = size - 10;
        }

        // 시나리오별 테스트 실행
        switch (scenario) {
            case REGISTER_COMMAND: {
                // Use fuzzed_opcode directly or modify it slightly
                FwOpcodeType reg_opcode = fuzzed_opcode;
                if (arg_size > 0 && arg_data != nullptr) { // Use first byte of arg_data to tweak opcode
                    reg_opcode += arg_data[0];
                }
                this->invoke_to_compCmdReg(0, reg_opcode);
                FuzzDoDispatch(this->m_impl);
                break;
            }
            
            case INVALID_OPCODE_SCENARIO: {
                Fw::ComBuffer buff;
                buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                buff.serialize(fuzzed_opcode + 0x10000); // Add a large offset to make it likely invalid
                if (arg_data && arg_size > 0) { // Optionally serialize some args too
                     buff.serialize(arg_data, (arg_size > 16 ? 16 : arg_size)); // Limit arg size
                }
                this->invoke_to_seqCmdBuff(0, buff, context);
                FuzzDoDispatch(this->m_impl);
                break;
            }
            
            case MALFORMED_COMMAND_SCENARIO: {
                Fw::ComBuffer buff;
                // Use a byte from arg_data to decide malformation type
                uint8_t malform_type = 0;
                if (arg_data && arg_size > 0) {
                    malform_type = arg_data[0];
                }

                if (malform_type % 3 == 0) {
                    // Invalid packet descriptor
                    buff.serialize(static_cast<FwPacketDescriptorType>(0xFF)); // Bogus type
                    buff.serialize(fuzzed_opcode); // Still serialize opcode
                } else if (malform_type % 3 == 1) {
                    // Valid descriptor, but incomplete opcode
                    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                    // Serialize only part of the opcode
                    U16 partial_opcode = static_cast<U16>(fuzzed_opcode & 0xFFFF);
                    buff.serialize(partial_opcode);
                } else {
                     // Valid descriptor, opcode, but args are garbage (or too short)
                    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                    buff.serialize(fuzzed_opcode);
                    if (arg_data && arg_size > 1) { // Use remaining arg_data as potentially malformed args
                        // Serialize a small, possibly incomplete, portion of args
                        buff.serialize(arg_data + 1, (arg_size -1 > 8 ? 8 : arg_size -1), true); // Allow partial
                    }
                }
                this->invoke_to_seqCmdBuff(0, buff, context);
                FuzzDoDispatch(this->m_impl);
                break;
            }
            
            case CLEAR_TRACKING_SCENARIO: {
                // 1. Dispatch a normal command first to potentially populate tracking table
                Fw::ComBuffer buff1;
                buff1.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                buff1.serialize(fuzzed_opcode);
                if (arg_data && arg_size > 0) {
                    buff1.serialize(arg_data, (arg_size > 64 ? 64 : arg_size));
                }
                this->invoke_to_seqCmdBuff(0, buff1, context);
                FuzzDoDispatch(this->m_impl);
                
                // Simulate receiving its status to free up tracker if it was a non-async cmd
                if (this->m_cmdSendRcvd) {
                    this->invoke_to_compCmdStat(0, this->m_cmdSendOpCode, this->m_cmdSendCmdSeq, Fw::CmdResponse::OK);
                    FuzzDoDispatch(this->m_impl);
                    this->m_cmdSendRcvd = false; // Reset for next potential command
                }

                // 2. Then dispatch CLEAR_TRACKING
                Fw::ComBuffer buff2;
                buff2.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                buff2.serialize(static_cast<FwOpcodeType>(Svc::CommandDispatcherImpl::OPCODE_CMD_CLEAR_TRACKING));
                // CLEAR_TRACKING has no args
                this->invoke_to_seqCmdBuff(0, buff2, context + 1); // Use a different context
                FuzzDoDispatch(this->m_impl);
                // Its response is also handled by compCmdStat
                if (this->m_cmdSendRcvd && this->m_cmdSendOpCode == Svc::CommandDispatcherImpl::OPCODE_CMD_CLEAR_TRACKING) {
                     this->invoke_to_compCmdStat(0, this->m_cmdSendOpCode, this->m_cmdSendCmdSeq, Fw::CmdResponse::OK);
                     FuzzDoDispatch(this->m_impl);
                     this->m_cmdSendRcvd = false;
                }
                break;
            }
            
            case NORMAL_COMMAND:
            default: {
                Fw::ComBuffer buff;
                buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                buff.serialize(fuzzed_opcode);

                if (arg_data && arg_size > 0) {
                    buff.serialize(arg_data, (arg_size > 64 ? 64 : arg_size)); // Limit arg length
                }
                
                this->m_cmdSendRcvd = false; // Reset before sending
                this->invoke_to_seqCmdBuff(0, buff, context);
                FuzzDoDispatch(this->m_impl);
                
                // If command was dispatched (m_cmdSendRcvd is true), simulate its response
                if (this->m_cmdSendRcvd) {
                    // Use the opcode and seq that was actually sent out via compCmdSend port
                    this->invoke_to_compCmdStat(0, this->m_cmdSendOpCode, this->m_cmdSendCmdSeq, sim_response_type);
                    FuzzDoDispatch(this->m_impl);
                    this->m_cmdSendRcvd = false; // Reset for next potential command
                } else {
                    // If cmdSend was not called, it might be an invalid opcode already handled by seqCmdBuff_handler
                    // or some other immediate error. The events would have been logged.
                }
                break;
            }
        }

        // --- Optional: Analyze eventLog here ---
        for (const auto& event : m_fuzzResult.eventLog) {
            if (event.name == "OpCodeError" || 
                event.name == "MalformedCommand" || 
                event.name == "InvalidCommand" || 
                event.name == "TooManyCommands") {
                m_fuzzResult.hasError = true; // Ensure hasError is set if critical events occurred
                break;
            }
        }
        // Add more sophisticated event analysis if needed.
        // For example, checking for missing OpCodeCompleted after OpCodeDispatched for non-async commands.

        return this->getFuzzResult();
    }

} // namespace Svc
