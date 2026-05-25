export const SAMPLE_STATUSES = {
  REGISTERED: 'Registered',
  IN_ANALYSIS: 'In Analysis',
  ANALYZED: 'Analyzed',
  VALIDATED: 'Validated',
  ARCHIVED: 'Archived',
} as const;

export const STATUS_COLORS = {
  REGISTERED: 'bg-blue-100 text-blue-800',
  IN_ANALYSIS: 'bg-yellow-100 text-yellow-800',
  ANALYZED: 'bg-purple-100 text-purple-800',
  VALIDATED: 'bg-green-100 text-green-800',
  ARCHIVED: 'bg-gray-100 text-gray-800',
} as const;

export const ORDER_STATUSES = {
  REQUESTED: 'Requested',
  IN_PROGRESS: 'In Progress',
  COMPLETED: 'Completed',
  VALIDATED: 'Validated',
  CANCELLED: 'Cancelled',
} as const;

export const ORDER_STATUS_COLORS = {
  REQUESTED: 'bg-blue-100 text-blue-800',
  IN_PROGRESS: 'bg-yellow-100 text-yellow-800',
  COMPLETED: 'bg-purple-100 text-purple-800',
  VALIDATED: 'bg-green-100 text-green-800',
  CANCELLED: 'bg-red-100 text-red-800',
} as const;

export const ORDER_PRIORITIES = {
  NORMAL: 'Normal',
  URGENT: 'Urgent',
  EMERGENCY: 'Emergency',
} as const;

export const ORDER_PRIORITY_COLORS = {
  NORMAL: 'bg-gray-100 text-gray-800',
  URGENT: 'bg-orange-100 text-orange-800',
  EMERGENCY: 'bg-red-100 text-red-800',
} as const;

export const RESULT_STATUSES = {
  PENDING: 'Pending',
  ENTERED: 'Entered',
  VALIDATED: 'Validated',
  REJECTED: 'Rejected',
  REPEATED: 'Repeated',
} as const;

export const RESULT_STATUS_COLORS = {
  PENDING: 'bg-gray-100 text-gray-800',
  ENTERED: 'bg-blue-100 text-blue-800',
  VALIDATED: 'bg-green-100 text-green-800',
  REJECTED: 'bg-red-100 text-red-800',
  REPEATED: 'bg-yellow-100 text-yellow-800',
} as const;

export const RESULT_FLAGS = {
  NORMAL: 'Normal',
  LOW: 'Low',
  HIGH: 'High',
  CRITICAL: 'Critical',
  UNDEFINED: 'Undefined',
} as const;

export const RESULT_FLAG_COLORS = {
  NORMAL: 'bg-green-100 text-green-800',
  LOW: 'bg-blue-100 text-blue-800',
  HIGH: 'bg-orange-100 text-orange-800',
  CRITICAL: 'bg-red-100 text-red-800',
  UNDEFINED: 'bg-gray-100 text-gray-600',
} as const;

// API Configuration
export const API_BASE_URL = import.meta.env.VITE_API_URL || 'http://localhost:8080/api/v1';

// Authentication storage keys
export const API_KEY_STORAGE_KEY = 'opensylab_api_key'; // Legacy, kept for backward compatibility
export const JWT_TOKEN_STORAGE_KEY = 'opensylab_jwt_token';
export const USER_INFO_STORAGE_KEY = 'opensylab_user_info';

export const SAMPLE_TRANSITIONS: Record<string, string[]> = {
  REGISTERED: ['IN_ANALYSIS', 'ARCHIVED'],
  IN_ANALYSIS: ['ANALYZED', 'ARCHIVED'],
  ANALYZED: ['VALIDATED', 'ARCHIVED'],
  VALIDATED: ['ARCHIVED'],
  ARCHIVED: [],
};

export const ORDER_TRANSITIONS: Record<string, string[]> = {
  REQUESTED: ['IN_PROGRESS', 'CANCELLED'],
  IN_PROGRESS: ['COMPLETED', 'CANCELLED'],
  COMPLETED: ['VALIDATED'],
  VALIDATED: [],
  CANCELLED: [],
};

export const RESULT_TRANSITIONS: Record<string, string[]> = {
  PENDING:   ['ENTERED', 'REJECTED'],
  ENTERED:   ['VALIDATED', 'REJECTED', 'REPEATED'],
  REPEATED:  ['ENTERED', 'VALIDATED', 'REJECTED'],
  VALIDATED: [],
  REJECTED:  [],
};
