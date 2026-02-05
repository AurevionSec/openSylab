import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Input } from '../components/common/Input';
import { Button } from '../components/common/Button';
import { getCurrentUser, changePassword } from '../services/users';
import type { User, ChangePasswordPayload } from '../types/user';
import { USER_ROLES, ROLE_COLORS } from '../types/user';

export const Profile = () => {
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [isChangingPassword, setIsChangingPassword] = useState(false);

  useEffect(() => {
    fetchProfile();
  }, []);

  const fetchProfile = async () => {
    try {
      setLoading(true);
      const data = await getCurrentUser();
      setUser(data);
    } catch (err: any) {
      setError(err.response?.data?.error?.message || 'Failed to load profile');
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

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
                  <span className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${ROLE_COLORS[user.role]}`}>
                    {USER_ROLES[user.role]}
                  </span>
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
                  <span className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${user.active ? 'bg-green-100 text-green-800' : 'bg-gray-100 text-gray-800'}`}>
                    {user.active ? 'Active' : 'Inactive'}
                  </span>
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
                }}
                onCancel={() => setIsChangingPassword(false)}
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
}

const ChangePasswordForm = ({ onSuccess, onCancel }: ChangePasswordFormProps) => {
  const [formData, setFormData] = useState<ChangePasswordPayload>({
    current_password: '',
    new_password: '',
  });
  const [confirmPassword, setConfirmPassword] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [success, setSuccess] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setSuccess(false);

    if (formData.new_password !== confirmPassword) {
      setError('New passwords do not match');
      return;
    }

    if (formData.new_password.length < 8) {
      setError('Password must be at least 8 characters long');
      return;
    }

    setLoading(true);

    try {
      await changePassword(formData);
      setSuccess(true);
      setTimeout(() => {
        onSuccess();
      }, 2000);
    } catch (err: any) {
      setError(err.response?.data?.error?.message || 'Failed to change password');
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

      <Input
        type="password"
        label="Confirm New Password *"
        value={confirmPassword}
        onChange={(e) => setConfirmPassword(e.target.value)}
        required
        disabled={loading || success}
      />

      <div className="flex justify-end gap-3">
        <Button
          type="button"
          variant="secondary"
          onClick={onCancel}
          disabled={loading || success}
        >
          Cancel
        </Button>
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
