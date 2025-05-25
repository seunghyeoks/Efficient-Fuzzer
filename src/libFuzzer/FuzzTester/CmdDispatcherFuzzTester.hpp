// CommandDispatcherTesterBase를 상속하여, Fuzz Testing 입력을 CmdDispatcher에 전달하는 harness 클래스

#ifndef CMD_DISPATCHER_FUZZ_TESTER_HPP
#define CMD_DISPATCHER_FUZZ_TESTER_HPP

#include "F-Prime/Svc/CmdDispatcher/CommandDispatcherTesterBase.hpp"
#include <Svc/CmdDispatcher/CommandDispatcherImpl.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/Com/ComPacket.hpp>
#include <vector>

namespace Svc {

    class CmdDispatcherFuzzTester : public CommandDispatcherTesterBase {
        public:
            // 퍼징 결과 추적 구조체
            struct FuzzResult {
                bool hasError;
                Fw::CmdResponse lastResponse;
                U32 lastOpcode;
                std::vector<Fw::CmdResponse> allResponses;
                U32 dispatchCount;
                U32 errorCount;
                bool opCodeReregistered;
                
                FuzzResult() : hasError(false), lastResponse(Fw::CmdResponse::OK), 
                            lastOpcode(0), dispatchCount(0), errorCount(0), opCodeReregistered(false) {}
            };

            // 생성자
            CmdDispatcherFuzzTester(Svc::CommandDispatcherImpl& inst);
            virtual ~CmdDispatcherFuzzTester();
            
            void init(NATIVE_INT_TYPE instance = 0);
            
            // 컴포넌트 포트 연결
            void connectPorts();
            
   
            // 상태 초기화
            void resetState();
            
            // 명령어 처리기 등록
            void registerCommands(U32 num, FwOpcodeType startOpCode = 0x100);
            
            // CommandDispatcherImpl에 직접 접근
            CommandDispatcherImpl& getImpl();
            
            // 핑 테스트 메소드
            void sendPing(U32 key);

            // Public wrapper for invoke_to_compCmdReg
            void public_invoke_to_compCmdReg(FwIndexType portNum, FwOpcodeType opCode) {
                this->invoke_to_compCmdReg(portNum, opCode);
            }

            // Public wrapper for invoke_to_seqCmdBuff
            void public_invoke_to_seqCmdBuff(FwIndexType portNum, Fw::ComBuffer &data, U32 context) {
                this->invoke_to_seqCmdBuff(portNum, data, context);
            }

            // Getter for m_fuzzResult
            const FuzzResult& getFuzzResult() const {
                return m_fuzzResult;
            }

            // Public wrapper for doDispatch loop
            void public_doDispatchLoop();

            // 퍼저 입력으로부터 명령어 버퍼 생성 함수 (추가 필요)
            Fw::ComBuffer createFuzzedCommandBuffer(
                const uint8_t* data, 
                size_t size
            );
        
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
            // 명령 디스패처 구현체
            Svc::CommandDispatcherImpl& m_impl;
            
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
        };

} // namespace Svc

#endif // CMD_DISPATCHER_FUZZ_TESTER_HPP