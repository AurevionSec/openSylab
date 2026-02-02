import axios from 'axios';
import type { User, CreateUserPayload, UpdateUserPayload, ChangePasswordPayload } from '../types/user';
import { API_BASE_URL } from '../utils/constants';

const getAuthHeaders = () => {
  const token = localStorage.getItem('authToken');
  return {
    'Authorization': `Bearer ${token}`,
    'Content-Type': 'application/json',
  };
};

export const getUsers = async (): Promise<User[]> => {
  const response = await axios.get(`${API_BASE_URL}/users`, {
    headers: getAuthHeaders(),
  });
  return response.data.data;
};

export const getCurrentUser = async (): Promise<User> => {
  const response = await axios.get(`${API_BASE_URL}/users/me`, {
    headers: getAuthHeaders(),
  });
  return response.data.data;
};

export const createUser = async (payload: CreateUserPayload): Promise<User> => {
  const response = await axios.post(`${API_BASE_URL}/users`, payload, {
    headers: getAuthHeaders(),
  });
  return response.data.data;
};

export const updateUser = async (userId: number, payload: UpdateUserPayload): Promise<User> => {
  const response = await axios.put(`${API_BASE_URL}/users/${userId}`, payload, {
    headers: getAuthHeaders(),
  });
  return response.data.data;
};

export const deleteUser = async (userId: number): Promise<void> => {
  await axios.delete(`${API_BASE_URL}/users/${userId}`, {
    headers: getAuthHeaders(),
  });
};

export const changePassword = async (payload: ChangePasswordPayload): Promise<void> => {
  await axios.put(`${API_BASE_URL}/users/me/password`, payload, {
    headers: getAuthHeaders(),
  });
};
