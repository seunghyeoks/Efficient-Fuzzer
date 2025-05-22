#include <Fw/Types/Assert.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include "FuzzTester/CmdDispatcherFuzzTester.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

// 퍼징 상태 관리
static Svc::CmdDispatcherFuzzTester* tester = nullptr;

// 특수한 입력 패턴을 생성하는 함수들
Fw::ComBuffer createValidCommandBuffer(FwOpcodeType opcode, U32 cmdSeq) {
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    buff.serialize(opcode);
    buff.serialize(cmdSeq);
    return buff;
}

Fw::ComBuffer createInvalidSizeCommandBuffer(const uint8_t* data, size_t size) {
    Fw::ComBuffer buff;
    buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
    // 최소 필요 크기보다 작게 직렬화
    size_t copySize = std::min(size, static_cast<size_t>(sizeof(FwOpcodeType)-1));
    if (copySize > 0) {
        buff.serialize(data, copySize);
    }
    return buff;
}

// libFuzzer 엔트리 포인트
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    // 첫 호출 시 테스터 초기화
    if (tester == nullptr) {
        tester = new Svc::CmdDispatcherFuzzTester();
        tester->init(10, 0);
        
        // 명령어 핸들러 등록 (10개의 명령어)
        tester->registerCommands(10, 0x100);
    }
    
    // 최소 크기 검사
    if (Size < 2) {
        return 0;
    }
    
    // 입력 데이터로부터 테스트 케이스 유형 결정 (첫 바이트 사용)
    uint8_t testType = Data[0] % 4;
    
    // 다양한 테스트 케이스 실행
    switch (testType) {
        case 0: {
            // 정상적인 명령어 퍼징
            if (Size > sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType)) {
                Fw::ComBuffer buff;
                buff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_COMMAND));
                
                // OpCode 추출
                FwOpcodeType opCode = 0;
                if (Size > 1) {
                    memcpy(&opCode, Data + 1, std::min(sizeof(FwOpcodeType), Size - 1));
                }
                buff.serialize(opCode);
                
                // 데이터 추가
                if (Size > 1 + sizeof(FwOpcodeType)) {
                    size_t dataSize = std::min(Size - 1 - sizeof(FwOpcodeType), static_cast<size_t>(FW_COM_BUFFER_MAX_SIZE/2));
                    buff.serialize(&Data[1 + sizeof(FwOpcodeType)], dataSize);
                }
                
                // 명령 전송
                tester->dispatchFuzzedCommand(buff, 0);
            }
            break;
        }
        case 1: {
            // 비정상 패킷 유형 퍼징
            if (Size > 1) {
                Fw::ComBuffer buff;
                // 비명령어 패킷 유형 사용
                FwPacketDescriptorType packetType = Data[1] % 3; // 0, 1, 2 중 하나
                if (packetType == Fw::ComPacket::FW_PACKET_COMMAND) {
                    packetType = Fw::ComPacket::FW_PACKET_UNKNOWN;
                }
                buff.serialize(packetType);
                
                // 나머지 데이터 추가
                if (Size > 2) {
                    size_t dataSize = std::min(Size - 2, static_cast<size_t>(FW_COM_BUFFER_MAX_SIZE/2));
                    buff.serialize(&Data[2], dataSize);
                }
                
                tester->dispatchFuzzedCommand(buff, 0);
            }
            break;
        }
        case 2: {
            // 핑 포트 퍼징
            if (Size > 1) {
                U32 pingKey = 0;
                memcpy(&pingKey, &Data[1], std::min(sizeof(U32), Size - 1));
                tester->sendPing(pingKey);
            }
            break;
        }
        case 3: {
            // 비정상 크기 버퍼 퍼징
            if (Size > 1) {
                Fw::ComBuffer buff = createInvalidSizeCommandBuffer(&Data[1], Size - 1);
                tester->dispatchFuzzedCommand(buff, 0);
            }
            break;
        }
    }
    
    return 0;
}