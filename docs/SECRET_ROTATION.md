# Secret Rotation Runbook

OpenSylab depends on two cryptographic secrets, both supplied via environment
variables and both **mandatory at startup** (the server hard-fails if either is
missing or shorter than 32 characters):

| Secret | Purpose | Rotatable? |
|--------|---------|------------|
| `OPENSYLAB_JWT_SECRET` | Signs/verifies JWT session tokens | ✅ Yes — see below |
| `OPENSYLAB_AUDIT_HMAC_KEY` | Keys the tamper-evident audit-trail hash chain | ⚠️ **No** on a populated database — see caveat |

This runbook documents the supported rotation procedure for the JWT secret and
the constraints around the audit HMAC key.

---

## 1. Rotating the JWT secret (`OPENSYLAB_JWT_SECRET`)

### When to rotate

- **Immediately** on suspected compromise (leaked secret, compromised host,
  departing operator with production access).
- **Periodically** as policy (e.g. quarterly) for defence in depth.

### Effect of rotation

The JWT secret is read once, when the API router is constructed at server start
(`src/api/ApiServer.cpp`, `ApiRouter` constructor). Tokens are signed with
HMAC using this secret and expire after **60 minutes**.

Rotating the secret means every previously issued token fails signature
validation and is rejected. **Consequence: all logged-in users are signed out
and must re-authenticate.** There is no partial invalidation — it is all-or-nothing.

### Procedure

1. **Generate a new secret** (≥32 characters, high entropy):

   ```bash
   openssl rand -hex 32
   ```

2. **Install the new value** in whichever mechanism your deployment uses:

   - **Bare process / systemd:** update the environment file (e.g.
     `EnvironmentFile=` target) with the new `OPENSYLAB_JWT_SECRET`.
   - **Docker Compose:** update `OPENSYLAB_JWT_SECRET` in `docker-compose.yml`
     (or, preferably, the `.env` file / secret store it references).
   - **Secret manager (Vault, AWS/GCP Secrets Manager, etc.):** update the stored
     value and ensure the container/process reads the new version on next start.

   The secret must be **≥32 characters** and present, otherwise the server
   **hard-fails at startup** (`ApiRouter` constructor). Additionally, a secret
   containing the substrings `dev` or `change` is treated as a development
   placeholder and logs a `[SECURITY WARNING]` — avoid those in production values.

3. **Restart the server** so the new secret is loaded:

   ```bash
   # Docker
   docker compose up -d --force-recreate opensylab-backend

   # systemd
   sudo systemctl restart opensylab
   ```

   > There is currently **no hot-reload** of the JWT secret without a restart.
   > A restart is required for the new secret to take effect.

4. **Verify** (see section 3).

### Multi-instance / zero-downtime note

The secret is per-process. During a rolling restart of multiple backend
instances, tokens signed by an already-restarted instance (new secret) will be
rejected by not-yet-restarted instances (old secret) and vice versa. For a clean
rotation, prefer a brief maintenance window, or accept that users may need to log
in again during the rollout. Because tokens live only 60 minutes, the disruption
window is naturally bounded.

---

## 2. Audit HMAC key (`OPENSYLAB_AUDIT_HMAC_KEY`) — do not rotate on a populated DB

The audit trail is a tamper-evident hash chain: each `audit_log` row stores a
`chain_hash` computed as an HMAC-SHA256 over the entry's canonical fields **and**
the previous row's `chain_hash`, keyed by `OPENSYLAB_AUDIT_HMAC_KEY`
(`src/db/Database.cpp`). The `GET /api/v1/audit/verify` endpoint (ADMIN-only)
recomputes the entire chain with the **current** key and reports the first entry
whose hash does not match.

**Therefore: changing `OPENSYLAB_AUDIT_HMAC_KEY` on a database that already
contains audit entries will make the whole existing chain fail verification** —
every historical entry recomputes to a different HMAC, and `audit/verify` will
report the chain as broken from the first row. This is an ISO 15189 integrity
concern, not just an inconvenience.

Guidance:

- **Set this key once, before the first audit entry is written, and keep it
  stable for the lifetime of the database.**
- Store it in a secret manager; back it up alongside (but not in) the database.
  Losing it means the existing chain can no longer be verified.
- A safe re-keying of an existing chain would require re-signing every entry with
  the new key and recording that administrative action in the audit trail itself.
  **This is not currently supported** and must not be improvised by editing the
  environment variable. Track this under the v1.x roadmap (secret-rotation
  tooling) if a re-key capability is required.

If you must migrate to a new key for a fresh deployment, do so on an empty
`audit_log` (new database), not on production history.

---

## 3. Post-rotation verification

After rotating the JWT secret and restarting:

1. **Health check** — server is up:

   ```bash
   curl -s http://localhost:8080/api/v1/health
   # {"status":"ok","service":"opensylab-lims"}
   ```

2. **Fresh login works** with the new secret:

   ```bash
   curl -s -X POST http://localhost:8080/api/v1/auth/login \
     -H "Content-Type: application/json" \
     -d '{"username":"<user>","password":"<password>"}'
   # returns a new token
   ```

3. **Old tokens are rejected** — a request with a pre-rotation token returns
   HTTP 401.

4. **Audit chain still verifies** (confirms you did *not* accidentally change the
   HMAC key):

   ```bash
   curl -s http://localhost:8080/api/v1/audit/verify \
     -H "Authorization: Bearer <new-admin-token>"
   # {"valid":true,"message":"Audit chain integrity verified"}
   ```

---

## Related

- Environment variables reference: [`docs/DOCKER.md`](DOCKER.md)
- Security policy / vulnerability reporting: [`../SECURITY.md`](../SECURITY.md)
