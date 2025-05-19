#!/usr/bin/env bash
set -e
cd /workspace/MyProject
source fprime-venv/bin/activate
exec "$@" 