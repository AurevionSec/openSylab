import { useCallback, useEffect, useRef, useState } from 'react';
import { useSearchParams } from 'react-router-dom';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { StatusBadge } from '../components/common/StatusBadge';
import { Input } from '../components/common/Input';
import { Button } from '../components/common/Button';
import { getCurrentUser, changePassword } from '../services/users';
import type { User, ChangePasswordPayload } from '../types/user';
import { USER_ROLES, ROLE_COLORS } from '../types/user';
import { useTheme } from '../context/ThemeContext';
import { useAuth } from '../context/AuthContext';
import { useDocumentTitle } from '../hooks/useDocumentTitle';

export const Profile = () => {
  useDocumentTitle({ action: 'My Profile' });
  const { isDarkMode, toggleDarkMode } = useTheme();
  const { clearMustChangePassword } = useAuth();
  const [searchParams] = useSearchParams();
  const mountedRef = useRef(true);
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [isChangingPassword, setIsChangingPassword] = useState(false);
  const isForceChange = searchParams.get('force_change') === '1';

  useEffect(() => {
    mountedRef.current = true;
    return () => { mountedRef.current = false; };
  }, []);

  const fetchProfile = useCallback(async () => {
    try {
      setLoading(true);
      const data = await getCurrentUser();
      if (mountedRef.current) setUser(data);
      if (mountedRef.current && data.must_change_password) setIsChangingPassword(true);
    } catch (err: unknown) {
      if (mountedRef.current) setError((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to load profile');
      console.error(err);
    } finally {
      if (mountedRef.current) setLoading(false);
    }
  }, []);

  useEffect(() => {
    if (isForceChange) {
      setIsChangingPassword(true);
    }
    fetchProfile();
  }, [isForceChange, fetchProfile]);

  if (loading) {
    return (
      <Layout>
        <div className="flex items-center justify-center h-64">
          <div className="text-center">
            <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
            <p className="mt-4 text-gray-600">Loading profile...</p>
          </div>
        </div>
      </Layout>
    );
  }

  if (error || !user) {
    return (
      <Layout>
        <div className="bg-red-50 border border-red-200 rounded p-4">
          <p className="text-red-800">{error || 'Failed to load profile'}</p>
        </div>
      </Layout>
    );
  }

  return (
    <Layout>
      <div className="space-y-6">
        <div>
          <h2 className="text-3xl font-bold text-gray-900">My Profile</h2>
          <p className="text-gray-600 mt-1">View and manage your account settings</p>
        </div>

        {isForceChange && (
          <div className="bg-yellow-50 border border-yellow-200 rounded-lg p-4">
            <p className="text-yellow-800 font-medium">
              Sie verwenden noch das Standard-Passwort. Bitte ändern Sie es jetzt.
            </p>
          </div>
        )}

        {/* Profile Information */}
        <Card title="Profile Information">
          <div className="space-y-4">
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Username
                </label>
                <p className="text-gray-900 font-mono bg-gray-50 px-3 py-2 rounded">
                  {user.username}
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Role
                </label>
                <div>
                  <StatusBadge colorClass={ROLE_COLORS[user.role]}>
                    {USER_ROLES[user.role]}
                  </StatusBadge>
                </div>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Full Name
                </label>
                <p className="text-gray-900 bg-gray-50 px-3 py-2 rounded">
                  {user.full_name || '-'}
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Email
                </label>
                <p className="text-gray-900 bg-gray-50 px-3 py-2 rounded">
                  {user.email || '-'}
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Account Status
                </label>
                <div>
                  <StatusBadge colorClass={user.active ? 'bg-green-100 text-green-800 border-green-200' : 'bg-gray-100 text-gray-800 border-gray-300'}>
                    {user.active ? 'Active' : 'Inactive'}
                  </StatusBadge>
                </div>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Last Login
                </label>
                <p className="text-gray-900 bg-gray-50 px-3 py-2 rounded">
                  {user.last_login ? new Date(user.last_login * 1000).toLocaleString() : 'Never'}
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Account Created
                </label>
                <p className="text-gray-900 bg-gray-50 px-3 py-2 rounded">
                  {new Date(user.created_at * 1000).toLocaleDateString()}
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  User ID
                </label>
                <p className="text-gray-900 font-mono bg-gray-50 px-3 py-2 rounded">
                  #{user.id}
                </p>
              </div>
            </div>
          </div>
        </Card>

        {/* Appearance Settings */}
        <Card title="Appearance">
          <div className="space-y-4">
            <div className="flex items-center justify-between">
              <div className="flex-1">
                <h3 className="text-lg font-semibold text-gray-900">Dark Mode</h3>
                <p className="text-sm text-gray-600 mt-1">
                  Switch to dark theme for reduced eye strain during extended use. Optimized for low-light environments.
                </p>
              </div>

              <button
                type="button"
                onClick={toggleDarkMode}
                className={`relative inline-flex h-8 w-14 flex-shrink-0 cursor-pointer items-center rounded-full border-2 transition-colors duration-200 ease-in-out focus:outline-none focus:ring-2 focus:ring-offset-2 ${
                  isDarkMode
                    ? 'bg-blue-600 border-blue-600 focus:ring-blue-600'
                    : 'bg-gray-200 border-gray-300 focus:ring-blue-600'
                }`}
                role="switch"
                aria-checked={isDarkMode}
                aria-label="Toggle dark mode"
              >
                <span
                  className={`inline-block h-6 w-6 transform rounded-full bg-white transition duration-200 ease-in-out ${
                    isDarkMode
                      ? 'translate-x-7'
                      : 'translate-x-1'
                  }`}
                />
              </button>
            </div>
          </div>
        </Card>

        {/* Change Password */}
        <Card title="Security">
          <div className="space-y-4">
            <p className="text-sm text-gray-600">
              Update your password to keep your account secure. Use a strong password with a mix of letters, numbers, and symbols.
            </p>

            {!isChangingPassword ? (
              <Button onClick={() => setIsChangingPassword(true)}>
                Change Password
              </Button>
            ) : (
              <ChangePasswordForm
                onSuccess={() => {
                  setIsChangingPassword(false);
                  clearMustChangePassword();
                  fetchProfile();
                }}
                onCancel={() => setIsChangingPassword(false)}
                forceChange={isForceChange}
              />
            )}
          </div>
        </Card>
      </div>
    </Layout>
  );
};

interface ChangePasswordFormProps {
  onSuccess: () => void;
  onCancel: () => void;
  forceChange?: boolean;
}

const ChangePasswordForm = ({ onSuccess, onCancel, forceChange }: ChangePasswordFormProps) => {
  const [formData, setFormData] = useState<ChangePasswordPayload>({
    current_password: '',
    new_password: '',
  });
  const [confirmPassword, setConfirmPassword] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [success, setSuccess] = useState(false);
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  useEffect(() => () => { if (timerRef.current) clearTimeout(timerRef.current); }, []);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess(false);
    setLoading(true);

    if (formData.new_password !== confirmPassword) {
      setError('New passwords do not match');
      setLoading(false);
      return;
    }

    if (formData.new_password.length < 8) {
      setError('Password must be at least 8 characters long');
      setLoading(false);
      return;
    }

    try {
      await changePassword(formData);
      setSuccess(true);
      timerRef.current = setTimeout(() => onSuccess(), 1500);
    } catch (err: unknown) {
      setError((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to change password');
    } finally {
      setLoading(false);
    }
  };

  return (
    <form onSubmit={handleSubmit} className="space-y-4 bg-gray-50 p-4 rounded">
      {error && (
        <div className="bg-red-50 border border-red-200 rounded p-4">
          <p className="text-red-800 text-sm">{error}</p>
        </div>
      )}

      {success && (
        <div className="bg-green-50 border border-green-200 rounded p-4">
          <p className="text-green-800 text-sm">Password changed successfully!</p>
        </div>
      )}

      <Input
        type="password"
        label="Current Password *"
        value={formData.current_password}
        onChange={(e) => setFormData({ ...formData, current_password: e.target.value })}
        required
        disabled={loading || success}
      />

      <Input
        type="password"
        label="New Password *"
        value={formData.new_password}
        onChange={(e) => setFormData({ ...formData, new_password: e.target.value })}
        required
        disabled={loading || success}
        placeholder="Minimum 8 characters"
      />
      {formData.new_password && (() => {
        const pw = formData.new_password;
        const score = [pw.length >= 8, /[A-Z]/.test(pw), /[a-z]/.test(pw), /[0-9]/.test(pw), /[^A-Za-z0-9]/.test(pw)].filter(Boolean).length;
        const colors = ['bg-red-500','bg-orange-400','bg-yellow-400','bg-blue-500','bg-green-500'];
        const labels = ['Sehr schwach','Schwach','Mittel','Gut','Stark'];
        return (
          <div className="-mt-1 mb-1">
            <div className="flex gap-1 mb-0.5">
              {[0,1,2,3,4].map(i => <div key={i} className={`h-1 flex-1 rounded-full ${i < score ? colors[score-1] : 'bg-gray-200'}`} />)}
            </div>
            <p className="text-xs text-gray-500">{labels[score-1] || 'Sehr schwach'}</p>
          </div>
        );
      })()}

      <Input
        type="password"
        label="Confirm New Password *"
        value={confirmPassword}
        onChange={(e) => setConfirmPassword(e.target.value)}
        required
        disabled={loading || success}
      />

      <div className="flex justify-end gap-3">
        {!forceChange && (
          <Button
            type="button"
            variant="secondary"
            onClick={onCancel}
            disabled={loading || success}
          >
            Cancel
          </Button>
        )}
        <Button
          type="submit"
          variant="primary"
          disabled={loading || success}
        >
          {loading ? 'Changing...' : 'Change Password'}
        </Button>
      </div>
    </form>
  );
};

export default Profile;
