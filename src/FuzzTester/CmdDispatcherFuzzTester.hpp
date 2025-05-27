// CommandDispatcherTesterBase를 상속하여, Fuzz Testing 입력을 CmdDispatcher에 전달하는 harness 클래스

#ifndef CMD_DISPATCHER_FUZZ_TESTER_HPP
#define CMD_DISPATCHER_FUZZ_TESTER_HPP

#include "F-Prime/Svc/CmdDispatcher/CommandDispatcherTesterBase.hpp"
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <vector>
#include <string> // For event names or simplified event info
#include "F-Prime/Cmd/CmdResponse.hpp"
#include <Fw/ComSer/DeserialStatus.hpp>
#include <Fw/Log/LogString.hpp> // For Fw::LogStringArg if used in events
#include <Fw/Time/Time.hpp>     // For Fw::Time if used in events

namespace Svc {

    class CmdDispatcherFuzzTester : public CommandDispatcherTesterBase {
        public:
            // Constructor and destructor
            CmdDispatcherFuzzTester();
            virtual ~CmdDispatcherFuzzTester();
            
            // Initialization function that can be called by the fuzzer harness
            void init(NATIVE_INT_TYPE instance = 0);
            
            // 퍼저의 랜덤 값을 받아서 초기화에 활용할 수 있도록 변경
            void initWithFuzzParams(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance = 0);
            
            // 컴포넌트 포트 연결
            void connectPorts();
            
            // 상태 초기화
            void resetState();
            
            // CommandDispatcherImpl에 직접 접근
            CommandDispatcherImpl& getImpl();
            
            // 핑 테스트 메소드
            void sendPing(U32 key);

            // Getter for m_fuzzResult
            const FuzzResult& getFuzzResult() const {
                return m_fuzzResult;
            }

            // Fuzzer를 위한 일반화된 테스트 실행 메소드: 상태 초기화, 명령 주입, 디스패치 수행 후 결과 반환
            FuzzResult tryTest(const uint8_t* data, size_t size);
        
            // Main fuzzing entry point
            FuzzResult getFuzzResult() { return this->m_fuzzResult; }

            // Public wrapper for regCommands if needed, or call directly in tryTest
            void registerCommands();

            // Port handlers - must match the signatures from the base class
            void from_compCmdSend_handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, Fw::CmdArgBuffer &args);
            void from_seqCmdStatus_handler(NATIVE_INT_TYPE portNum, FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdResponse &response);
            void from_pingOut_handler(NATIVE_INT_TYPE portNum, U32 key);

            // Telemetry handlers
            void tlmInput_CommandsDispatched(const Fw::Time& timeTag, const U32 val);
            void tlmInput_CommandErrors(const Fw::Time& timeTag, const U32 val);

            // Event handlers - must match the signatures from the generated TesterBase
            void logIn_DIAGNOSTIC_OpCodeRegistered(U32 Opcode, I32 port, I32 slot);
            void logIn_DIAGNOSTIC_OpCodeReregistered(U32 Opcode, I32 port);
            void logIn_COMMAND_OpCodeDispatched(U32 Opcode, I32 port);
            void logIn_COMMAND_OpCodeCompleted(U32 Opcode);
            void logIn_COMMAND_OpCodeError(U32 Opcode, const Fw::CmdResponse& error); // Ensure const& for CmdResponse
            void logIn_WARNING_HI_MalformedCommand(Fw::DeserialStatus Status);
            void logIn_WARNING_HI_InvalidCommand(U32 Opcode);
            void logIn_WARNING_HI_TooManyCommands(U32 Opcode);
            void logIn_ACTIVITY_HI_NoOpReceived();
            void logIn_ACTIVITY_HI_NoOpStringReceived(const Fw::LogStringArg& message); // Ensure const&
            void logIn_ACTIVITY_HI_TestCmd1Args(I32 arg1, F32 arg2, U8 arg3);

        protected:
            // compCmdSend 핸들러 오버라이드
            void from_compCmdSend_handler(
                NATIVE_INT_TYPE portNum,
                FwOpcodeType opCode,
                U32 cmdSeq,
                Fw::CmdArgBuffer &args
            ) override;

            void from_seqCmdStatus_handler(
                NATIVE_INT_TYPE portNum,
                FwOpcodeType opCode,
                U32 cmdSeq,
                const Fw::CmdResponse &response
            ) override;
            
            // 텔레메트리 핸들러 오버라이드
            void tlmInput_CommandsDispatched(
                const Fw::Time& timeTag,
                const U32 val
            );
            
            void tlmInput_CommandErrors(
                const Fw::Time& timeTag,
                const U32 val
            );
            
            // 이벤트 핸들러 오버라이드
            void logIn_WARNING_HI_MalformedCommand(
                Fw::DeserialStatus Status
            ) override;
            
            void logIn_WARNING_HI_InvalidCommand(
                U32 Opcode
            ) override;
            
            void logIn_WARNING_HI_TooManyCommands(
                U32 Opcode
            ) override;
            
            void logIn_COMMAND_OpCodeError(
                U32 Opcode,
                Fw::CmdResponse error
            ) override;

            void logIn_DIAGNOSTIC_OpCodeReregistered(
                U32 Opcode,
                I32 port
            ) override;
        
        private:
            // 명령 디스패처 구현체를 참조에서 멤버 변수로 변경
            Svc::CommandDispatcherImpl m_impl;
            
            // 퍼징 결과 저장
            FuzzResult m_fuzzResult;

            // 명령 송신 관련 상태
            bool m_cmdSendRcvd = false;
            FwOpcodeType m_cmdSendOpCode = 0;
            U32 m_cmdSendCmdSeq = 0;
            Fw::CmdArgBuffer m_cmdSendArgs;
            // 명령 응답 관련 상태
            bool m_seqStatusRcvd = false;
            FwOpcodeType m_seqStatusOpCode = 0;
            U32 m_seqStatusCmdSeq = 0;
            Fw::CmdResponse m_seqStatusCmdResponse = Fw::CmdResponse::OK;

            // Internal helper methods
            void connectPorts();
            void resetState();
            // CommandDispatcherImpl& getImpl(); // Keep if direct access is still needed
            // void sendPing(U32 key); // Keep if ping testing is part of fuzz scenarios

            // Helper to get the component instance, ensures correct type
            Svc::CommandDispatcherImpl& getImpl_private() { return this->m_impl; }

            // Structure to hold event information
            struct EventInfo {
                std::string name; // Event name (e.g., "OpCodeRegistered", "MalformedCommand")
                FwOpcodeType opcode;
                I32 portOrSlot; // Can represent port, slot, or other integer params
                Fw::CmdResponse response;
                Fw::DeserialStatus desStatus;
                // Add more fields as necessary for different events
                // Example for string arguments in events:
                // Fw::LogStringArg eventStringArg; 

                // Default constructor
                EventInfo() : name(""), opcode(0), portOrSlot(0) {}
                // Constructor for easier logging
                EventInfo(const std::string& n) : name(n), opcode(0), portOrSlot(0) {}
            };
        };

} // namespace Svc

#endif // CMD_DISPATCHER_FUZZ_TESTER_HPP