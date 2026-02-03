# Backend Integration Guide

This document outlines the integration requirements between the OpenSylab React frontend and C++ backend.

## API Endpoints Required

The frontend expects the following REST API endpoints:

### Samples API

#### GET /api/v1/samples
Returns a paginated list of samples.

**Query Parameters:**
- `status` (optional): Filter by sample status (REGISTERED, IN_ANALYSIS, ANALYZED, VALIDATED, ARCHIVED)
- `from` (optional): Start date filter (ISO 8601 format)
- `to` (optional): End date filter (ISO 8601 format)
- `limit` (optional): Number of results per page (default: 20)
- `offset` (optional): Pagination offset (default: 0)

**Response:**
```json
{
  "samples": [
    {
      "id": 1,
      "sample_id": "S2024-001",
      "patient_id": "P12345",
      "patient_name": "John Doe",
      "description": "Blood sample",
      "status": "REGISTERED",
      "created_at": "2024-01-15T10:30:00Z",
      "updated_at": "2024-01-15T10:30:00Z"
    }
  ],
  "total": 100,
  "limit": 20,
  "offset": 0
}
```

#### GET /api/v1/samples/:id
Returns a single sample by ID.

**Response:**
```json
{
  "id": 1,
  "sample_id": "S2024-001",
  "patient_id": "P12345",
  "patient_name": "John Doe",
  "description": "Blood sample",
  "status": "REGISTERED",
  "created_at": "2024-01-15T10:30:00Z",
  "updated_at": "2024-01-15T10:30:00Z"
}
```

#### POST /api/v1/samples
Creates a new sample.

**Request Body:**
```json
{
  "sample_id": "S2024-001",
  "patient_id": "P12345",
  "patient_name": "John Doe",
  "description": "Blood sample",
  "status": "REGISTERED"
}
```

**Response:** Same as GET /api/v1/samples/:id

#### PUT /api/v1/samples/:id
Updates an existing sample.

**Request Body:** Same as POST (partial updates allowed)

**Response:** Same as GET /api/v1/samples/:id

#### DELETE /api/v1/samples/:id
Deletes a sample.

**Response:** 204 No Content

## Authentication

The frontend uses API key authentication via the `X-API-Key` header.

**Header:**
```
X-API-Key: your-api-key-here
```

**Authentication Flow:**
1. User enters API key in login form
2. Frontend validates key by making a test request to `/api/v1/samples?limit=1`
3. If request succeeds (200 OK), key is stored in localStorage
4. All subsequent requests include the `X-API-Key` header

**Error Handling:**
- 401 Unauthorized: Invalid or missing API key
- 403 Forbidden: Valid key but insufficient permissions

Frontend automatically:
- Clears stored API key on 401/403 responses
- Redirects user to login page
- Includes API key in all authenticated requests

## CORS Configuration

The backend MUST configure CORS to allow requests from the frontend:

### Development
```
Access-Control-Allow-Origin: http://localhost:5173
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, X-API-Key
Access-Control-Max-Age: 3600
```

### Production
```
Access-Control-Allow-Origin: https://your-production-domain.com
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, X-API-Key
Access-Control-Max-Age: 3600
```

### C++ Implementation Example (Crow/Pistache)

```cpp
// Add CORS headers to response
app.before_handle([](crow::request& req, crow::response& res, context& ctx){
    res.add_header("Access-Control-Allow-Origin", "http://localhost:5173");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, X-API-Key");
    res.add_header("Access-Control-Max-Age", "3600");
});

// Handle OPTIONS preflight requests
CROW_ROUTE(app, "/<path>").methods(crow::HTTPMethod::OPTIONS)
([](const crow::request& req) {
    return crow::response(204);
});
```

## Data Models

### Sample Status Enum
Valid values:
- `REGISTERED`: Sample registered in system
- `IN_ANALYSIS`: Currently being analyzed
- `ANALYZED`: Analysis completed
- `VALIDATED`: Results validated
- `ARCHIVED`: Sample archived

### Date Formats
All dates must be in ISO 8601 format:
```
2024-01-15T10:30:00Z
```

## Error Responses

The backend should return consistent error responses:

```json
{
  "error": "Error message here",
  "code": "ERROR_CODE",
  "details": "Additional details if available"
}
```

**Common HTTP Status Codes:**
- 200: Success
- 201: Created
- 204: No Content (successful deletion)
- 400: Bad Request (validation error)
- 401: Unauthorized (missing/invalid API key)
- 403: Forbidden (insufficient permissions)
- 404: Not Found
- 500: Internal Server Error

## Testing the Integration

### Test API Key Validation
```bash
curl -H "X-API-Key: test-key" http://localhost:8080/api/v1/samples?limit=1
```

### Test CORS
```bash
curl -H "Origin: http://localhost:5173" \
     -H "Access-Control-Request-Method: GET" \
     -H "Access-Control-Request-Headers: X-API-Key" \
     -X OPTIONS \
     http://localhost:8080/api/v1/samples
```

### Test Sample Creation
```bash
curl -X POST http://localhost:8080/api/v1/samples \
  -H "Content-Type: application/json" \
  -H "X-API-Key: test-key" \
  -d '{
    "sample_id": "S2024-001",
    "patient_id": "P12345",
    "patient_name": "John Doe",
    "description": "Blood sample",
    "status": "REGISTERED"
  }'
```

## Migration from HTTP to HTTPS

When the backend upgrades to HTTPS (port 8443):

1. Update frontend environment variables:
   ```
   VITE_API_URL=https://localhost:8443/api/v1
   ```

2. Update CORS origin in backend:
   ```
   Access-Control-Allow-Origin: https://localhost:5173
   ```

3. No code changes required in frontend - the Axios client automatically handles HTTPS

4. For development with self-signed certificates, you may need to:
   - Accept the certificate in browser
   - Or configure Axios to ignore certificate validation (not recommended for production)

## Performance Considerations

### Pagination
- Default page size: 20 items
- Maximum page size: 100 items
- Frontend calculates total pages: `Math.ceil(total / limit)`

### Caching
- Frontend does not cache API responses
- Backend should implement appropriate caching strategies
- Use ETags/Last-Modified headers for conditional requests (future enhancement)

### Rate Limiting
If backend implements rate limiting:
- Return 429 Too Many Requests
- Include `Retry-After` header
- Frontend will display error to user
