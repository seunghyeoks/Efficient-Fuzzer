F′ CmdDispatcher 테스트 하네스 구조 분석

fprime-util impl --ut 명령은 각 컴포넌트(예: Svc::CmdDispatcher)의 단위 테스트용 스텁 코드를 자동 생성한다. 예를 들어, 수학 컴포넌트 예제에 따르면 이 명령은 Tester.hpp/Tester.cpp, GTestBase.hpp/GTestBase.cpp, TesterBase.hpp/TesterBase.cpp, TestMain.cpp 등의 파일을 생성한다 ￼ ￼. 각 파일의 역할은 다음과 같다 ￼:
	•	TesterBase.*: 자동 생성된 기본 테스트 클래스. 컴포넌트와 동일한 인터페이스를 제공하여 입력/출력 포트를 시뮬레이션하고, H(히스토리) 객체로 전송 데이터를 저장한다. 테스트 작성에 필요한 유틸리티 함수(명령 전송, 포트 호출 등)를 제공한다.
	•	GTestBase.*: TesterBase를 상속하는 클래스. Google Test 헤더와 F′ 특정 매크로를 포함하여 ASSERT_EQ 등의 단언(Assertion) 기능을 추가로 제공한다. (GTest 지원이 필요 없는 플랫폼용으로 별도 분리됨)
	•	Tester.*: GTestBase(또는 TesterBase)를 상속하는 개발자 작성 클래스. 실제 컴포넌트 인스턴스를 멤버로 갖고, 자동 생성된 포트 연결 코드와 테스트용 헬퍼를 포함한다.
	•	TestMain.cpp: Google Test 프레임워크를 이용한 TEST() 매크로 기반 단위 테스트의 진입점(main). 예제에서는 TEST(Nominal, AddOperationTest){ Ref::Tester tester; tester.testAddCommand(); } 식으로 테스트 메서드를 호출한다 ￼.

따라서 CmdDispatcher에 대해 fprime-util impl --ut를 실행하면 위 구조와 유사한 파일들이 Svc/CmdDispatcher/test/ut/ 등의 디렉토리에 생성된다. 이 하네스는 기본적으로 GTest를 이용해 정의된 테스트 함수를 호출하는 형태이므로, 퍼저와 연동하기 위해서는 이 구조를 수정하여 libFuzzer, AFL++, boofuzz 가공 입력을 받을 수 있는 함수 형태로 변환해야 한다.

libFuzzer 연동 구조 설계

LibFuzzer는 “인프로세스” 퍼저로, 라이브러리(또는 프로그램) 내부로 연결되어 입력 바이트 배열을 반복적으로 투입하며 코드 커버리지를 최적화한다 ￼ ￼. 이를 위해 extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) 형태의 퍼즈 타겟 함수를 구현해야 한다 ￼. 설계 방향은 다음과 같다:
	•	입력 처리 함수: 퍼저에서 전달받은 Data, Size를 적절한 형태(예: std::vector<uint8_t> 또는 Fw::ComBuffer)로 변환하고, CmdDispatcher의 핸들러에 전달한다. 예를 들어, CmdDispatcher는 Fw::Com(버퍼) 형태의 명령을 수신하므로, uint8_t 배열을 Fw::ComBuffer 구조체로 래핑하여 cmdBuffIn_handler( portNum, buffer ) 등을 호출할 수 있다. 또는 자동 생성된 Tester 클래스의 메서드(예: invoke_cmdBuffIn(...))를 통해 전달할 수 있다.
	•	가공 함수 예제:

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // 테스트 클래스 인스턴스 생성 (포트 연결 등 초기화 포함)
    Svc::CmdDispatcherTester tester;
    // 퍼저 입력을 Fw::ComBuffer 형식으로 변환
    Fw::ComBuffer cmdBuf;
    cmdBuf.setBuff(Data, Size);
    // 예: 포트 0번으로 가정하고 핸들러 호출
    tester.invoke_cmdBuffIn(0, cmdBuf);
    // 필요 시 상태 업데이트 등을 반복
    return 0;
}

위와 같은 구조로 LLVMFuzzerTestOneInput 함수 내에서 CmdDispatcher 컴포넌트에 입력을 주입한다. LibFuzzer는 이 함수를 반복 호출하며 다양한 입력을 시도한다 ￼.

	•	컴파일/링크 옵션: LibFuzzer 사용을 위해서는 Clang으로 컴파일하고, 컴파일 시 -fsanitize=fuzzer,address와 같은 옵션을 지정해야 한다 ￼. 이 옵션은 주소/메모리 오류 감지(AddressSanitizer) 및 커버리지 트레이스(coverage) 기능을 포함한다. 예:

clang++ -g -O1 -fsanitize=address,fuzzer -std=c++11 ... fuzz_target.cpp -o fuzzer

이렇게 빌드된 바이너리는 fuzzer로 실행하면 자동으로 퍼징을 수행한다.

수정 포인트 및 테스트 하네스 개조

기존 자동 생성 GTest 기반 하네스는 다음과 같은 변경이 필요하다:
	•	main 루틴 분리/제거: 기존 TestMain.cpp에는 TEST() 매크로를 등록하는 main()이 포함된다. LibFuzzer와 연동하려면 GTest 의존 메인 코드를 제거하고, 대신 퍼저 엔트리 함수(LLVMFuzzerTestOneInput)를 구현하거나, AFL용 main()을 작성해야 한다.
	•	Google Test 의존성 제거: Tester 클래스가 GTestBase를 상속하도록 자동 생성되는데, 더 이상 ASSERT_* 등을 쓸 필요가 없으면 이 상속을 TesterBase로 바꾸고 <gtest/gtest.h> 포함도 제거한다. (또는 GTestBase 사용을 유지하되, 테스트 내 단언문 대신 fuzz 타겟 로직만 수행하도록 수정)
	•	입력을 인자로 받는 함수화: 퍼저 엔진이 파일이나 바이트 배열을 공급할 수 있도록, CmdDispatcher에 대한 호출을 함수로 분리한다. 예를 들어 위의 LLVMFuzzerTestOneInput처럼, 바이트 배열 인자를 받아 처리하는 함수를 만든다. 또한 AFL++를 고려하여 파일 입출력을 처리하는 main()을 별도로 구현할 수도 있다.

예를 들어, AFL++와 libFuzzer를 모두 지원하도록 아래와 같이 설계할 수 있다:

// fuzz_target.cpp (예시)
#include "Tester.hpp"  // 수정된 Tester 클래스 포함

static void testWithData(const uint8_t *Data, size_t Size) {
    Svc::CmdDispatcherTester tester;
    // 퍼저 데이터를 CmdDispatcher에 투입
    Fw::ComBuffer cmdBuf;
    cmdBuf.setBuff(Data, Size);
    tester.invoke_cmdBuffIn(0, cmdBuf);
    // 필요 시 추가 로직
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    testWithData(Data, Size);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return 0;
    }
    // AFL++ 방식: 파일에서 입력 읽기
    std::ifstream ifs(argv[1], std::ios::binary);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(ifs)), {});
    testWithData(buf.data(), buf.size());
    return 0;
}

	•	이 예시에서는 testWithData()로 핵심 처리 코드를 통합했으며, LibFuzzer용 함수와 AFL++용 main()을 함께 제공했다. 빌드 시에는 LibFuzzer용은 LLVMFuzzerTestOneInput만 필요하므로 main() 정의를 제외하고 빌드하거나, AFL++용 바이너리를 별도로 컴파일할 수 있다.

AFL++ 및 BooFuzz 확장 설계

AFL++ 및 BooFuzz를 고려한 설계 방향은 다음과 같다:
	•	AFL++ 통합: AFL++는 보통 별도의 메인 함수에서 파일 입력을 처리하며, 컴파일 시에도 -fsanitize-coverage=trace-pc-guard -fsanitize=address 옵션을 사용한다 ￼. 위 예시의 main() 함수(파일 입력 처리)를 AFL++ 타겟으로 사용하면, AFL++ 실행 시 afl-fuzz -i in_dir -o out_dir -- ./fuzzer @@ 형태로 입력 파일을 testWithData()에 전달할 수 있다. (또는 clusterfuzz 가이드처럼 AFL의 FuzzingEngine.a를 연결해 LLVMFuzzerTestOneInput 함수만으로 AFL++ 구동도 가능하다 ￼.)
	•	Boofuzz 연동: Boofuzz는 Python 기반의 네트워크 프로토콜 퍼징 도구로, 주로 타깃 애플리케이션을 소켓 등으로 실행시켜 입력을 보낸다 ￼. CmdDispatcher 같은 컴포넌트를 Boofuzz와 연동하려면, 테스트 하네스를 네트워크 서비스 형태로 제공하거나 C/C++ 라이브러리로 래핑해야 한다. 예를 들어, C++ 코드에 간단한 TCP 서버를 구현하여 수신된 바이트를 testWithData()로 전달하거나, Boofuzz의 C 라이브러리 로딩 기능을 이용하여 함수 포인터를 직접 호출할 수 있다. 전반적인 설계는 다음과 같다:
	•	CmdDispatcher 테스트 코드를 라이브러리(libcmddisp.so)로 컴파일하고, 이 라이브러리에 testWithData()와 같은 C 인터페이스를 노출한다.
	•	Python Boofuzz 스크립트에서 타깃을 프로세스 모드(Attach, 예: target = Target(connection=Process('./runner')))로 설정하거나 네트워크 모드로 구현한다. 네트워크 모드라면 C++ 쪽에 간단한 서버를 띄워놓고, Boofuzz가 TCP/UDP로 데이터를 보낸다.
	•	예를 들어, C++에서 CmdDispatcher를 메모리에 올리고 소켓을 열어, 받은 데이터(uint8_t 스트림)를 testWithData에 넘기고 결과(로그/상태)를 반환하는 구조를 짤 수 있다.
	•	Boofuzz 예제:

from boofuzz import *
session = Session(target=Target(connection=SocketConnection("127.0.0.1", 12345, proto='tcp')))
s_initialize("cmdbuf")
s_block_start("cmd")
s_bytes("\xAA\xBB", name="Header")  # 예시 헤더
s_random("CMD1", min_length=1, max_length=4)  # 가변 길이 커맨드
s_block_end()
session.connect(s_get("cmdbuf"))
session.fuzz()

이 스크립트는 TCP로 포트 12345에 데이터를 전송하는데, C++ 쪽에서는 CommandDispatcherTester를 생성해 이 데이터를 CmdDispatcher로 투입한다. BooFuzz 사용 시에는 타깃 실행마다 상태를 초기화하거나, 실패 시 재시작 기능 등을 활용해 연속 테스트할 수 있다.

코드 예시 및 설정
	•	CMake 설정: F′에서 UT를 등록하듯, 새로 만든 fuzz_target.cpp도 CMake UT_SOURCE_FILES에 추가하고 register_fprime_ut()를 호출한다. 그러나 퍼징빌드를 위해서는 별도 빌드 타겟을 정의할 수도 있다. 예를 들어:

set(UT_SOURCE_FILES
    test/ut/Tester.cpp
    test/ut/TestMain.cpp  # (필요 시 제외)
    # 새로 작성한 fuzz_target.cpp 등
)
register_fprime_ut()


	•	빌드/실행 예시:
	•	libFuzzer:

clang++ -g -O1 -fsanitize=address,fuzzer -I<path_to_fprime_include> fuzz_target.cpp -o fuzzer_cmd
./fuzzer_cmd


	•	AFL++:

# AFL++ 엔진 빌드 후 
clang++ -g -O1 -fsanitize=address -fsanitize-coverage=trace-pc-guard fuzz_target.cpp FuzzingEngine.a -o afl_cmd
AFL_SKIP_CPUFREQ=1 afl-fuzz -i input_corpus -o afl_output -- ./afl_cmd @@


	•	Boofuzz:

# C++ 코드: server.cpp
clang++ server.cpp -o server
./server   # (백그라운드로 실행하여 포트 리슨)
# Python 코드: fuzz.py (위 예시 내용)
python3 fuzz.py



위와 같이 기존 GTest 기반 하네스를 수정하여, 단위 테스트 대신 퍼징 엔진으로 입력을 주입하는 구조로 전환하면 libFuzzer, AFL++, Boofuzz 등과의 통합이 가능하다. 예시 코드와 설정 파일을 참고하여, 주요 수정 지점(메인 분리, GTest 제거, 입력 함수화 등)을 구현하면 효율적인 퍼징 환경을 구축할 수 있다.

참고문헌: fprime 개발자 가이드 ￼ ￼, libFuzzer 공식 문서 ￼ ￼, ClusterFuzz AFL 가이드 ￼, Boofuzz 개요 ￼.