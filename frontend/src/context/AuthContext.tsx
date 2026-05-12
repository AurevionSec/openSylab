import { createContext, useContext, useState, useEffect } from 'react';
import type { ReactNode } from 'react';
import {
  login as loginService,
  logout as logoutService,
  isAuthenticated as checkAuth,
  getStoredUser,
  type User,
} from '../services/auth';

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
      // Load user info from storage
      const storedUser = getStoredUser();
      setUser(storedUser);
      setMustChangePassword(localStorage.getItem('opensylab_must_change_pw') === 'true');
    }

    setLoading(false);
  }, []);

  const login = async (username: string, password: string, mfaCode?: string): Promise<{ success: boolean; error?: string; mfaRequired?: boolean; mustChangePassword?: boolean }> => {
    const result = await loginService(username, password, mfaCode);

    if (result.success && result.user) {
      setIsAuthenticated(true);
      setUser(result.user);
      const mcp = result.user.must_change_password === true;
      setMustChangePassword(mcp);
      localStorage.setItem('opensylab_must_change_pw', mcp ? 'true' : 'false');
      return { success: true, mustChangePassword: mcp };
    }

    return { success: false, error: result.error, mfaRequired: result.mfaRequired };
  };

  const clearMustChangePassword = () => {
    setMustChangePassword(false);
    localStorage.setItem('opensylab_must_change_pw', 'false');
  };

  const logout = () => {
    logoutService();
    setIsAuthenticated(false);
    setUser(null);
    setMustChangePassword(false);
    localStorage.removeItem('opensylab_must_change_pw');
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
