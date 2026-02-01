# JWT Authentication Implementation Plan

**Date:** 2026-02-01
**Status:** Planning
**Estimated Time:** 3-5 days

---

## Current State Analysis

### Backend Authentication (src/api/ApiServer.cpp)

**Current Flow:**
1. Client sends request with `X-API-Key` or `Authorization: Bearer <api-key>`
2. `extractApiKey()` extracts token from headers (line 219)
3. `ApiRouter::handleRequest()` validates with `database_->isApiKeyValid()` (line 510)
4. If invalid → 401 Unauthorized

**Problem:**
- `Authorization: Bearer` is treated as API-Key, not JWT
- No token expiration
- No user context in requests
- No token refresh mechanism

### Frontend Authentication (frontend/src/services/auth.ts)

**Current Flow:**
1. User enters API-Key in login form
2. `validateApiKey()` tests key with GET /samples
3. If valid → store in localStorage as `opensylab_api_key`
4. API client injects key via interceptor (X-API-Key header)

**Problem:**
- API-Key stored in plaintext
- No automatic logout on expiration
- No user information displayed

---

## Implementation Steps

### Phase 1: JWT Library Integration (Day 1)

**1.1 Choose JWT Library**
- **Option A:** `jwt-cpp` (header-only, C++11+, MIT license)
  - Repository: https://github.com/Thalhammer/jwt-cpp
  - Pros: Header-only, no dependencies, widely used
  - Cons: Requires C++11 (we have C++17 ✓)

**1.2 Add to CMakeLists.txt**
```cmake
# Fetch jwt-cpp
include(FetchContent)
FetchContent_Declare(
  jwt-cpp
  GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
  GIT_TAG        v0.7.0
)
FetchContent_MakeAvailable(jwt-cpp)

target_link_libraries(OpenSylab PRIVATE jwt-cpp::jwt-cpp)
```

**1.3 Create JwtAuth Class**
- `include/auth/JwtAuth.h`
- `src/auth/JwtAuth.cpp`

```cpp
class JwtAuth {
public:
  struct JwtConfig {
    std::string secret;
    int expirationMinutes = 60;
    std::string issuer = "opensylab";
  };

  struct TokenPayload {
    int userId;
    std::string username;
    std::string role;
    std::time_t exp;
  };

  explicit JwtAuth(const JwtConfig& config);

  // Generate JWT token
  std::string generateToken(int userId, const std::string& username,
                           const std::string& role);

  // Validate and decode JWT token
  std::optional<TokenPayload> validateToken(const std::string& token);

  // Check if token is expired
  bool isTokenExpired(const TokenPayload& payload);

private:
  JwtConfig config_;
};
```

---

### Phase 2: Login Endpoint (Day 2)

**2.1 Add POST /api/v1/auth/login**

Request:
```json
{
  "username": "admin",
  "password": "password123"
}
```

Response (Success):
```json
{
  "success": true,
  "token": "eyJhbGciOiJIUzI1NiIs...",
  "user": {
    "id": 1,
    "username": "admin",
    "role": "admin"
  },
  "expiresIn": 3600
}
```

Response (Failure):
```json
{
  "error": {
    "code": "invalid_credentials",
    "message": "Invalid username or password"
  }
}
```

**2.2 Modify ApiRouter::handleRequest()**
```cpp
// Add login endpoint (no auth required)
if (method == "post" && path == "/auth/login") {
  return handleLogin(request);
}

// THEN check authentication for other endpoints
const std::string token = extractAuthToken(request.headers);
auto payload = jwtAuth_->validateToken(token);
if (!payload) {
  // Fallback to API-Key for backward compatibility
  const std::string apiKey = extractApiKey(request.headers);
  if (apiKey.empty() || !database_->isApiKeyValid(apiKey)) {
    return 401 Unauthorized;
  }
}
```

**2.3 Implement handleLogin()**
```cpp
ApiResponse ApiRouter::handleLogin(const ApiRequest& request) {
  auto json = nlohmann::json::parse(request.body);
  std::string username = json["username"];
  std::string password = json["password"];

  auto user = database_->authenticateUser(username, password);
  if (!user) {
    return makeError(401, "invalid_credentials", "Invalid username or password");
  }

  std::string token = jwtAuth_->generateToken(user->id, user->username, user->role);

  nlohmann::json response;
  response["success"] = true;
  response["token"] = token;
  response["user"]["id"] = user->id;
  response["user"]["username"] = user->username;
  response["user"]["role"] = user->role;
  response["expiresIn"] = jwtConfig_.expirationMinutes * 60;

  return ApiResponse{200, response.dump(), "application/json"};
}
```

---

### Phase 3: Token Validation (Day 2-3)

**3.1 Create extractAuthToken()**
```cpp
std::optional<JwtAuth::TokenPayload> extractAndValidateToken(
    const std::unordered_map<std::string, std::string>& headers,
    JwtAuth* jwtAuth) {

  auto it = headers.find("authorization");
  if (it == headers.end()) {
    return std::nullopt;
  }

  const std::string value = trim(it->second);
  const std::string prefix = "bearer ";

  if (value.size() <= prefix.size() ||
      toLower(value.substr(0, prefix.size())) != prefix) {
    return std::nullopt;
  }

  std::string token = trim(value.substr(prefix.size()));
  return jwtAuth->validateToken(token);
}
```

**3.2 Update ApiRouter Authentication Logic**
```cpp
ApiResponse ApiRouter::handleRequest(const ApiRequest& request) {
  const std::string method = toLower(request.method);

  // CORS preflight
  if (method == "options") {
    return ApiResponse{200, "", "text/plain"};
  }

  // Login endpoint (no auth required)
  if (method == "post" && request.path == "/auth/login") {
    return handleLogin(request);
  }

  // Try JWT authentication first
  auto tokenPayload = extractAndValidateToken(request.headers, jwtAuth_.get());

  if (!tokenPayload) {
    // Fallback to API-Key (backward compatibility)
    const std::string apiKey = extractApiKey(request.headers);
    if (apiKey.empty() || !database_->isApiKeyValid(apiKey)) {
      return makeError(401, "unauthorized", "Invalid credentials",
                       "Provide valid JWT token or API-Key");
    }
    // API-Key valid → continue with empty user context
  } else {
    // JWT valid → attach user context to request
    request.userId = tokenPayload->userId;
    request.username = tokenPayload->username;
    request.userRole = tokenPayload->role;
  }

  // Continue with endpoint routing...
}
```

---

### Phase 4: Frontend Integration (Day 3-4)

**4.1 Update auth.ts Login Flow**
```typescript
export interface LoginResponse {
  success: boolean;
  token: string;
  user: {
    id: number;
    username: string;
    role: string;
  };
  expiresIn: number;
}

export const login = async (username: string, password: string): Promise<boolean> => {
  try {
    const response = await api.post<LoginResponse>('/auth/login', {
      username,
      password,
    });

    if (response.data.success) {
      // Store JWT token
      localStorage.setItem(API_KEY_STORAGE_KEY, response.data.token);

      // Store user info
      localStorage.setItem('opensylab_user', JSON.stringify(response.data.user));

      return true;
    }
    return false;
  } catch (error) {
    console.error('[AUTH] Login error:', error);
    return false;
  }
};
```

**4.2 Update API Interceptor**
```typescript
// Update frontend/src/services/api.ts
api.interceptors.request.use((config) => {
  const token = localStorage.getItem(API_KEY_STORAGE_KEY);
  if (token) {
    // Send as Bearer token (JWT)
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});
```

**4.3 Update Login Form**
```typescript
// Update frontend/src/pages/Login.tsx
const [username, setUsername] = useState('');
const [password, setPassword] = useState('');

const handleSubmit = async (e: React.FormEvent) => {
  e.preventDefault();
  const success = await login(username, password);
  if (success) {
    navigate('/dashboard');
  } else {
    setError('Invalid username or password');
  }
};
```

---

### Phase 5: Token Refresh (Day 4-5)

**5.1 Add Refresh Endpoint**
POST /api/v1/auth/refresh
- Accepts valid (but expiring) JWT
- Issues new JWT with extended expiration
- Updates session timestamp

**5.2 Frontend Auto-Refresh**
```typescript
// Check token expiration on app load
const token = localStorage.getItem(API_KEY_STORAGE_KEY);
if (token) {
  const payload = parseJwt(token);
  const expiresIn = payload.exp - Date.now() / 1000;

  if (expiresIn < 300) { // Less than 5 minutes
    await refreshToken();
  } else if (expiresIn < 0) { // Expired
    logout();
    navigate('/login');
  }
}
```

---

### Phase 6: Testing (Day 5)

**6.1 Backend Tests**
```cpp
TEST_CASE("JWT Authentication", "[auth][jwt]") {
  JwtAuth::JwtConfig config{"secret123", 60, "opensylab"};
  JwtAuth auth(config);

  SECTION("Generate and validate token") {
    std::string token = auth.generateToken(1, "admin", "admin");
    auto payload = auth.validateToken(token);

    REQUIRE(payload.has_value());
    REQUIRE(payload->userId == 1);
    REQUIRE(payload->username == "admin");
    REQUIRE(payload->role == "admin");
  }

  SECTION("Reject invalid token") {
    auto payload = auth.validateToken("invalid.token.here");
    REQUIRE_FALSE(payload.has_value());
  }

  SECTION("Reject expired token") {
    // Test with expiration = -1 minute
    JwtAuth::JwtConfig shortConfig{"secret123", -1, "opensylab"};
    JwtAuth shortAuth(shortConfig);
    std::string token = shortAuth.generateToken(1, "admin", "admin");

    auto payload = shortAuth.validateToken(token);
    REQUIRE_FALSE(payload.has_value()); // Should be expired
  }
}
```

**6.2 Integration Tests**
```cpp
TEST_CASE("Login Endpoint", "[api][auth]") {
  // Create test user
  Database db(":memory:");
  db.open();
  db.initializeSchema();

  core::User user{};
  user.username = "testuser";
  user.passwordHash = "hashed_password";
  user.role = "user";
  db.createUser(user);

  // Test login
  ApiRequest request;
  request.method = "POST";
  request.path = "/auth/login";
  request.body = R"({"username":"testuser","password":"password123"})";

  ApiRouter router(&db);
  ApiResponse response = router.handleRequest(request);

  REQUIRE(response.status == 200);
  auto json = nlohmann::json::parse(response.body);
  REQUIRE(json["success"] == true);
  REQUIRE(json.contains("token"));
  REQUIRE(json["user"]["username"] == "testuser");
}
```

---

## Migration Strategy

### Backward Compatibility

**During Transition (v0.5.0 - v0.5.2):**
- Support BOTH JWT and API-Key authentication
- Log deprecation warnings for API-Key usage
- Update documentation to recommend JWT

**Deprecation (v0.6.0):**
- Mark API-Key authentication as deprecated
- Add migration guide
- Provide tool to generate JWT from API-Key

**Removal (v1.0.0):**
- Remove API-Key support entirely
- JWT-only authentication

---

## Configuration

### Backend Config (opensylab.conf)
```ini
[auth]
jwt_secret = your-secret-key-here-change-in-production
jwt_expiration_minutes = 60
jwt_issuer = opensylab
allow_api_key_fallback = true  # Set to false in v0.6.0
```

### Environment Variables (Production)
```bash
OPENSYLAB_JWT_SECRET=your-production-secret-key-min-256-bits
OPENSYLAB_JWT_EXPIRATION=60
```

---

## Security Considerations

1. **JWT Secret:**
   - Minimum 256 bits (32 bytes)
   - Generated randomly for each installation
   - Stored securely (environment variable, secrets manager)
   - Never committed to version control

2. **Token Expiration:**
   - Default: 60 minutes
   - Configurable based on security requirements
   - Shorter for high-security environments (15-30 min)

3. **Password Hashing:**
   - Use bcrypt or Argon2 (already implemented in Database?)
   - Check current implementation in `authenticateUser()`

4. **HTTPS Requirement:**
   - JWT tokens MUST be transmitted over HTTPS
   - Enforce in production configuration
   - Log warning if HTTP is used with JWT

---

## Timeline

| Day | Task | Estimated Time |
|-----|------|----------------|
| 1 | JWT library integration + JwtAuth class | 6-8 hours |
| 2 | Login endpoint + token validation | 6-8 hours |
| 3 | Frontend login form + interceptor | 4-6 hours |
| 4 | Token refresh + auto-logout | 4-6 hours |
| 5 | Testing + documentation | 4-6 hours |
| **Total** | | **24-34 hours (3-5 days)** |

---

## Success Criteria

- ✅ User can login with username/password
- ✅ JWT token is generated and returned
- ✅ JWT token is validated on API requests
- ✅ Token expiration is enforced
- ✅ Frontend stores and sends JWT correctly
- ✅ Backward compatibility with API-Key maintained
- ✅ All tests passing
- ✅ Documentation updated

---

## Next Steps

1. Review this plan with team
2. Start with Phase 1: JWT library integration
3. Implement incrementally, test each phase
4. Update TODO.md as tasks complete

---

**Last Updated:** 2026-02-01
**Author:** Development Team
**Status:** Ready for Implementation
