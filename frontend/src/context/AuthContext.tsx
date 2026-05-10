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
  login: (username: string, password: string) => Promise<{ success: boolean; error?: string }>;
  logout: () => void;
  loading: boolean;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const AuthProvider = ({ children }: { children: ReactNode }) => {
  const [isAuthenticated, setIsAuthenticated] = useState<boolean>(false);
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState<boolean>(true);

  useEffect(() => {
    // Check if user is already authenticated
    const authenticated = checkAuth();
    setIsAuthenticated(authenticated);

    if (authenticated) {
      // Load user info from storage
      const storedUser = getStoredUser();
      setUser(storedUser);
    }

    setLoading(false);
  }, []);

  const login = async (username: string, password: string): Promise<{ success: boolean; error?: string }> => {
    const result = await loginService(username, password);

    if (result.success && result.user) {
      setIsAuthenticated(true);
      setUser(result.user);
      return { success: true };
    }

    return { success: false, error: result.error };
  };

  const logout = () => {
    logoutService();
    setIsAuthenticated(false);
    setUser(null);
  };

  return (
    <AuthContext.Provider value={{ isAuthenticated, user, login, logout, loading }}>
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
