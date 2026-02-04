#!/bin/bash
# Integration tests for API input validation
# Tests all validation rules from VALIDATION_RULES.md

# Don't exit on error - we want to count failures
# set -e

API_URL="${API_URL:-http://localhost:8080/api/v1}"
PASSED=0
FAILED=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test result tracking
declare -a FAILED_TESTS=()

echo "========================================"
echo "API Input Validation Integration Tests"
echo "========================================"
echo "API URL: $API_URL"
echo ""

# Helper function to test API endpoint
test_endpoint() {
    local test_name="$1"
    local method="$2"
    local endpoint="$3"
    local data="$4"
    local expected_code="$5"
    local token="$6"

    echo -n "Testing: $test_name... "

    local http_code

    if [ "$method" = "GET" ]; then
        if [ -n "$token" ]; then
            http_code=$(curl -s -o /dev/null -w "%{http_code}" \
                -H "Authorization: Bearer $token" \
                "$API_URL$endpoint")
        else
            http_code=$(curl -s -o /dev/null -w "%{http_code}" \
                "$API_URL$endpoint")
        fi
    else
        if [ -n "$token" ]; then
            http_code=$(curl -s -o /dev/null -w "%{http_code}" \
                -X "$method" \
                -H "Content-Type: application/json" \
                -H "Authorization: Bearer $token" \
                -d "$data" \
                "$API_URL$endpoint")
        else
            http_code=$(curl -s -o /dev/null -w "%{http_code}" \
                -X "$method" \
                -H "Content-Type: application/json" \
                -d "$data" \
                "$API_URL$endpoint")
        fi
    fi

    if [ "$http_code" = "$expected_code" ]; then
        echo -e "${GREEN}PASS${NC} (HTTP $http_code)"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC} (Expected HTTP $expected_code, got $http_code)"
        FAILED_TESTS+=("$test_name")
        ((FAILED++))
    fi
}

# Get admin token for authenticated tests
echo "=== Setup: Getting admin token ==="
LOGIN_RESPONSE=$(curl -s -X POST "$API_URL/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"username":"admin","password":"admin"}')
TOKEN=$(echo "$LOGIN_RESPONSE" | grep -o '"token":"[^"]*' | cut -d'"' -f4)

if [ -z "$TOKEN" ]; then
    echo -e "${RED}ERROR: Failed to get admin token${NC}"
    echo "Response: $LOGIN_RESPONSE"
    exit 1
fi
echo -e "${GREEN}✓ Admin token obtained${NC}"
echo ""

# ============================================
# Phase 1: User Management Validation Tests
# ============================================
echo "=== Phase 1: User Management Validation ==="

# Test 1: Weak password (too short)
test_endpoint \
    "Reject password < 8 chars" \
    "POST" \
    "/users" \
    '{"username":"testuser1","password":"abc123","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 2: Weak password (no numbers)
test_endpoint \
    "Reject password without numbers" \
    "POST" \
    "/users" \
    '{"username":"testuser2","password":"abcdefgh","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 3: Weak password (no letters)
test_endpoint \
    "Reject password without letters" \
    "POST" \
    "/users" \
    '{"username":"testuser3","password":"12345678","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 4: Invalid email format (no @)
test_endpoint \
    "Reject invalid email (no @)" \
    "POST" \
    "/users" \
    '{"username":"testuser4","password":"test1234","email":"notanemail","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 5: Invalid email format (no domain)
test_endpoint \
    "Reject invalid email (no domain)" \
    "POST" \
    "/users" \
    '{"username":"testuser5","password":"test1234","email":"test@","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 6: Invalid username (contains spaces)
test_endpoint \
    "Reject username with spaces" \
    "POST" \
    "/users" \
    '{"username":"test user","password":"test1234","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 7: Invalid username (special characters)
test_endpoint \
    "Reject username with special chars" \
    "POST" \
    "/users" \
    '{"username":"test@user","password":"test1234","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 8: Username too short (< 3 chars)
test_endpoint \
    "Reject username < 3 chars" \
    "POST" \
    "/users" \
    '{"username":"ab","password":"test1234","role":"TECHNICIAN"}' \
    "400" \
    "$TOKEN"

# Test 9: Valid user creation (should succeed)
test_endpoint \
    "Accept valid user data" \
    "POST" \
    "/users" \
    '{"username":"validuser","password":"valid1234","email":"valid@example.com","role":"TECHNICIAN"}' \
    "200" \
    "$TOKEN"

echo ""

# ============================================
# Phase 2: Entity CRUD Validation Tests
# ============================================
echo "=== Phase 2: Entity CRUD Validation ==="

# Test 10: Oversized sample_id (> 64 chars)
LONG_ID=$(printf 'a%.0s' {1..65})
test_endpoint \
    "Reject sample_id > 64 chars" \
    "POST" \
    "/samples" \
    "{\"sample_id\":\"$LONG_ID\",\"patient_id\":\"P001\"}" \
    "400" \
    "$TOKEN"

# Test 11: Oversized description (> 5000 chars)
LONG_DESC=$(printf 'x%.0s' {1..5001})
test_endpoint \
    "Reject description > 5000 chars" \
    "POST" \
    "/samples" \
    "{\"sample_id\":\"S001\",\"patient_id\":\"P001\",\"description\":\"$LONG_DESC\"}" \
    "400" \
    "$TOKEN"

# Test 12: Valid sample creation
test_endpoint \
    "Accept valid sample data" \
    "POST" \
    "/samples" \
    '{"sample_id":"S001","patient_id":"P001","patient_name":"Test Patient"}' \
    "200" \
    "$TOKEN"

# Test 13: Oversized order notes (> 5000 chars)
test_endpoint \
    "Reject order notes > 5000 chars" \
    "POST" \
    "/orders" \
    "{\"order_id\":\"O001\",\"sample_id\":\"S001\",\"test_type\":\"Blood\",\"notes\":\"$LONG_DESC\"}" \
    "400" \
    "$TOKEN"

# Test 14: Valid order creation
test_endpoint \
    "Accept valid order data" \
    "POST" \
    "/orders" \
    '{"order_id":"O001","sample_id":"S001","test_type":"Blood Test"}' \
    "200" \
    "$TOKEN"

# Test 15: Oversized result comment (> 5000 chars)
test_endpoint \
    "Reject result comment > 5000 chars" \
    "POST" \
    "/results" \
    "{\"result_id\":\"R001\",\"order_id\":1,\"test_parameter\":\"Glucose\",\"value\":\"100\",\"unit\":\"mg/dL\",\"comment\":\"$LONG_DESC\"}" \
    "400" \
    "$TOKEN"

echo ""

# ============================================
# Phase 3: Edge Case Validation Tests
# ============================================
echo "=== Phase 3: Edge Case Validation ==="

# Test 16: Invalid reference range (high <= low)
test_endpoint \
    "Reject reference_high <= reference_low" \
    "POST" \
    "/results" \
    '{"result_id":"R002","order_id":1,"test_parameter":"Glucose","value":"100","unit":"mg/dL","reference_low":100,"reference_high":50}' \
    "400" \
    "$TOKEN"

# Test 17: Invalid reference range (high = low)
test_endpoint \
    "Reject reference_high = reference_low" \
    "POST" \
    "/results" \
    '{"result_id":"R003","order_id":1,"test_parameter":"Glucose","value":"100","unit":"mg/dL","reference_low":100,"reference_high":100}' \
    "400" \
    "$TOKEN"

# Test 18: Valid reference range
test_endpoint \
    "Accept valid reference range" \
    "POST" \
    "/results" \
    '{"result_id":"R004","order_id":1,"test_parameter":"Glucose","value":"100","unit":"mg/dL","reference_low":70,"reference_high":120}' \
    "200" \
    "$TOKEN"

# Test 19: Invalid timestamp range (from > to)
FROM_TS=$(($(date +%s) + 86400))  # Tomorrow
TO_TS=$(date +%s)  # Today
test_endpoint \
    "Reject from > to in date filter" \
    "GET" \
    "/samples?from=$FROM_TS&to=$TO_TS" \
    "" \
    "400" \
    "$TOKEN"

# Test 20: Valid timestamp range
TO_TS=$(($(date +%s) + 86400))  # Tomorrow
FROM_TS=$(date +%s)  # Today
test_endpoint \
    "Accept valid date range (from <= to)" \
    "GET" \
    "/samples?from=$FROM_TS&to=$TO_TS" \
    "" \
    "200" \
    "$TOKEN"

# Test 21: Pagination limit capping (should cap to 1000, not error)
test_endpoint \
    "Cap limit to 1000 (not reject)" \
    "GET" \
    "/samples?limit=999999" \
    "" \
    "200" \
    "$TOKEN"

echo ""

# ============================================
# Summary
# ============================================
echo "========================================"
echo "Test Summary"
echo "========================================"
echo -e "Total tests:  $((PASSED + FAILED))"
echo -e "${GREEN}Passed:       $PASSED${NC}"
echo -e "${RED}Failed:       $FAILED${NC}"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed tests:${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}✗${NC} $test"
    done
    echo ""
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests passed! ✓${NC}"
    echo ""
    exit 0
fi
