import type { ReactNode } from 'react';
import { Navigate, useLocation } from 'react-router-dom';
import { useAuth } from '../../context/AuthContext';

interface ProtectedRouteProps {
  children: ReactNode;
  requiredRole?: string;
}

export const ProtectedRoute = ({ children, requiredRole }: ProtectedRouteProps) => {
  const { isAuthenticated, user, loading, mustChangePassword } = useAuth();
  const location = useLocation();

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <div className="text-center">
          <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
          <p className="mt-4 text-gray-600">Loading...</p>
        </div>
      </div>
    );
  }

  if (!isAuthenticated) {
    return <Navigate to="/login" replace />;
  }

  // isAuthenticated but user not loaded (localStorage corruption) — force re-login
  if (!user) {
    return <Navigate to="/login" replace />;
  }

  // Force password change: redirect non-profile routes to change password
  if (mustChangePassword && location.pathname !== '/profile') {
    return <Navigate to="/profile?force_change=1" replace />;
  }

  // Check role-based access using hierarchy (ADMIN > OPERATOR > VIEWER > CUSTOM)
  if (requiredRole) {
    const ROLE_LEVELS: Record<string, number> = { ADMIN: 4, OPERATOR: 3, VIEWER: 2, CUSTOM: 1 };
    const required = ROLE_LEVELS[requiredRole] ?? 0;
    const actual = ROLE_LEVELS[user.role] ?? 0;
    if (actual < required) {
      return <Navigate to="/" replace />;
    }
  }

  return <>{children}</>;
};
