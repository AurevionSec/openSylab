## Summary

What does this PR change and why?

Closes #

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Security fix
- [ ] Refactor / tech debt
- [ ] Documentation
- [ ] Release / version bump

## Checklist

- [ ] Build is clean: 0 errors, 0 new warnings
- [ ] All tests pass (`ctest --test-dir build` and `npm test` in `frontend/`)
- [ ] New public functions have tests, including at least one failure path
- [ ] Data-mutating changes write an `AuditEntry` and enforce an RBAC check (ISO 15189)
- [ ] No secrets/passwords logged; SQL only via prepared statements
- [ ] New `.cpp` registered in `CMakeLists.txt`; new test in `test/CMakeLists.txt`
- [ ] 5-layer architecture respected (no layer N → N+1 or `api/` → `db/` direct import)
- [ ] `CHANGELOG.md` updated for user-visible changes
- [ ] Version SSOT unchanged unless this is an intentional release bump

## Security & compliance impact

Describe any impact on authentication, RBAC, audit trail, or patient-data handling.

## How was this tested?

Describe the tests you ran and their results.
