import { createContext, useContext, useState, useEffect } from 'react';
import type { ReactNode } from 'react';
import {
  login as loginService,
  logout as logoutService,
  isAuthenticated as checkAuth,
  getStoredUser,
  type User,
} from '../services/auth';
import { USER_INFO_STORAGE_KEY } from '../utils/constants';

interface AuthContextType {
  isAuthenticated: boolean;
  user: User | null;
  mustChangePassword: boolean;
  login: (username: string, password: string, mfaCode?: string) => Promise<{ success: boolean; error?: string; mfaRequired?: boolean; mustChangePassword?: boolean }>;
  logout: () => void;
  loading: boolean;
  clearMustChangePassword: () => void;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const AuthProvider = ({ children }: { children: ReactNode }) => {
  const [isAuthenticated, setIsAuthenticated] = useState<boolean>(false);
  const [user, setUser] = useState<User | null>(null);
  const [mustChangePassword, setMustChangePassword] = useState<boolean>(false);
  const [loading, setLoading] = useState<boolean>(true);

  useEffect(() => {
    // Check if user is already authenticated
    const authenticated = checkAuth();
    setIsAuthenticated(authenticated);

    if (authenticated) {
      // Load user info from storage — derive mustChangePassword from the user
      // object, not from a separate localStorage key that can be trivially bypassed.
      const storedUser = getStoredUser();
      setUser(storedUser);
      setMustChangePassword(storedUser?.must_change_password === true);
    }

    setLoading(false);

    // React to token expiry/401 events fired by the axios interceptor (api.ts).
    // Using an event avoids window.location.href which breaks React Router history.
    const handleAuthExpired = () => {
      setIsAuthenticated(false);
      setUser(null);
      setMustChangePassword(false);
    };
    window.addEventListener('opensylab:auth-expired', handleAuthExpired);
    return () => {
      window.removeEventListener('opensylab:auth-expired', handleAuthExpired);
    };
  }, []);

  const login = async (username: string, password: string, mfaCode?: string): Promise<{ success: boolean; error?: string; mfaRequired?: boolean; mustChangePassword?: boolean }> => {
    const result = await loginService(username, password, mfaCode);

    if (result.success && result.user) {
      setIsAuthenticated(true);
      setUser(result.user);
      const mcp = result.user.must_change_password === true;
      setMustChangePassword(mcp);
      return { success: true, mustChangePassword: mcp };
    }

    return { success: false, error: result.error, mfaRequired: result.mfaRequired };
  };

  const clearMustChangePassword = () => {
    setMustChangePassword(false);
    // Update the stored user object so the flag persists correctly across reloads
    const storedUser = getStoredUser();
    if (storedUser) {
      storedUser.must_change_password = false;
      localStorage.setItem(USER_INFO_STORAGE_KEY, JSON.stringify(storedUser));
    }
  };

  const logout = () => {
    logoutService();
    setIsAuthenticated(false);
    setUser(null);
    setMustChangePassword(false);
  };

  return (
    <AuthContext.Provider value={{ isAuthenticated, user, mustChangePassword, login, logout, loading, clearMustChangePassword }}>
      {children}
    </AuthContext.Provider>
  );
};

export const useAuth = (): AuthContextType => {
  const context = useContext(AuthContext);
  if (context === undefined) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
};
