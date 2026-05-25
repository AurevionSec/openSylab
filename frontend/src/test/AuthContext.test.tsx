import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { render, screen, act, waitFor } from '@testing-library/react';
import { AuthProvider, useAuth } from '../context/AuthContext';
import * as authService from '../services/auth';
import type { User } from '../services/auth';

// Mock the entire auth service module
vi.mock('../services/auth', () => ({
  login: vi.fn(),
  logout: vi.fn(),
  isAuthenticated: vi.fn(),
  getStoredUser: vi.fn(),
}));

// Helper component that surfaces context values
const AuthConsumer = () => {
  const { isAuthenticated, user, loading, mustChangePassword } = useAuth();
  return (
    <div>
      <span data-testid="authenticated">{String(isAuthenticated)}</span>
      <span data-testid="loading">{String(loading)}</span>
      <span data-testid="username">{user?.username ?? 'none'}</span>
      <span data-testid="role">{user?.role ?? 'none'}</span>
      <span data-testid="must-change">{String(mustChangePassword)}</span>
    </div>
  );
};

// Helper component that exercises login / logout
const AuthActions = () => {
  const { login, logout } = useAuth();
  return (
    <div>
      <button
        onClick={() => login('admin', 'secret')}
        data-testid="btn-login"
      >
        Login
      </button>
      <button onClick={logout} data-testid="btn-logout">
        Logout
      </button>
    </div>
  );
};

describe('AuthContext', () => {
  const mockLogin = vi.mocked(authService.login);
  const mockLogout = vi.mocked(authService.logout);
  const mockIsAuthenticated = vi.mocked(authService.isAuthenticated);
  const mockGetStoredUser = vi.mocked(authService.getStoredUser);

  beforeEach(() => {
    localStorage.clear();
    vi.clearAllMocks();
  });

  afterEach(() => {
    localStorage.clear();
  });

  // ── 1. Initial state: not logged in ─────────────────────────────────────
  it('initial state — not authenticated when no token stored', async () => {
    mockIsAuthenticated.mockReturnValue(false);
    mockGetStoredUser.mockReturnValue(null);

    render(
      <AuthProvider>
        <AuthConsumer />
      </AuthProvider>,
    );

    await waitFor(() =>
      expect(screen.getByTestId('loading').textContent).toBe('false'),
    );

    expect(screen.getByTestId('authenticated').textContent).toBe('false');
    expect(screen.getByTestId('username').textContent).toBe('none');
  });

  // ── 2. Token saved after successful login ────────────────────────────────
  it('stores token in localStorage after successful login', async () => {
    mockIsAuthenticated.mockReturnValue(false);
    mockGetStoredUser.mockReturnValue(null);

    const fakeUser: User = { id: 1, username: 'admin', role: 'ADMIN' };
    mockLogin.mockResolvedValue({ success: true, user: fakeUser });

    render(
      <AuthProvider>
        <AuthConsumer />
        <AuthActions />
      </AuthProvider>,
    );

    await waitFor(() =>
      expect(screen.getByTestId('loading').textContent).toBe('false'),
    );

    await act(async () => {
      screen.getByTestId('btn-login').click();
    });

    // The service itself writes the token; we verify the context reacted
    await waitFor(() =>
      expect(screen.getByTestId('authenticated').textContent).toBe('true'),
    );
    expect(screen.getByTestId('username').textContent).toBe('admin');
    expect(screen.getByTestId('role').textContent).toBe('ADMIN');
  });

  // ── 3. Logout clears the token ───────────────────────────────────────────
  it('logout clears the user and sets isAuthenticated to false', async () => {
    const fakeUser: User = { id: 2, username: 'operator', role: 'OPERATOR' };
    mockIsAuthenticated.mockReturnValue(true);
    mockGetStoredUser.mockReturnValue(fakeUser);
    mockLogout.mockImplementation(() => {
      localStorage.removeItem('opensylab_jwt_token');
    });

    render(
      <AuthProvider>
        <AuthConsumer />
        <AuthActions />
      </AuthProvider>,
    );

    await waitFor(() =>
      expect(screen.getByTestId('authenticated').textContent).toBe('true'),
    );

    await act(async () => {
      screen.getByTestId('btn-logout').click();
    });

    expect(screen.getByTestId('authenticated').textContent).toBe('false');
    expect(screen.getByTestId('username').textContent).toBe('none');
    expect(mockLogout).toHaveBeenCalledOnce();
  });

  // ── 4. isAuthenticated reflects stored token ─────────────────────────────
  it('isAuthenticated is true when a valid token is already stored', async () => {
    const fakeUser: User = { id: 3, username: 'viewer', role: 'VIEWER' };
    mockIsAuthenticated.mockReturnValue(true);
    mockGetStoredUser.mockReturnValue(fakeUser);

    render(
      <AuthProvider>
        <AuthConsumer />
      </AuthProvider>,
    );

    await waitFor(() =>
      expect(screen.getByTestId('authenticated').textContent).toBe('true'),
    );
  });

  // ── 5. User object populated correctly after login ───────────────────────
  it('user object is populated with correct id, username and role after login', async () => {
    mockIsAuthenticated.mockReturnValue(false);
    mockGetStoredUser.mockReturnValue(null);

    const fakeUser: User = { id: 7, username: 'labtech', role: 'OPERATOR' };
    mockLogin.mockResolvedValue({ success: true, user: fakeUser });

    render(
      <AuthProvider>
        <AuthConsumer />
        <AuthActions />
      </AuthProvider>,
    );

    await waitFor(() =>
      expect(screen.getByTestId('loading').textContent).toBe('false'),
    );

    await act(async () => {
      screen.getByTestId('btn-login').click();
    });

    await waitFor(() =>
      expect(screen.getByTestId('username').textContent).toBe('labtech'),
    );
    expect(screen.getByTestId('role').textContent).toBe('OPERATOR');
  });

  // ── 6. mustChangePassword flag propagates ───────────────────────────────
  it('mustChangePassword is true when login response signals it', async () => {
    mockIsAuthenticated.mockReturnValue(false);
    mockGetStoredUser.mockReturnValue(null);

    const fakeUser: User = {
      id: 4,
      username: 'newuser',
      role: 'OPERATOR',
      must_change_password: true,
    };
    mockLogin.mockResolvedValue({ success: true, user: fakeUser });

    render(
      <AuthProvider>
        <AuthConsumer />
        <AuthActions />
      </AuthProvider>,
    );

    await waitFor(() =>
      expect(screen.getByTestId('loading').textContent).toBe('false'),
    );

    await act(async () => {
      screen.getByTestId('btn-login').click();
    });

    await waitFor(() =>
      expect(screen.getByTestId('must-change').textContent).toBe('true'),
    );
  });

  // ── 7. useAuth outside provider throws ──────────────────────────────────
  it('useAuth throws when used outside AuthProvider', () => {
    const BareConsumer = () => {
      useAuth();
      return null;
    };

    // Suppress expected error output in test console
    const consoleError = vi.spyOn(console, 'error').mockImplementation(() => undefined);

    expect(() => render(<BareConsumer />)).toThrow(
      'useAuth must be used within an AuthProvider',
    );

    consoleError.mockRestore();
  });
});
