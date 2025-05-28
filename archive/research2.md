F′의 단위 테스트 구성과 CmdDispatcher 테스트 코드 분석

F′에서는 fprime-util 툴로 빌드 환경을 구성하고 테스트를 자동 관리합니다. 예를 들어 fprime-util generate --ut 명령은 단위 테스트용 빌드 캐시를 생성하고, 이후 fprime-util build --ut로 빌드하여 테스트 실행 파일을 만듭니다. 그 다음 fprime-util check 명령을 실행하면(또는 --all, --coverage 같은 옵션과 함께) 해당 디렉터리의 모든 단위 테스트를 빌드하고 구글테스트(GTest)를 통해 실행합니다 ￼ ￼. 이 과정에서 F′의 CMake 시스템은 register_fprime_ut() 매크로를 통해 테스트 실행 파일을 자동 생성합니다. 예를 들어 CmdDispatcher 모듈의 CMakeLists.txt에는 다음과 같은 설정이 있습니다:

set(UT_SOURCE_FILES
  ${FPRIME_FRAMEWORK_PATH}/Svc/CmdDispatcher/CommandDispatcherComponentAi.xml
  test/ut/CommandDispatcherTester.cpp
  test/ut/CommandDispatcherImplTester.cpp
)
register_fprime_ut()

이렇게 하면 CommandDispatcherTester.cpp와 CommandDispatcherImplTester.cpp 같은 테스트 소스들이 테스트 실행 파일에 포함되어 빌드됩니다 ￼. 즉, fprime-util generate/check는 CMake 테스트 대상을 자동으로 구성하여 GoogleTest 기반의 단위 테스트를 실행할 수 있도록 합니다.

생성된 테스트 코드의 구조와 작동 방식

fprime-util impl --ut 명령을 사용하면 Tester.cpp/Tester.hpp, GTestBase.cpp/\.hpp, TesterBase.cpp/\.hpp, TestMain.cpp 등이 생성됩니다. 예를 들어 MathComponent 튜토리얼에서 보여주듯이, 각 테스트는 다음과 같은 Google Test 매크로를 사용합니다:

TEST(Nominal, AddOperationTest) {
    Ref::Tester tester;
    tester.testAddCommand();
}

Tester 클래스 인스턴스를 생성하고, 그 안에 정의된 테스트 메서드(testAddCommand() 등)를 호출하는 방식입니다 ￼. 여기서 Tester 클래스는 TesterBase(자동 생성)로부터 상속받거나 연결되어 있으며, 테스트 전후의 초기화와 포트 연결을 담당합니다. GTestBase.cpp에는 main() 함수로서 ::testing::InitGoogleTest()와 RUN_ALL_TESTS() 호출이 포함되어 있어, 모든 TEST() 매크로를 실행시킵니다 ￼.

CommandDispatcherTester.cpp와 CommandDispatcherImplTester.cpp는 각각 CmdDispatcher 컴포넌트와 그 구현(Impl) 부분에 대한 테스트 코드를 담고 있습니다. 두 파일 모두 fprime-util로 생성된 골격(stub) 코드이며, 개발자가 실제 테스트 로직(예: tester.testXxx() 메서드)을 추가해야 합니다. 예를 들어, Math 예시에서는 Tester.cpp에서 testAddCommand() 같은 테스트 메서드를 구현합니다 ￼ ￼. 이때, 테스트 코드는 싱글스레드로 동작하며(별도의 스레드 시작 없이), 비동기 포트에 대한 메시지는 테스트 핸들러에서 수동으로 큐를 처리하도록 구성됩니다 ￼.

테스트 드라이버 클래스와 포트 연결

F′의 테스트 하네스는 TesterBase와 GTestBase 같은 자동 생성 클래스를 사용하여 구성됩니다. TesterBase는 테스트할 컴포넌트를 실제로 인스턴스화하고, 컴포넌트의 포트를 테스트 환경에 연결해 줍니다. 모든 입/출력 포트에 대해 스텁 구현이 제공되어 있어, 테스트 코드에서 포트로 데이터를 보내거나 받을 수 있습니다. 예를 들어, 각 출력 포트(component에서 Test로 가는 방향)에 대해서는 from_<포트명>_handler(...) 메서드가 자동 생성되며, 이 핸들러는 pushFromPortEntry_<포트명>(…)을 호출하여 데이터 큐에 값을 밀어넣습니다 ￼. 테스트 코드는 ASSERT_FROM_<포트명>(기댓값) 매크로 등을 사용해 이 큐에서 값을 꺼내 검증할 수 있습니다. 반면 입력 포트(Test에서 component로 가는 방향)로 데이터를 보내려면, TesterBase가 제공하는 invoke_to_<포트명>(…)이나 대응되는 pushToPortEntry_<포트명>(…) 함수를 사용할 수 있습니다. 요약하면, 테스트 드라이버(Tester/TesterBase)는 컴포넌트를 초기화(initComponents())하고 포트를 연결한 뒤, to_ 함수를 통해 입력을 주고 from_ 핸들러와 pushFromPortEntry로 출력 결과를 수집하여 검증합니다 ￼ ￼. 테스트 코드(Tester.cpp) 내의 toDo() 같은 함수에서 이러한 입력/출력 단계를 조립하고, ASSERT_FROM_ 매크로로 기대 동작을 확인합니다.

퍼징 하네스를 위한 코드 활용 방법

CmdDispatcher를 퍼징하려면 위의 테스트 하네스 로직을 참고하여 독립적인 입력 루프를 구현해야 합니다. 예를 들어 퍼저의 LLVMFuzzerTestOneInput 함수 내에서 Svc::CmdDispatcher 객체를 생성하고 초기화(init() 호출)합니다. 그 뒤 테스트 하네스처럼 포트 연결은 최소화하고, 오히려 명령 버퍼 데이터(예: Fw::Com 버퍼)를 퍼저 입력으로부터 생성하여 직접 from_cmdBuff_handler(0, fuzzedCom)를 호출할 수 있습니다. CmdDispatcher는 명령 등록이 필요하므로, 경우에 따라 적절한 등록 단계(from_compCmdReg_handler)를 먼저 수행해야 합니다. 내부적으로 CmdDispatcher는 받은 버퍼에서 opcode를 추출하여 등록된 컴포넌트로 명령을 발행하고, 결과 상태를 다시 보고하는 로직을 가집니다 ￼. 퍼저 하네스에서는 이 과정을 따라, 입력 데이터(버퍼)가 어떤 식으로 처리되는지, 메모리 접근이 안전한지 등을 검사할 수 있습니다. 예를 들어:

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    Svc::CmdDispatcher dispatcher("CmdDisp");
    dispatcher.init(0, 0); // 초기화
    // 필요시 가짜 컴포넌트로 등록(테스트 하네스 로직 참조)
    // ...
    Fw::ComBuffer comBuf;
    comBuf.setsize(size);
    memcpy(comBuf.getBuffAddr(), data, size); // 퍼저 입력을 버퍼로 사용
    dispatcher.from_CmdBuff_handler(0, comBuf); // 명령 입력 시뮬레이션
    // 추가로 dispatcher 내부 상태나 출력 체크
    return 0;
}

이처럼, 테스트 코드에서 사용하는 포트 접근 방식, 컴포넌트 초기화, 메모리 버퍼 처리 등을 재활용할 수 있습니다. 특히, CmdDispatcher는 명령 테이블과 상태 반환 테이블을 내부에 유지하므로, 퍼저 루프마다 객체를 재생성하거나 관련 데이터를 리셋해야 합니다. 테스터 클래스의 initComponents()나 reset() 기능이 있다면 이를 호출해 초기 상태를 보장할 수 있고, F′ 프레임워크의 Fw::Buffer나 Fw::ComBuffer 사용법을 그대로 응용하면 됩니다. 요약하면, 기존 GTest 기반 테스트 하네스에서 CmdDispatcher 초기화·포트 발송·상태 수집 로직을 발췌하여, 이를 퍼저 환경의 드라이버 함수로 변환하면 됩니다.

F′에서의 퍼저 연동 일반 사례

현재 F′ 문서나 예제에는 특별히 퍼징용 핸즈(harness)를 직접 다루는 예시는 많지 않습니다. 그러나 단위 테스트 코드를 GoogleTest 없이 독립 실행하는 방법은 존재합니다. 예를 들어 register_fprime_ut 매크로를 사용할 때 Ai.xml 파일 대신 직접 의존성을 지정하면, GTest 헬퍼 클래스를 사용하지 않는 테스트도 구성할 수 있습니다 ￼. 이를 응용하면, 퍼저 타겟용 CMake 빌드를 작성하여 GTest 대신 LLVMFuzzer 라이브러리를 링크하고 LLVMFuzzerTestOneInput 함수를 포함시킬 수 있습니다. 요컨대, F′의 단위 테스트 구성(컴포넌트 빌드, 포트, 메모리 초기화 등)에서 필요한 부분만 골라 새로운 executable 타깃으로 빌드하여 퍼저와 연결하면 됩니다.

다른 사례로는 개발자들이 컴포넌트 구현 코드를 독립적인 라이브러리나 스레드 비활성화 상태로 빌드하여 테스트하는 방법이 있으며, 이를 퍼징에 활용할 수 있습니다. 일반적으로 퍼저 연동 시에는 fprime-util generate로 생성된 빌드 캐시를 기반으로, cmake 옵션(-DCMAKE_BUILD_TYPE=Debug 등)과 타깃을 추가하여 fuzzer를 링크하면 됩니다. 현재 공개된 문서에는 구체적 예시가 부족하므로, 위에서 언급한 GTest 제거 및 단위 테스트 코드를 재활용하는 방식으로 자체 구현하는 것이 현실적입니다.

참고: F′ 테스트 하네스는 기본적으로 포트를 스텁으로 제공하므로, 이를 활용해 원하는 동작을 모의(mock)할 수 있습니다 ￼ ￼. 예를 들어 출력 포트의 동작을 테스트 목적에 맞게 수정하려면 Tester.cpp의 from_<Port>_handler 부분에 코드를 추가할 수 있습니다. 이처럼 fprime-util이 제공하는 자동 생성 코드와 구조를 이해하면, GTest에 얽매이지 않고도 원하는 테스트/퍼징 환경을 구성할 수 있습니다.