# TODO - Findings Backlog

This file captures findings from the 5-part analysis (features, security/failure, tests, performance, compliance).

## 1) Feature Behavior Gaps / Alignment
- [x] [P1] Clarify support scope: should support users access list views or only search-by-ID? (Affects Sample/Order/Result lists)
- [x] [P3] Decide if auto-refresh should be opt-in or default-on for CLI views (behavior vs expected UX)
- [x] [P1] Confirm whether auto-refresh cycles should count as access events for audit purposes

## 2) Security & Failure Modes
- [x] [P1] Review API key lifecycle: storage, rotation, revocation, and visibility (risk if keys leak)
- [x] [P1] Normalize logging failure handling: decide where audit failures should block the operation vs warn-only
- [x] [P2] Validate status inputs strictly everywhere (avoid partial normalization gaps)
- [x] [P2] Add size limits / streaming for import payloads (CSV/HL7/FHIR) to reduce memory risk
- [x] [P2] Consider masking file paths in audit log details if paths may reveal sensitive data

## 3) Test Coverage Gaps & Proposed Tests
- [x] [P2] Add tests for support access in list views (ensure limited fields + access logging)
- [x] [P2] Add tests for audit logging failure behavior (should fail vs warn) on critical operations
- [x] [P3] Add tests for stats export ordering consistency (entity + status order)
- [x] [P2] Add tests for API error cases: missing fields, invalid status, inactive API key, invalid payloads
- [x] [P3] Add import boundary tests: large files, duplicate IDs, malformed headers, BOM, extra columns
- [x] [P3] Add refresh behavior tests to ensure no edits are interrupted (where feasible)
- [x] [P3] Add retention + audit export tests (verify exported set after purge)

## 4) Performance & Scalability Risks
- [x] [P2] Review indexing strategy for common filters (samples: status/date, orders: status/date/priority, results: status/date)
- [x] [P2] Consider pagination for API reads and list views to avoid full scans
- [x] [P2] Add streaming or chunked export paths for large datasets
- [x] [P2] Add chunked/batched inserts for CSV/HL7/FHIR imports
- [x] [P3] Evaluate auto-refresh impact under multiple concurrent users (potential DB load)

## 5) Compliance & Audit Completeness
- [x] [P2] Standardize audit action semantics (avoid generic UPDATE where a specific action exists)
- [x] [P1] Ensure access logging policy: define whether refresh loops must log or not
- [x] [P1] Document data minimization rules for support role (fields allowed)
- [x] [P3] Assess need for tamper-evident audit logs (hash chaining / append-only) if compliance scope expands
