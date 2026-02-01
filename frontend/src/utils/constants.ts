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

// Authentication storage keys
export const API_KEY_STORAGE_KEY = 'opensylab_api_key'; // Legacy, kept for backward compatibility
export const JWT_TOKEN_STORAGE_KEY = 'opensylab_jwt_token';
export const USER_INFO_STORAGE_KEY = 'opensylab_user_info';
