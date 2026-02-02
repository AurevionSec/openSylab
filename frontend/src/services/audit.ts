import axios from 'axios';
import type { AuditEntry, AuditLogFilter } from '../types/audit';
import { API_BASE_URL } from '../utils/constants';

const getAuthHeaders = () => {
  const token = localStorage.getItem('authToken');
  return {
    'Authorization': `Bearer ${token}`,
    'Content-Type': 'application/json',
  };
};

export const getAuditLog = async (filter?: AuditLogFilter): Promise<AuditEntry[]> => {
  const params = new URLSearchParams();
  
  if (filter) {
    if (filter.user) params.append('user', filter.user);
    if (filter.action) params.append('action', filter.action);
    if (filter.entity) params.append('entity', filter.entity);
    if (filter.from) params.append('from', filter.from.toString());
    if (filter.to) params.append('to', filter.to.toString());
    if (filter.limit) params.append('limit', filter.limit.toString());
  }

  const queryString = params.toString();
  const url = queryString ? `${API_BASE_URL}/audit?${queryString}` : `${API_BASE_URL}/audit`;

  const response = await axios.get(url, {
    headers: getAuthHeaders(),
  });
  
  return response.data.data;
};
