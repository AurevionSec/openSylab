// Audit log type definitions
export type AuditAction = 
  | 'CREATE'
  | 'UPDATE'
  | 'DELETE'
  | 'LOGIN'
  | 'LOGOUT'
  | 'VALIDATE'
  | 'EXPORT'
  | 'ACCESS';

export type AuditEntity = 
  | 'SAMPLE'
  | 'ORDER'
  | 'RESULT'
  | 'USER'
  | 'ROLE'
  | 'SYSTEM';

export interface AuditEntry {
  id: number;
  action: AuditAction;
  entity: AuditEntity;
  entity_id: string;
  user: string;
  timestamp: number;
  details: string;
}

export interface AuditLogFilter {
  user?: string;
  action?: AuditAction;
  entity?: AuditEntity;
  from?: number;
  to?: number;
  limit?: number;
}

export const AUDIT_ACTIONS: Record<AuditAction, string> = {
  CREATE: 'Create',
  UPDATE: 'Update',
  DELETE: 'Delete',
  LOGIN: 'Login',
  LOGOUT: 'Logout',
  VALIDATE: 'Validate',
  EXPORT: 'Export',
  ACCESS: 'Access',
};

export const AUDIT_ENTITIES: Record<AuditEntity, string> = {
  SAMPLE: 'Sample',
  ORDER: 'Order',
  RESULT: 'Result',
  USER: 'User',
  ROLE: 'Role',
  SYSTEM: 'System',
};

export const ACTION_COLORS: Record<AuditAction, string> = {
  CREATE: 'bg-green-100 text-green-800',
  UPDATE: 'bg-blue-100 text-blue-800',
  DELETE: 'bg-red-100 text-red-800',
  LOGIN: 'bg-gray-100 text-gray-800',
  LOGOUT: 'bg-gray-100 text-gray-800',
  VALIDATE: 'bg-purple-100 text-purple-800',
  EXPORT: 'bg-yellow-100 text-yellow-800',
  ACCESS: 'bg-indigo-100 text-indigo-800',
};
