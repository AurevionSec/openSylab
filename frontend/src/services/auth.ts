import api from './api';
import { JWT_TOKEN_STORAGE_KEY, USER_INFO_STORAGE_KEY, API_KEY_STORAGE_KEY } from '../utils/constants';

export interface User {
  id: number;
  username: string;
  role: string;
}

export interface LoginResponse {
  token: string;
  user: User;
  expiresIn: number;
}

/**
 * Login with username and password (JWT authentication)
 */
export const login = async (username: string, password: string): Promise<{ success: boolean; user?: User; error?: string }> => {
  try {
    console.log('[AUTH] Logging in with username:', username);
    console.log('[AUTH] API URL:', import.meta.env.VITE_API_URL || 'http://localhost:8080/api/v1');

    const response = await api.post<LoginResponse>('/auth/login', {
      username,
      password,
    });

    console.log('[AUTH] Login successful:', response.data);

    const { token, user, expiresIn } = response.data;

    // Store JWT token and user info
    localStorage.setItem(JWT_TOKEN_STORAGE_KEY, token);
    localStorage.setItem(USER_INFO_STORAGE_KEY, JSON.stringify(user));

    // Calculate and store expiration time
    const expirationTime = Date.now() + expiresIn * 1000;
    localStorage.setItem('opensylab_token_expiry', expirationTime.toString());

    console.log('[AUTH] Token stored, expires in:', expiresIn, 'seconds');

    return { success: true, user };
  } catch (error: any) {
    console.error('[AUTH] Login error:', error);

    let errorMessage = 'An error occurred. Please try again.';

    if (error.response) {
      console.error('[AUTH] Response status:', error.response.status);
      console.error('[AUTH] Response data:', error.response.data);

      if (error.response.status === 401) {
        errorMessage = 'Invalid username or password.';
      } else if (error.response.data?.error?.message) {
        errorMessage = error.response.data.error.message;
      }
    }

    return { success: false, error: errorMessage };
  }
};

/**
 * Logout - clear all authentication data
 */
export const logout = (): void => {
  localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
  localStorage.removeItem(USER_INFO_STORAGE_KEY);
  localStorage.removeItem('opensylab_token_expiry');

  // Also remove legacy API key if present
  localStorage.removeItem(API_KEY_STORAGE_KEY);

  console.log('[AUTH] Logged out, all tokens cleared');
};

/**
 * Get stored JWT token
 */
export const getStoredToken = (): string | null => {
  return localStorage.getItem(JWT_TOKEN_STORAGE_KEY);
};

/**
 * Get stored user information
 */
export const getStoredUser = (): User | null => {
  const userJson = localStorage.getItem(USER_INFO_STORAGE_KEY);
  if (!userJson) return null;

  try {
    return JSON.parse(userJson);
  } catch (error) {
    console.error('[AUTH] Failed to parse user info:', error);
    return null;
  }
};

/**
 * Check if token is expired
 */
export const isTokenExpired = (): boolean => {
  const expiryStr = localStorage.getItem('opensylab_token_expiry');
  if (!expiryStr) return true;

  const expiryTime = parseInt(expiryStr, 10);
  const now = Date.now();

  return now >= expiryTime;
};

/**
 * Check if user is authenticated (has valid token)
 */
export const isAuthenticated = (): boolean => {
  const token = getStoredToken();
  if (!token) return false;

  // Check if token is expired
  if (isTokenExpired()) {
    console.log('[AUTH] Token expired, clearing authentication');
    logout();
    return false;
  }

  return true;
};

/**
 * Legacy API-Key validation (for backward compatibility)
 * @deprecated Use login() with username/password instead
 */
export const validateApiKey = async (apiKey: string): Promise<boolean> => {
  try {
    console.log('[AUTH] Validating API key (legacy):', apiKey.substring(0, 10) + '...');

    const response = await api.get('/samples', {
      headers: {
        'X-API-Key': apiKey,
      },
      params: {
        limit: 1,
      },
    });

    console.log('[AUTH] API key validation response:', response.status);
    return response.status === 200;
  } catch (error) {
    console.error('[AUTH] API key validation error:', error);
    return false;
  }
};
