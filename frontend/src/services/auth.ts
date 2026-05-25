import api from './api';
import { JWT_TOKEN_STORAGE_KEY, USER_INFO_STORAGE_KEY, API_KEY_STORAGE_KEY } from '../utils/constants';

export interface User {
  id: number;
  username: string;
  role: string;
  must_change_password?: boolean;
}

export interface LoginResponse {
  token: string;
  user: User;
  expiresIn: number;
}

/**
 * Login with username and password (JWT authentication)
 */
export const login = async (username: string, password: string, mfaCode?: string): Promise<{ success: boolean; user?: User; error?: string; mfaRequired?: boolean }> => {
  try {
    const body: Record<string, string> = { username, password };
    if (mfaCode) body.mfa_code = mfaCode;

    const response = await api.post<LoginResponse>('/auth/login', body);


    const { token, user: rawUser, expiresIn } = response.data;
    // Normalize role to uppercase short form for consistent RBAC comparison
    const roleMap: Record<string, string> = {
      'Administrator': 'ADMIN', 'admin': 'ADMIN',
      'Operator': 'OPERATOR', 'operator': 'OPERATOR',
      'Betrachter': 'VIEWER', 'viewer': 'VIEWER',
      'Custom': 'CUSTOM', 'custom': 'CUSTOM', 'Benutzerdefiniert': 'CUSTOM', 'Unbekannt': 'VIEWER',
    };
    const rawRole: string = rawUser.role ?? '';
    const user = { ...rawUser, role: roleMap[rawRole] ?? (rawRole ? rawRole.toUpperCase() : 'VIEWER') };

    // Store JWT token and user info
    localStorage.setItem(JWT_TOKEN_STORAGE_KEY, token);
    localStorage.setItem(USER_INFO_STORAGE_KEY, JSON.stringify(user));

    // Calculate and store expiration time
    const expirationTime = Date.now() + expiresIn * 1000;
    localStorage.setItem('opensylab_token_expiry', expirationTime.toString());


    return { success: true, user };
  } catch (error: unknown) {
    let errorMessage = 'An error occurred. Please try again.';
    let mfaRequired = false;

    if (error && typeof error === 'object' && 'response' in error) {
      const r = error as { response?: { status?: number; data?: { error?: { code?: string; message?: string } } } };
      if (r.response?.status === 403 && r.response?.data?.error?.code === 'mfa_required') {
        mfaRequired = true;
        errorMessage = r.response.data?.error?.message ?? 'MFA code required.';
      } else if (r.response?.status === 401) {
        errorMessage = 'Invalid username or password.';
      } else if (r.response?.data?.error?.message) {
        errorMessage = r.response.data.error.message;
      }
    }

    return { success: false, error: errorMessage, mfaRequired };
  }
};

/**
 * Logout - clear all authentication data
 */
export const logout = (): void => {
  // Best-effort: notify the backend so it can blacklist the token.
  // Fire-and-forget — the UI must not block on this call.
  api.post('/auth/logout').catch(() => {});

  localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
  localStorage.removeItem(USER_INFO_STORAGE_KEY);
  localStorage.removeItem('opensylab_token_expiry');
  localStorage.removeItem(API_KEY_STORAGE_KEY);
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

    const response = await api.get('/samples', {
      headers: {
        'X-API-Key': apiKey,
      },
      params: {
        limit: 1,
      },
    });

    return response.status === 200;
  } catch (error) {
    console.error('[AUTH] API key validation error:', error);
    return false;
  }
};
