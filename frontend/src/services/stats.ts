import api from './api';
import type { DashboardStats, StatsFilter } from '../types/stats';

export const getDashboardStats = async (filter?: StatsFilter): Promise<DashboardStats> => {
  const params = new URLSearchParams();

  if (filter) {
    if (filter.from) params.append('from', filter.from.toString());
    if (filter.to) params.append('to', filter.to.toString());
  }

  const queryString = params.toString();
  const url = queryString ? `/stats?${queryString}` : '/stats';

  const response = await api.get(url);

  return response.data;
};
