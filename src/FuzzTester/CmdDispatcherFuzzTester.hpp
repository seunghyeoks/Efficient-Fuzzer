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

            // 생성자: CommandDispatcherImpl을 내부에서 직접 생성하도록 변경
            CmdDispatcherFuzzTester();
            virtual ~CmdDispatcherFuzzTester();
            
            // 기존 init 함수 선언 추가
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
        };

} // namespace Svc

#endif // CMD_DISPATCHER_FUZZ_TESTER_HPP