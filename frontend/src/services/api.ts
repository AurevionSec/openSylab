import axios from 'axios';
import { JWT_TOKEN_STORAGE_KEY, API_KEY_STORAGE_KEY, USER_INFO_STORAGE_KEY } from '../utils/constants';


const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || 'http://localhost:8080/api/v1',
  headers: {
    'Content-Type': 'application/json',
  },
});

// Dispatch a custom event so AuthContext (inside the React tree) can handle
// logout and navigate via React Router, avoiding the back-button breakage that
// window.location.href causes (bypasses React Router history).
const dispatchAuthExpired = () =>
  window.dispatchEvent(new Event('opensylab:auth-expired'));

// Add JWT Bearer token interceptor
api.interceptors.request.use((config) => {
  // Try JWT token first
  const jwtToken = localStorage.getItem(JWT_TOKEN_STORAGE_KEY);
  if (jwtToken) {
    const _expiry = localStorage.getItem('opensylab_token_expiry');
    const _isExpired = !_expiry || Date.now() >= parseInt(_expiry, 10);
    if (_isExpired) {
      localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
      localStorage.removeItem(USER_INFO_STORAGE_KEY);
      localStorage.removeItem('opensylab_token_expiry');
      localStorage.removeItem(API_KEY_STORAGE_KEY);
      dispatchAuthExpired();
      return Promise.reject(new Error('Token expired'));
    }
    config.headers['Authorization'] = `Bearer ${jwtToken}`;
    return config;
  }

  // Fall back to API-Key for backward compatibility
  const apiKey = localStorage.getItem(API_KEY_STORAGE_KEY);
  if (apiKey) {
    config.headers['X-API-Key'] = apiKey;
  }

  return config;
});

// Add response interceptor for error handling
api.interceptors.response.use(
  (response) => response,
  (error) => {
    const status = error.response?.status;
    // Only 401 signals a truly expired/invalid token — clear the session.
    // 403 means the token is valid but the role is insufficient (RBAC denial);
    // logging out on 403 would boot authenticated VIEWER/CUSTOM users.
    const isLoginEndpoint = (error.config?.url ?? '').includes('/auth/login');
    if (status === 401 && !isLoginEndpoint) {
      // Session expired — clear auth and signal React to navigate.
      localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
      localStorage.removeItem(USER_INFO_STORAGE_KEY);
      localStorage.removeItem('opensylab_token_expiry');
      localStorage.removeItem(API_KEY_STORAGE_KEY);
      dispatchAuthExpired();
    }
    return Promise.reject(error);
  }
);

export default api;
