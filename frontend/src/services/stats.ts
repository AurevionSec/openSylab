import axios from 'axios';
import type { DashboardStats, StatsFilter } from '../types/stats';
import { API_BASE_URL } from '../utils/constants';

const getAuthHeaders = () => {
  const token = localStorage.getItem('authToken');
  return {
    'Authorization': `Bearer ${token}`,
    'Content-Type': 'application/json',
  };
};

export const getDashboardStats = async (filter?: StatsFilter): Promise<DashboardStats> => {
  const params = new URLSearchParams();
  
  if (filter) {
    if (filter.from) params.append('from', filter.from.toString());
    if (filter.to) params.append('to', filter.to.toString());
  }

  const queryString = params.toString();
  const url = queryString ? `${API_BASE_URL}/stats?${queryString}` : `${API_BASE_URL}/stats`;

  const response = await axios.get(url, {
    headers: getAuthHeaders(),
  });
  
  return response.data;
};
