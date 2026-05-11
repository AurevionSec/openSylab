import { useState } from 'react';
import type { FormEvent } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';

import { Input } from '../components/common/Input';
import { Button } from '../components/common/Button';
import { Card } from '../components/common/Card';
import { useDocumentTitle } from '../hooks/useDocumentTitle';

export const Login = () => {
  useDocumentTitle({ action: 'Sign In' });
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [mfaCode, setMfaCode] = useState('');
  const [mfaRequired, setMfaRequired] = useState(false);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const { login, mustChangePassword } = useAuth();
  const navigate = useNavigate();
  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    try {
      const result = await login(username, password, mfaRequired ? mfaCode : undefined);
      if (result.success) {
        if (mustChangePassword) {
          navigate('/profile?force_change=1');
        } else {
          navigate('/');
        }
      } else if (result.mfaRequired) {
        setMfaRequired(true);
        setError('');
      } else {
        setError(result.error || 'Invalid credentials. Please check your username and password and try again.');
      }
    } catch (err) {
      setError('An error occurred. Please try again.');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-blue-100 flex items-center justify-center px-4">
      <div className="max-w-md w-full">
        <div className="text-center mb-8">
          <h1 className="text-4xl font-bold text-blue-600 mb-2">OpenSylab</h1>
          <p className="text-gray-600">Laboratory Information Management System</p>
        </div>

        <Card>
          <form onSubmit={handleSubmit} className="space-y-6">
            <div>
              <h2 className="text-2xl font-semibold text-gray-900 mb-6">Sign In</h2>

              <div className="space-y-4">
                <Input
                  type="text"
                  label="Username"
                  placeholder="Enter your username"
                  value={username}
                  onChange={(e) => setUsername(e.target.value)}
                  required
                  autoFocus
                  autoComplete="username"
                  aria-label="Username"
                />

                <Input
                  type="password"
                  label="Password"
                  placeholder="Enter your password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  error={error}
                  required
                  autoComplete="current-password"
                  aria-label="Password"
                />
              </div>
            </div>

            {mfaRequired && (
              <div>
                <p className="text-sm text-blue-700 bg-blue-50 border border-blue-200 rounded p-3 mb-3">
                  An MFA code is required. Enter the 6-digit code from your authenticator app.
                </p>
                <Input
                  type="text"
                  label="MFA Code"
                  placeholder="6-digit code"
                  value={mfaCode}
                  onChange={(e) => setMfaCode(e.target.value)}
                  maxLength={6}
                  autoFocus
                  autoComplete="one-time-code"
                  aria-label="MFA Code"
                />
              </div>
            )}

            {error && (
              <p className="text-red-600 text-sm">{error}</p>
            )}

            <Button
              type="submit"
              className="w-full"
              disabled={loading || !username || !password || (mfaRequired && !mfaCode)}
            >
              {loading ? 'Signing in...' : mfaRequired ? 'Verify MFA Code' : 'Sign In'}
            </Button>
          </form>

          <div className="mt-6 pt-6 border-t border-gray-200">
            <p className="text-sm text-gray-500 text-center">
              Enter your credentials to access the OpenSylab LIMS system.
            </p>
          </div>
        </Card>
      </div>
    </div>
  );
};

export default Login;
