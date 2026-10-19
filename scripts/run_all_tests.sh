#!/usr/bin/env bash
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== Running C Test Suite ==="
make test

echo ""
echo "=== Starting Live Server on Port 8080 for Python Suite ==="
./bin/server -c config/server.conf.example -p 8080 >/dev/null 2>&1 &
SERVER_PID=$!
trap "kill -TERM $SERVER_PID 2>/dev/null || true; wait $SERVER_PID 2>/dev/null || true" EXIT

sleep 0.6

echo "=== Running Full Python Test Suite (Unit + Integration) ==="
python3 -m unittest discover -s tests -p "test_*.py" -v
echo "=== All Tests Completed Successfully ==="
