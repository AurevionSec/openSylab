# API Input Validation Rules - Level 1: Critical Security Fixes

**Status:** In Implementation
**Priority:** P0 - Critical Security
**Date:** 2026-02-04

## Overview

This document defines the input validation rules implemented in `src/api/ApiServer.cpp` to address critical security vulnerabilities identified in the security audit.

## Validation Helper Functions

Located in `src/api/ApiServer.cpp` (lines 220-336):

```cpp
// String length validation (min/max)
bool validateStringLength(const std::string &value, size_t minLen, size_t maxLen, std::string &error);

// Password strength validation
bool validatePassword(const std::string &password, std::string &error);

// Email format validation
bool validateEmail(const std::string &email, std::string &error);

// Username format validation
bool validateUsername(const std::string &username, std::string &error);

// DoS protection - limit capping
bool validateAndCapLimit(int &limit, std::string &error);
```

## Field Validation Rules

### Identifiers (Sample ID, Order ID, Result ID, Patient ID)
- **Min Length:** 1 character
- **Max Length:** 64 characters
- **Validation:** `validateStringLength(value, 1, 64, error)`
- **Rationale:** Prevent buffer overflow, DoS attacks

### Names (Patient Name, Full Name, Measured By, Requested By)
- **Min Length:** 1 character
- **Max Length:** 255 characters
- **Validation:** `validateStringLength(value, 1, 255, error)`
- **Rationale:** Prevent database errors, memory exhaustion

### Descriptions/Notes/Comments
- **Min Length:** 0 characters (optional fields)
- **Max Length:** 5000 characters
- **Validation:** `validateStringLength(value, 0, 5000, error)`
- **Rationale:** Prevent DoS via massive text submissions

### Test Parameters/Types/Units/Values
- **Min Length:** 1 character
- **Max Length:** 255 characters
- **Validation:** `validateStringLength(value, 1, 255, error)`

### Username
- **Min Length:** 3 characters
- **Max Length:** 64 characters
- **Format:** Alphanumeric + underscore only
- **Validation:** `validateUsername(value, error)`
- **Rationale:** Prevent injection, ensure consistent format

### Password
- **Min Length:** 8 characters
- **Max Length:** 128 characters
- **Requirements:** Must contain letters AND numbers
- **Validation:** `validatePassword(value, error)`
- **Rationale:** OWASP password guidelines, prevent weak passwords

### Email
- **Min Length:** 5 characters
- **Max Length:** 255 characters
- **Format:** Must contain @ with text before/after, and . after @
- **Validation:** `validateEmail(value, error)`

### Pagination Limit
- **Min Value:** 1
- **Max Value:** 1000 (hard cap - auto-capped, not rejected)
- **Validation:** `validateAndCapLimit(limit, error)`
- **Rationale:** DoS protection - prevent memory exhaustion

## Endpoint-Specific Rules

### POST /api/v1/auth/login
**Required Fields:**
- `username`: validateStringLength(1, 64)
- `password`: (no validation during login - only during creation)

**Implementation Location:** Line ~686-720

### POST /api/v1/users (Create User)
**Required Fields:**
- `username`: validateUsername()
- `password`: validatePassword()
- `role`: enum validation (already implemented)

**Optional Fields:**
- `full_name`: validateStringLength(0, 255)
- `email`: validateEmail() if provided

**Implementation Location:** Line ~1870-1940

### PUT /api/v1/users/:id (Update User)
**Optional Fields:**
- `username`: validateUsername() if changed
- `password`: validatePassword() if changed
- `full_name`: validateStringLength(0, 255) if provided
- `email`: validateEmail() if provided

### POST /api/v1/samples (Create Sample)
**Required Fields:**
- `sample_id`: validateStringLength(1, 64)
- `patient_id`: validateStringLength(1, 64)

**Optional Fields:**
- `patient_name`: validateStringLength(0, 255)
- `description`: validateStringLength(0, 5000)

**Implementation Location:** Line ~865-920

### PUT /api/v1/samples/:id (Update Sample)
**Same validation as Create Sample**
**Implementation Location:** Line ~1130-1190

### POST /api/v1/orders (Create Order)
**Required Fields:**
- `order_id`: validateStringLength(1, 64)
- `sample_id`: validateStringLength(1, 64)
- `test_type`: validateStringLength(1, 255)

**Optional Fields:**
- `requested_by`: validateStringLength(0, 255)
- `notes`: validateStringLength(0, 5000)

**Implementation Location:** Line ~925-1020

### PUT /api/v1/orders/:id (Update Order)
**Same validation as Create Order**
**Implementation Location:** Line ~1195-1285

### POST /api/v1/results (Create Result)
**Required Fields:**
- `result_id`: validateStringLength(1, 64)
- `order_id`: numeric (already validated)
- `test_parameter`: validateStringLength(1, 255)
- `value`: validateStringLength(1, 255)
- `unit`: validateStringLength(1, 255)

**Optional Fields:**
- `measured_by`: validateStringLength(0, 255)
- `comment`: validateStringLength(0, 5000)
- `reference_range`: validateStringLength(0, 255)

**Implementation Location:** Line ~1025-1125

### PUT /api/v1/results/:id (Update Result)
**Same validation as Create Result**
**Implementation Location:** Line ~1290-1400

### GET Endpoints with Pagination
**Query Parameters:**
- `limit`: validateAndCapLimit() - auto-caps to 1000
- `offset`: Already validated (>= 0)

**Affected Endpoints:**
- GET /api/v1/samples
- GET /api/v1/orders
- GET /api/v1/results
- GET /api/v1/audit

**Implementation Locations:**
- Line ~1475 (samples)
- Line ~1540 (orders)
- Line ~1635 (results)
- Line ~2120 (audit)

## Implementation Strategy

Due to file size (2000+ lines) and repetition across endpoints, validation is applied in phases:

### Phase 1: Critical Endpoints (COMPLETED)
- [x] Validation helper functions created
- [x] Login endpoint - username/password length
- [x] User Create - username format, password strength, email validation
- [x] User Update - same as create when fields provided
- [x] Pagination limits - DoS protection

### Phase 2: Entity CRUD (COMPLETED)
- [x] Sample Create/Update - ID and name length validation
- [x] Order Create/Update - ID and field length validation
- [x] Result Create/Update - ID and field length validation

### Phase 3: Edge Cases (COMPLETED)
- [x] Change password endpoint - password strength validation (already implemented at line 2232-2235)
- [x] Reference range validation (high > low) - POST /api/v1/results (line 1122-1125), PUT /api/v1/results/:id (line 1474-1482)
- [x] Timestamp range validation (from <= to) - GET /api/v1/samples filter (line 1685-1688)

## Error Response Format

When validation fails, return:

```cpp
return makeError(400, "validation_error",
                "Invalid field_name",
                error_message_from_validator);
```

Example:
```json
{
  "error": {
    "code": "validation_error",
    "message": "Invalid password",
    "hint": "Password must be at least 8 characters"
  }
}
```

## Testing

**Unit Tests Required:**
- [ ] Test validateStringLength with edge cases (empty, whitespace, too long)
- [ ] Test validatePassword with weak passwords
- [ ] Test validateEmail with invalid formats
- [ ] Test validateUsername with special characters
- [ ] Test validateAndCapLimit with extreme values (999999)

**Integration Tests Required:**
- [ ] POST /users with weak password (should return 400)
- [ ] POST /users with invalid email (should return 400)
- [ ] POST /samples with oversized description (should return 400)
- [ ] GET /samples?limit=999999 (should cap to 1000, not error)
- [ ] POST /users with username containing spaces (should return 400)

## Backward Compatibility

**Breaking Changes:**
- Existing weak passwords will continue to work (validation only on creation/update)
- Existing data with oversized fields will remain (validation only on new writes)
- Existing usernames with special characters will continue to work (validation only on creation/update)

**Mitigation:**
- Add data migration script to identify non-compliant data
- Log warnings for existing non-compliant data
- Provide admin tool to fix non-compliant data

## Security Impact

### Vulnerabilities Fixed:
1. **DoS via Large Payloads:** Max field lengths prevent memory exhaustion
2. **DoS via Large Limits:** Pagination capped at 1000 records
3. **Weak Passwords:** Enforced 8+ chars with letters + numbers
4. **Whitespace-Only Values:** Trimming and empty-check prevents data pollution
5. **SQL Injection Risk Reduced:** While prepared statements protect, limiting input reduces attack surface

### Remaining Risks (Level 2 & 3):
- Business logic validation (e.g., foreign key existence)
- Status transition rules
- Race conditions
- Advanced DoS (rate limiting needed)

## Performance Impact

**Minimal:**
- String length checks: O(n) where n = string length
- Password strength check: O(n) single pass
- Email validation: O(n) string search operations
- Username validation: O(n) character checking

**Total overhead per request:** < 1ms for typical inputs

## Deployment Checklist

- [ ] Code review completed
- [ ] Unit tests passing
- [ ] Integration tests passing
- [ ] Performance benchmarks acceptable
- [ ] Security audit approval
- [ ] Documentation updated
- [ ] Migration plan for existing data
- [ ] Monitoring alerts configured
- [ ] Rollback plan documented
