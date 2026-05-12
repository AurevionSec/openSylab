import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { Input } from '../components/common/Input';
import { getUsers, createUser, updateUser, deleteUser } from '../services/users';
import type { User, CreateUserPayload, UpdateUserPayload, UserRole } from '../types/user';
import { USER_ROLES, ROLE_COLORS } from '../types/user';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { ErrorBanner } from '../components/common/ErrorBanner';

export const Users = () => {
  useDocumentTitle({ module: 'User Management' });
  const [users, setUsers] = useState<User[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [editingUser, setEditingUser] = useState<User | null>(null);

  useEffect(() => {
    let cancelled = false;
    const load = async () => {
      try {
        setLoading(true);
        const data = await getUsers();
        if (!cancelled) setUsers(data);
      } catch (err: unknown) {
        if (!cancelled) setError((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to load users');
        console.error(err);
      } finally {
        if (!cancelled) setLoading(false);
      }
    };
    load();
    return () => { cancelled = true; };
  }, []);

  const fetchUsers = async () => {
    try {
      setLoading(true);
      const data = await getUsers();
      setUsers(data);
    } catch (err: unknown) {
      setError((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to load users');
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  const handleCreateUser = async (payload: CreateUserPayload | UpdateUserPayload) => {
    try {
      await createUser(payload as CreateUserPayload);
      await fetchUsers();
      setIsCreateModalOpen(false);
    } catch (err: unknown) {
      throw new Error((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to create user');
    }
  };

  const handleUpdateUser = async (userId: number, payload: CreateUserPayload | UpdateUserPayload) => {
    try {
      await updateUser(userId, payload as UpdateUserPayload);
      await fetchUsers();
      setEditingUser(null);
    } catch (err: unknown) {
      throw new Error((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to update user');
    }
  };

  const handleDeleteUser = async (userId: number) => {
    if (!confirm('Are you sure you want to delete this user?')) return;
    
    try {
      await deleteUser(userId);
      await fetchUsers();
    } catch (err: unknown) {
      setError((err && typeof err === 'object' && 'response' in err ? (err as {response?: {data?: {error?: {message?: string}}}}).response?.data?.error?.message : undefined) || 'Failed to delete user');
    }
  };

  if (loading) {
    return (
      <Layout>
        <div className="flex items-center justify-center h-64">
          <div className="text-center">
            <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
            <p className="mt-4 text-gray-600">Loading users...</p>
          </div>
        </div>
      </Layout>
    );
  }

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex justify-between items-center">
          <div>
            <h2 className="text-3xl font-bold text-gray-900">User Management</h2>
            <p className="text-gray-600 mt-1">Manage system users and their roles</p>
          </div>
          <Button onClick={() => setIsCreateModalOpen(true)}>
            + Create User
          </Button>
        </div>

        <ErrorBanner message={error || null} />

        <Card title={`Users (${users.length})`}>
          <div className="overflow-x-auto">
            <table className="min-w-full divide-y divide-gray-200">
              <thead>
                <tr>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Username
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Full Name
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Email
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Role
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Status
                  </th>
                  <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Last Login
                  </th>
                  <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider">
                    Actions
                  </th>
                </tr>
              </thead>
              <tbody className="bg-white divide-y divide-gray-200">
                {users.map((user) => (
                  <tr key={user.id} className="hover:bg-gray-50">
                    <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                      {user.username}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                      {user.full_name || '-'}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                      {user.email || '-'}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap">
                      <span className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${ROLE_COLORS[user.role]}`}>
                        {USER_ROLES[user.role]}
                      </span>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap">
                      <span className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${user.active ? 'bg-green-100 text-green-800' : 'bg-gray-100 text-gray-800'}`}>
                        {user.active ? 'Active' : 'Inactive'}
                      </span>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                      {user.last_login ? new Date(user.last_login * 1000).toLocaleDateString() : 'Never'}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-right text-sm font-medium space-x-2">
                      <button
                        onClick={() => setEditingUser(user)}
                        className="text-blue-600 hover:text-blue-900"
                      >
                        Edit
                      </button>
                      <button
                        onClick={() => handleDeleteUser(user.id)}
                        className="text-red-600 hover:text-red-900"
                      >
                        Delete
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Card>
      </div>

      {isCreateModalOpen && (
        <UserModal
          onClose={() => setIsCreateModalOpen(false)}
          onSubmit={handleCreateUser}
        />
      )}

      {editingUser && (
        <UserModal
          user={editingUser}
          onClose={() => setEditingUser(null)}
          onSubmit={(payload) => handleUpdateUser(editingUser.id, payload)}
        />
      )}
    </Layout>
  );
};

interface UserModalProps {
  user?: User;
  onClose: () => void;
  onSubmit: (payload: CreateUserPayload | UpdateUserPayload) => Promise<void>;
}

const UserModal = ({ user, onClose, onSubmit }: UserModalProps) => {
  const [formData, setFormData] = useState({
    username: user?.username || '',
    password: '',
    role: (user?.role || 'OPERATOR') as UserRole,
    full_name: user?.full_name || '',
    email: user?.email || '',
    active: user?.active ?? true,
  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    try {
      const basePayload: UpdateUserPayload = {
        username: formData.username,
        role: formData.role as UserRole,
        full_name: formData.full_name,
        email: formData.email,
        active: formData.active,
      };

      if (formData.password) {
        basePayload.password = formData.password;
      } else if (!user) {
        setError('Password is required for new users');
        setLoading(false);
        return;
      }

      await onSubmit(user ? basePayload : (basePayload as CreateUserPayload));
    } catch (err: unknown) {
      setError((err instanceof Error ? err.message : String(err)));
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
      <div className="bg-white rounded shadow-xl max-w-2xl w-full max-h-[90vh] overflow-y-auto">
        <div className="p-6">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-2xl font-bold text-gray-900">
              {user ? 'Edit User' : 'Create User'}
            </h2>
            <button
              onClick={onClose}
              className="text-gray-400 hover:text-gray-600 transition-colors"
            >
              <svg className="w-6 h-6" fill="none" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" viewBox="0 0 24 24" stroke="currentColor">
                <path d="M6 18L18 6M6 6l12 12"></path>
              </svg>
            </button>
          </div>

          <form onSubmit={handleSubmit} className="space-y-4">
            <ErrorBanner message={error || null} />

            <Input
              type="text"
              label="Username *"
              value={formData.username}
              onChange={(e) => setFormData({ ...formData, username: e.target.value })}
              required
              disabled={!!user}
            />

            <Input
              type="password"
              label={user ? "Password (leave empty to keep current)" : "Password *"}
              value={formData.password}
              onChange={(e) => setFormData({ ...formData, password: e.target.value })}
              required={!user}
            />

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                Role *
              </label>
              <select
                value={formData.role}
                onChange={(e) => setFormData({ ...formData, role: e.target.value as UserRole })}
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
                required
              >
                {Object.entries(USER_ROLES).map(([value, label]) => (
                  <option key={value} value={value}>{label}</option>
                ))}
              </select>
            </div>

            <Input
              type="text"
              label="Full Name"
              value={formData.full_name}
              onChange={(e) => setFormData({ ...formData, full_name: e.target.value })}
            />

            <Input
              type="email"
              label="Email"
              value={formData.email}
              onChange={(e) => setFormData({ ...formData, email: e.target.value })}
            />

            <div className="flex items-center">
              <input
                type="checkbox"
                id="active"
                checked={formData.active}
                onChange={(e) => setFormData({ ...formData, active: e.target.checked })}
                className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
              />
              <label htmlFor="active" className="ml-2 block text-sm text-gray-900">
                Active
              </label>
            </div>

            <div className="flex justify-end gap-3 pt-4 border-t border-gray-200">
              <Button
                type="button"
                variant="secondary"
                onClick={onClose}
                disabled={loading}
              >
                Cancel
              </Button>
              <Button
                type="submit"
                variant="primary"
                disabled={loading}
              >
                {loading ? 'Saving...' : user ? 'Update User' : 'Create User'}
              </Button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default Users;
