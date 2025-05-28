#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <Fw/Types/SerialBuffer.hpp> // For Fw::SerialBuffer
#include "../FuzzTester/CmdDispatcherFuzzTester.hpp"
#include "cmd_dispatcher_fuzz_input.pb.h" // Generated protobuf header
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h" // For DEFINE_PROTO_FUZZER
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <fstream> // 파일 출력을 위해 추가
#include <string>  // std::string 사용을 위해 추가
#include <sstream> // 문자열 스트림 사용을 위해 추가
#include <cstdlib>


// Helper function to convert FuzzedCmdPacket (protobuf) to Fw::ComBuffer
// This function attempts to serialize the protobuf message into a ComBuffer.
// Note: This is a simplified serialization. Real F' command serialization is more complex.
void convertProtoToComBuffer(const Svc::FuzzedCmdPacket& proto_pkt, Fw::ComBuffer& com_buffer) {
    // Reset com_buffer before use
    com_buffer.resetSer();

    // Simulate Fw::ComPacket::FW_PACKET_COMMAND descriptor
    FwPacketDescriptorType desc = static_cast<FwPacketDescriptorType>(proto_pkt.desc_type());
    // Forcing FW_PACKET_COMMAND if an unknown type is fuzzed for basic functionality
    if (desc < Fw::ComPacket::FW_PACKET_COMMAND || desc > Fw::ComPacket::FW_PACKET_LOG) { // Basic check, might need more robust handling
        desc = Fw::ComPacket::FW_PACKET_COMMAND;
    }
    com_buffer.serialize(static_cast<FwPacketDescriptorType>(desc));


    // Serialize opcode
    FwOpcodeType opcode = static_cast<FwOpcodeType>(proto_pkt.opcode());
    com_buffer.serialize(opcode);

    // Serialize arguments (copy bytes directly)
    // This assumes arg_data in proto_pkt is already a serialized Fw::CmdArgBuffer
    const std::string& arg_data_str = proto_pkt.arguments().arg_data();
    // Create a temporary Fw::SerialBuffer to hold the argument data for serialization
    // The size of this buffer should be sufficient for the arg_data
    // We cap the size to avoid excessively large buffers.
    const size_t max_arg_size = 256; // Define a reasonable max size for args
    size_t arg_size_to_copy = std::min(arg_data_str.length(), max_arg_size);

    // Create a Fw::ExternalSerializeBuffer for the arguments
    // This buffer will wrap the data from the protobuf message
    // Note: Fw::ComBuffer typically expects to serialize Fw::CmdArgBuffer directly,
    // which itself handles its internal data.
    // Here we are directly pushing bytes, which might not perfectly match all F' internal checks
    // if the fuzzer generates arbitrary byte sequences.

    // Serialize the length of the arguments first (as Fw::CmdArgBuffer would do internally)
    // This is a simplification. Fw::CmdArgBuffer serializes its internal buffer\'s current size.
    // com_buffer.serialize(static_cast<FwBuffSizeType>(arg_size_to_copy));


    // Directly serialize the argument bytes.
    // This is a bit of a hack. Fw::ComBuffer serializes Fw::CmdArgBuffer which contains its own buffer.
    // We are mimicking that by serializing the size and then the data.
    // Fw::CmdArgBuffer's serialize method writes its m_buf.getBuffLength() and then its m_buf.getBuffAddr().
    // For fuzzing, we can try to directly write the fuzzed bytes.
    // The receiver (CmdDispatcher) will deserialize this into an Fw::CmdArgBuffer.

    // Create a temporary Fw::SerialBuffer for arguments
    // Max size for ComBuffer data is Fw::ComBuffer::COM_BUFFER_SIZE
    // Ensure we don't overflow com_buffer
    Fw::SerialBuffer tempArgBuff(const_cast<U8*>(reinterpret_cast<const U8*>(arg_data_str.data())), arg_size_to_copy);
    tempArgBuff.setBuffLen(arg_size_to_copy); // Set the actual data length

    // Serialize the argument buffer into the ComBuffer
    // This mimics how Fw::CmdArgBuffer would be serialized.
    com_buffer.serialize(tempArgBuff);


    // Ensure the ComBuffer's internal size is updated correctly after all serializations
    // The serialize calls should handle this, but good to be aware of.
}


DEFINE_PROTO_FUZZER(const Svc::CmdDispatcherFuzzInput& fuzz_input) {
    NATIVE_INT_TYPE queueDepth = 10; // Default value
    NATIVE_INT_TYPE instance = 0;    // Default value

    // Use values from protobuf if they are somewhat reasonable
    if (fuzz_input.initial_queue_depth() > 0 && fuzz_input.initial_queue_depth() < 100) {
        queueDepth = fuzz_input.initial_queue_depth();
    }
    // Instance ID can be anything, but keep it within a byte for simplicity if fuzzed
    // F\' usually uses small integers for instance IDs.
    if (fuzz_input.initial_instance_id() < 256) {
        instance = fuzz_input.initial_instance_id();
    }

    Svc::CmdDispatcherFuzzTester tester;
    tester.initWithFuzzParams(queueDepth, instance);
    tester.connectPorts();

    // Iterate over the actions defined in the protobuf input
    for (const auto& action : fuzz_input.actions()) {
        switch (action.action_type_case()) {
            case Svc::CmdDispatcherFuzzAction::kSendCmdBuffer: {
                const Svc::CommandBufferInput& cmd_buf_input = action.send_cmd_buffer();
                Fw::ComBuffer com_buffer; // Create an empty ComBuffer

                if (cmd_buf_input.simulate_serialization_failure()) {
                    // Use raw_buffer_override if serialization failure is simulated
                    const std::string& raw_bytes_str = cmd_buf_input.raw_buffer_override();
                    // Ensure data does not exceed com_buffer capacity
                    size_t bytes_to_copy = std::min(raw_bytes_str.length(), static_cast<size_t>(Fw::ComBuffer::COM_BUFFER_SIZE));
                    // Directly copy raw bytes into ComBuffer\'s internal storage
                    // This is low-level and assumes the fuzzer knows ComBuffer structure or is just providing garbage.
                    memcpy(com_buffer.getBuffAddr(), raw_bytes_str.data(), bytes_to_copy);
                    com_buffer.setBuffLen(bytes_to_copy); // Manually set the length
                } else {
                    // Convert the FuzzedCmdPacket part of CommandBufferInput to Fw::ComBuffer
                    convertProtoToComBuffer(cmd_buf_input.packet_content(), com_buffer);
                }

                // Invoke the port with the populated ComBuffer and context
                tester.invoke_to_seqCmdBuff(0, cmd_buf_input.context(), com_buffer);
                break;
            }
            case Svc::CmdDispatcherFuzzAction::kRegisterCmd: {
                const Svc::CommandRegistrationInput& reg_input = action.register_cmd();
                // FuzzTester's add_example_cmd_reg_entries already registers some commands.
                // To make this action more meaningful, we might need a way to
                // register specific opcodes to specific ports if the test setup allows dynamic registration.
                // For now, let\'s call a method on the tester if it has one for dynamic registration,
                // or simply log that we received this action.
                // Assuming tester has a method like: tester.registerOpcode(opcode);
                // If not, this part might need more FuzzTester capabilities.
                // For this example, we'll assume CmdDispatcherFuzzTester can handle this.
                // This is a placeholder for actual registration logic if needed beyond initial setup.
                // Simulating a call, actual implementation in FuzzTester would be needed.
                // tester.dynamicRegisterOpcode(reg_input.opcode_to_register());

                // As a simple test, we can try to dispatch a command with this opcode
                // to see if it's handled (e.g., as invalid if not registered).
                // This is just an example of how one might react to this action.
                Fw::ComBuffer com_buffer;
                Fw::SerializeStatus _status;
                _status = com_buffer.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                FW_ASSERT(_status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(_status));
                _status = com_buffer.serialize(static_cast<FwOpcodeType>(reg_input.opcode_to_register()));
                FW_ASSERT(_status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(_status));
                // Add empty args for simplicity
                Fw::CmdArgBuffer empty_args;
                _status = com_buffer.serialize(empty_args);
                FW_ASSERT(_status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(_status));
                tester.invoke_to_seqCmdBuff(0, 0, com_buffer); // Context 0 for simplicity
                break;
            }
            case Svc::CmdDispatcherFuzzAction::kInternalCmd: {
                // This was intended for CmdDispatcher's own commands like NO_OP.
                // The FuzzTester's tryTest or specific methods might handle these.
                // For now, we can log or ignore, as CmdDispatcher's internal commands
                // are usually tested via its standard command input port (seqCmdBuff).
                // The `tryTest` method from the original LLVMFuzzerTestOneInput might be
                // more relevant if we were to pass raw bytes to it.
                // With protobuf, we are more structured.
                // We can construct a ComBuffer for a known internal command if desired.
                break;
            }
            case Svc::CmdDispatcherFuzzAction::kSimulateCmdStatus: {
                const Svc::CommandStatusInput& status_input = action.simulate_cmd_status();
                FwOpcodeType opcode = status_input.opcode();
                U32 cmd_seq = status_input.use_fuzzed_cmd_seq() ? status_input.fuzzed_cmd_seq() : tester.getCmdSendCmdSeq(); // Assuming getter exists
                Fw::CmdResponse response = static_cast<Fw::CmdResponse>(status_input.response_enum());
                // Ensure enum value is valid for Fw::CmdResponse
                if (response.e < Fw::CmdResponse::OK || response.e > Fw::CmdResponse::BUSY) { // Adjust range based on actual Fw::CmdResponse
                     response = Fw::CmdResponse::EXECUTION_ERROR; // Default to an error
                }
                tester.invoke_to_compCmdStat(0, opcode, cmd_seq, response);
                break;
            }
            case Svc::CmdDispatcherFuzzAction::ACTION_TYPE_NOT_SET:
                // No action set, do nothing.
                break;
            default:
                // Unknown action
                break;
        }
    }
    // Perform any cleanup or final checks on the tester if necessary
    // tester.cleanup(); // Example
}
