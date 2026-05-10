import axios from 'axios';
import { JWT_TOKEN_STORAGE_KEY, API_KEY_STORAGE_KEY, USER_INFO_STORAGE_KEY } from '../utils/constants';
import { isTokenExpired } from './auth';

const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || 'http://localhost:8080/api/v1',
  headers: {
    'Content-Type': 'application/json',
  },
});

// Add JWT Bearer token interceptor
api.interceptors.request.use((config) => {
  // Try JWT token first
  const jwtToken = localStorage.getItem(JWT_TOKEN_STORAGE_KEY);
  if (jwtToken) {
    if (isTokenExpired()) {
      localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
      localStorage.removeItem(USER_INFO_STORAGE_KEY);
      if (window.location.pathname !== '/login') {
        window.location.href = '/login';
      }
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
    if (error.response?.status === 401 || error.response?.status === 403) {
      // Unauthorized - clear authentication and redirect to login
      localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
      localStorage.removeItem(USER_INFO_STORAGE_KEY);
      localStorage.removeItem('opensylab_token_expiry');
      localStorage.removeItem(API_KEY_STORAGE_KEY);

      // Only redirect if not already on login page
      if (window.location.pathname !== '/login') {
        window.location.href = '/login';
      }
    }
    return Promise.reject(error);
  }
);

export default api;
