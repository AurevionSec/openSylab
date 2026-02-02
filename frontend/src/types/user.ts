// Authentication context
export interface AuthUser {
  apiKey: string;
  isAuthenticated: boolean;
}

// User management types
export type UserRole = 'ADMIN' | 'OPERATOR' | 'VIEWER' | 'CUSTOM';

export interface User {
  id: number;
  username: string;
  role: UserRole;
  active: boolean;
  created_at: number;
  last_login: number;
  full_name?: string;
  email?: string;
}

export interface CreateUserPayload {
  username: string;
  password: string;
  role?: UserRole;
  full_name?: string;
  email?: string;
  active?: boolean;
}

export interface UpdateUserPayload {
  username?: string;
  role?: UserRole;
  full_name?: string;
  email?: string;
  active?: boolean;
  password?: string;
}

export interface ChangePasswordPayload {
  current_password: string;
  new_password: string;
}

export const USER_ROLES: Record<UserRole, string> = {
  ADMIN: 'Administrator',
  OPERATOR: 'Operator',
  VIEWER: 'Viewer',
  CUSTOM: 'Custom',
};

export const ROLE_COLORS: Record<UserRole, string> = {
  ADMIN: 'bg-red-100 text-red-800',
  OPERATOR: 'bg-blue-100 text-blue-800',
  VIEWER: 'bg-gray-100 text-gray-800',
  CUSTOM: 'bg-purple-100 text-purple-800',
};
