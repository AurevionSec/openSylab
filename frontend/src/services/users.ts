import api from './api';
import type { User, CreateUserPayload, UpdateUserPayload, ChangePasswordPayload } from '../types/user';
import { withNormalizedRole } from '../utils/roles';

export const getUsers = async (): Promise<User[]> => {
  const response = await api.get('/users');
  return (response.data.data as User[]).map(withNormalizedRole);
};

export const getCurrentUser = async (): Promise<User> => {
  const response = await api.get('/users/me');
  return withNormalizedRole(response.data.data as User);
};

export const createUser = async (payload: CreateUserPayload): Promise<User> => {
  const response = await api.post('/users', payload);
  return withNormalizedRole(response.data.data as User);
};

export const updateUser = async (userId: number, payload: UpdateUserPayload): Promise<User> => {
  const response = await api.put(`/users/${userId}`, payload);
  return withNormalizedRole(response.data.data as User);
};

export const deleteUser = async (userId: number): Promise<void> => {
  await api.delete(`/users/${userId}`);
};

export const changePassword = async (payload: ChangePasswordPayload): Promise<void> => {
  await api.put('/users/me/password', payload);
};
