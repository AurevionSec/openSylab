// Shared E2E endpoints. A non-standard backend port is used deliberately so the
// suite never collides with other services a developer may run on 8080/9080.
export const BACKEND_PORT = 18080
export const BACKEND_URL = `http://localhost:${BACKEND_PORT}`
export const API_BASE = `${BACKEND_URL}/api/v1`

export const FRONTEND_PORT = 5173
export const FRONTEND_URL = `http://localhost:${FRONTEND_PORT}`
