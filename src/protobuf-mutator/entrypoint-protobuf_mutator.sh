#!/bin/bash
# 컴파일러를 clang 계열로 설정 (g++ 대신 clang 사용)
export CC=clang
export CXX=clang++
set -e

echo "=== F-Prime CmdDispatcher LibFuzzer 시작 ==="

# CMake 기반 빌드 디렉토리 설정
BUILD_DIR="/workspace/Efficient-Fuzzer/src/protobuf-mutator/build"

echo "=== 기존 빌드 디렉토리 삭제 및 생성 ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" # 작업 디렉토리를 빌드 디렉토리로 변경


# === Seed corpus 자동 생성 ===
echo "=== Seed corpus 자동 생성 (corpus/seed_*.bin) ==="
mkdir -p "$BUILD_DIR/corpus"
# NoOp 명령 (opcode 0, 인자 0, context 1)
# python3 -c "with open('$BUILD_DIR/corpus/seed_noop.bin','wb') as f: f.write(bytes([0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00]))"
# 사용자 명령 (opcode 0x50, 인자 100, context 2)
# python3 -c "with open('$BUILD_DIR/corpus/seed_user.bin','wb') as f: f.write(bytes([0x01,0x00,0x50,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x02,0x00,0x00,0x00]))"

# === Seed corpus 복사 ===
# echo "=== FuzzTester/seed/*.bin 파일을 corpus/로 복사 ==="
# cp /workspace/Efficient-Fuzzer/src/FuzzTester/seed/seed*.bin "$BUILD_DIR/corpus/" || true


echo "=== CMake 구성 시작 ==="
# CMake 실행 시 상세 로그 출력 옵션 추가 (필요시)
# cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ..
cmake ..

echo "=== Make 빌드 시작 (proto_fuzzer) ==="
# CMakeLists.txt에서 정의한 실행 파일 이름은 'proto_fuzzer' 입니다.
# make VERBOSE=1 proto_fuzzer # 임시 주석 처리. 상세 로그 필요시 이 라인 사용
make proto_fuzzer

echo "결과 디렉토리 생성 ($BUILD_DIR/findings) ==="
# 현재 디렉토리($BUILD_DIR) 내에 findings 디렉토리 생성
mkdir -p findings

echo "=== libFuzzer 실행 시작 (./proto_fuzzer) ==="
# ASAN 설정
export ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:handle_abort=1:abort_on_error=0"

# 현재 디렉토리($BUILD_DIR)에 빌드된 'proto_fuzzer' 실행 파일을 실행
# corpus 및 findings 경로는 현재 디렉토리를 기준으로 전달
./proto_fuzzer -max_len=1024 \
             -artifact_prefix=findings/ \
             -print_final_stats=1 \
             -fork=1 \
             -ignore_crashes=1 \
             -timeout=1 \
             -runs=-1 \
             corpus

echo "=== 스크립트 완료 ==="