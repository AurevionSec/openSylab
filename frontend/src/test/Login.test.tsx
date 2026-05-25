import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import { Login } from '../pages/Login';
import { AuthProvider } from '../context/AuthContext';
import * as authService from '../services/auth';

// Mock auth service so no real HTTP calls happen
vi.mock('../services/auth', () => ({
  login: vi.fn(),
  logout: vi.fn(),
  isAuthenticated: vi.fn(),
  getStoredUser: vi.fn(),
}));

// react-router-dom navigate is provided by MemoryRouter

const renderLogin = () => {
  const mockIsAuthenticated = vi.mocked(authService.isAuthenticated);
  const mockGetStoredUser = vi.mocked(authService.getStoredUser);
  mockIsAuthenticated.mockReturnValue(false);
  mockGetStoredUser.mockReturnValue(null);

  return render(
    <MemoryRouter>
      <AuthProvider>
        <Login />
      </AuthProvider>
    </MemoryRouter>,
  );
};

describe('Login page', () => {
  const mockLogin = vi.mocked(authService.login);

  beforeEach(() => {
    localStorage.clear();
    vi.clearAllMocks();
  });

  // ── 1. Form renders ────────────────────────────────────────────────────
  it('renders username and password input fields', () => {
    renderLogin();

    expect(screen.getByLabelText(/username/i)).toBeInTheDocument();
    expect(screen.getByLabelText(/password/i)).toBeInTheDocument();
  });

  // ── 2. Submit button present ───────────────────────────────────────────
  it('renders a submit button', () => {
    renderLogin();

    const btn = screen.getByRole('button', { name: /sign in/i });
    expect(btn).toBeInTheDocument();
  });

  // ── 3. Submit button disabled on empty form ────────────────────────────
  it('submit button is disabled when username and password are empty', () => {
    renderLogin();

    const btn = screen.getByRole('button', { name: /sign in/i });
    expect(btn).toBeDisabled();
  });

  // ── 4. Submit button enabled when both fields filled ──────────────────
  it('submit button becomes enabled after filling in username and password', async () => {
    renderLogin();

    const user = userEvent.setup();
    await user.type(screen.getByLabelText(/username/i), 'admin');
    await user.type(screen.getByLabelText(/password/i), 'secret');

    expect(screen.getByRole('button', { name: /sign in/i })).not.toBeDisabled();
  });

  // ── 5. Error on failed login ───────────────────────────────────────────
  it('shows error message when login fails with invalid credentials', async () => {
    mockLogin.mockResolvedValue({
      success: false,
      error: 'Invalid username or password.',
    });

    renderLogin();

    const user = userEvent.setup();
    await user.type(screen.getByLabelText(/username/i), 'baduser');
    await user.type(screen.getByLabelText(/password/i), 'wrongpass');
    await user.click(screen.getByRole('button', { name: /sign in/i }));

    // The error appears both in the Input's inline error and in the form error <p>.
    // getAllByText handles the duplicate gracefully.
    await waitFor(() => {
      const errors = screen.getAllByText(/invalid username or password/i);
      expect(errors.length).toBeGreaterThan(0);
    });
  });

  // ── 6. Headings / branding ────────────────────────────────────────────
  it('shows the OpenSylab branding heading', () => {
    renderLogin();

    expect(screen.getByText('OpenSylab')).toBeInTheDocument();
  });

  // ── 7. MFA field hidden initially ─────────────────────────────────────
  it('MFA code input is not visible on initial render', () => {
    renderLogin();

    expect(screen.queryByLabelText(/mfa code/i)).not.toBeInTheDocument();
  });

  // ── 8. MFA field appears after mfa_required response ──────────────────
  it('shows MFA input after server signals mfa_required', async () => {
    mockLogin.mockResolvedValue({
      success: false,
      mfaRequired: true,
      error: 'MFA code required.',
    });

    renderLogin();

    const user = userEvent.setup();
    await user.type(screen.getByLabelText(/username/i), 'admin');
    await user.type(screen.getByLabelText(/password/i), 'correct-pass');
    await user.click(screen.getByRole('button', { name: /sign in/i }));

    await waitFor(() =>
      expect(screen.getByLabelText(/mfa code/i)).toBeInTheDocument(),
    );
  });

  // ── 9. Loading state during submission ────────────────────────────────
  it('shows "Signing in…" text while the login request is in flight', async () => {
    // Never resolves during this test
    mockLogin.mockImplementation(
      () => new Promise<{ success: boolean }>(() => undefined),
    );

    renderLogin();

    const user = userEvent.setup();
    await user.type(screen.getByLabelText(/username/i), 'admin');
    await user.type(screen.getByLabelText(/password/i), 'secret');

    fireEvent.click(screen.getByRole('button', { name: /sign in/i }));

    await waitFor(() =>
      expect(screen.getByRole('button', { name: /signing in/i })).toBeInTheDocument(),
    );
  });
});
