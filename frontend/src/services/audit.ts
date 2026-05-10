import api from './api';
import type { AuditEntry, AuditLogFilter } from '../types/audit';

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
  const url = queryString ? `/audit?${queryString}` : '/audit';

  const response = await api.get<{ data: AuditEntry[] }>(url);

  return response.data.data ?? [];
};
