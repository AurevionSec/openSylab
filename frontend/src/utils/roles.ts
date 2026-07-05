import type { UserRole } from '../types/user';

// The backend persists role names in German legacy form ("Administrator",
// "Betrachter", ...) as well as canonical codes. The whole frontend keys off the
// canonical UserRole codes (badges, colors, the edit dropdown), so every role
// coming from the API must pass through here — not just the login response.
const ROLE_MAP: Record<string, UserRole> = {
  Administrator: 'ADMIN', admin: 'ADMIN', ADMIN: 'ADMIN',
  Operator: 'OPERATOR', operator: 'OPERATOR', OPERATOR: 'OPERATOR',
  Betrachter: 'VIEWER', viewer: 'VIEWER', VIEWER: 'VIEWER', Unbekannt: 'VIEWER',
  Custom: 'CUSTOM', custom: 'CUSTOM', CUSTOM: 'CUSTOM', Benutzerdefiniert: 'CUSTOM',
};

export const normalizeRole = (raw: string | null | undefined): UserRole => {
  if (!raw) return 'VIEWER';
  const mapped = ROLE_MAP[raw];
  if (mapped) return mapped;
  const upper = raw.toUpperCase();
  return ROLE_MAP[upper] ?? 'VIEWER';
};

// Return a copy of a user-like object with its role normalized to a UserRole.
export const withNormalizedRole = <T extends { role: string }>(
  user: T,
): T & { role: UserRole } => ({ ...user, role: normalizeRole(user.role) });
