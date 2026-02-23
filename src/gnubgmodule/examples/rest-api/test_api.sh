#!/usr/bin/env bash
# Quick test of the REST API (server must be running on http://127.0.0.1:5000)
# Usage: ./test_api.sh [base_url]
set -e
BASE="${1:-http://127.0.0.1:5000}"

run_json() {
  local resp
  resp=$(curl -s -w "\n%{http_code}" "$@")
  local code
  code=$(echo "$resp" | tail -n1)
  local body
  body=$(echo "$resp" | sed '$d')
  if [ -z "$body" ]; then
    echo "(empty response, HTTP $code)"
    echo "If the server crashed (e.g. segfault in gnubg), see README.md troubleshooting in this directory."
    return 1
  fi
  echo "$body" | python3 -m json.tool 2>/dev/null || echo "$body"
  return 0
}

echo "=== GET /health ==="
run_json "$BASE/health" || true

echo ""
echo "=== GET /config ==="
run_json "$BASE/config" || true

echo ""
echo "=== POST /evaluate ==="
run_json -X POST "$BASE/evaluate" \
  -H "Content-Type: application/json" \
  -d '{"position_id": "4HPwATDgc/ABMA"}' || true

echo ""
echo "=== POST /best-move ==="
run_json -X POST "$BASE/best-move" \
  -H "Content-Type: application/json" \
  -d '{"position_id": "4HPwATDgc/ABMA", "die1": 6, "die2": 1}' || true

echo ""
echo "=== Done ==="
