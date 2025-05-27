#!/bin/bash
set -e

echo "=== F-Prime CmdDispatcher Gramatron 시작 ==="

# 빌드 디렉토리 설정
BUILD_DIR="/workspace/Efficient-Fuzzer/src/gramatron/build"

echo "=== 기존 빌드 디렉토리 삭제 및 생성 ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# === Seed corpus 자동 생성 ===
echo "=== Seed corpus 자동 생성 (corpus/seed_*.bin) ==="
mkdir -p "$BUILD_DIR/corpus"
cp /workspace/Efficient-Fuzzer/src/FuzzTester/seed/seed*.bin "$BUILD_DIR/corpus/" || true

echo "=== CMake 구성 시작 ==="
cmake .. 

echo "=== Make 빌드 시작 (gramatron_main) ==="
make gramatron_main

echo "=== 결과 디렉토리 생성 $BUILD_DIR/findings ==="
mkdir -p findings

echo "=== Gramatron fuzzing 시작 ==="
# Gramatron 실행 예시 (grammar/automaton 파일 경로는 실제 환경에 맞게 수정 필요)
GRAMMAR_AUTOMATON="/workspace/Gramatron/grammars/example_automaton.json"
GRAMATRON_BIN="/workspace/Gramatron/src/gramfuzz-mutator/run_campaign.sh"
$GRAMATRON_BIN $GRAMMAR_AUTOMATON $BUILD_DIR/findings "$BUILD_DIR/gramatron_main @@"