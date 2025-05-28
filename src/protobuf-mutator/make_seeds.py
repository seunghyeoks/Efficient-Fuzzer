import cmd_dispatcher_fuzz_input_pb2
import os

os.makedirs("seed", exist_ok=True)

def save_seed(msg, filename):
    with open(os.path.join("seed", filename), "wb") as f:
        f.write(msg.SerializeToString())

# 1. 기본 OK 명령 (NoOp)
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.packet_content.desc_type = 0  # FW_PACKET_COMMAND
action.send_cmd_buffer.packet_content.opcode = 0
action.send_cmd_buffer.packet_content.arguments.arg_data = b""
action.send_cmd_buffer.context = 1
save_seed(msg, "seed_basic_ok.bin")

# 2. 인자가 있는 명령
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.packet_content.desc_type = 0
action.send_cmd_buffer.packet_content.opcode = 2  # TEST_CMD_1
action.send_cmd_buffer.packet_content.arguments.arg_data = b"\x01\x00\x00\x00" + b"\x00\x00\x80\x3f" + b"\x05"
action.send_cmd_buffer.context = 2
save_seed(msg, "seed_with_args_ok.bin")

# 3. 잘못된 명령 (미등록 opcode)
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.packet_content.desc_type = 0
action.send_cmd_buffer.packet_content.opcode = 0x1000
action.send_cmd_buffer.packet_content.arguments.arg_data = b""
action.send_cmd_buffer.context = 3
save_seed(msg, "seed_with_args_invalid.bin")

# 4. 명령 등록 시나리오
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.register_cmd.opcode_to_register = 0x50
save_seed(msg, "seed_register_command.bin")

# 5. 중복 등록 시나리오
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.register_cmd.opcode_to_register = 0x50
action2 = msg.actions.add()
action2.register_cmd.opcode_to_register = 0x50
save_seed(msg, "seed_register_duplicate.bin")

# 6. CLEAR_TRACKING 명령
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.internal_cmd.command_type = cmd_dispatcher_fuzz_input_pb2.InternalCommandScenario.CLEAR_TRACKING
save_seed(msg, "seed_clear_tracking.bin")

# 7. 경계값 opcode
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.packet_content.desc_type = 0
action.send_cmd_buffer.packet_content.opcode = 0xFFFFFFFF
action.send_cmd_buffer.packet_content.arguments.arg_data = b""
action.send_cmd_buffer.context = 4
save_seed(msg, "seed_boundary_opcode.bin")

# 8. 잘못된 형식(직렬화 실패) 시나리오
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.simulate_serialization_failure = True
action.send_cmd_buffer.raw_buffer_override = b"\x00\xff\xff"
action.send_cmd_buffer.context = 5
save_seed(msg, "seed_malformed_command.bin")

# 9. 긴 인자
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.packet_content.desc_type = 0
action.send_cmd_buffer.packet_content.opcode = 1
action.send_cmd_buffer.packet_content.arguments.arg_data = b"A" * 100
action.send_cmd_buffer.context = 6
save_seed(msg, "seed_long_args.bin")

# 10. 기본 에러 응답 시나리오
msg = cmd_dispatcher_fuzz_input_pb2.CmdDispatcherFuzzInput()
action = msg.actions.add()
action.send_cmd_buffer.packet_content.desc_type = 0
action.send_cmd_buffer.packet_content.opcode = 0
action.send_cmd_buffer.packet_content.arguments.arg_data = b""
action.send_cmd_buffer.context = 7
action2 = msg.actions.add()
action2.simulate_cmd_status.opcode = 0
action2.simulate_cmd_status.cmd_seq = 1
action2.simulate_cmd_status.response_enum = cmd_dispatcher_fuzz_input_pb2.CmdResponseEnum.EXECUTION_ERROR
save_seed(msg, "seed_basic_error.bin")
