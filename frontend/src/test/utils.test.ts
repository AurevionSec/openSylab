import { describe, it, expect } from 'vitest';
import {
  SAMPLE_STATUSES,
  STATUS_COLORS,
  ORDER_STATUSES,
  ORDER_STATUS_COLORS,
  ORDER_PRIORITIES,
  ORDER_PRIORITY_COLORS,
  RESULT_STATUSES,
  RESULT_STATUS_COLORS,
  RESULT_FLAGS,
  RESULT_FLAG_COLORS,
  JWT_TOKEN_STORAGE_KEY,
  API_KEY_STORAGE_KEY,
  USER_INFO_STORAGE_KEY,
  SAMPLE_TRANSITIONS,
  ORDER_TRANSITIONS,
  RESULT_TRANSITIONS,
} from '../utils/constants';

// ── Storage keys ──────────────────────────────────────────────────────────────
describe('constants — storage keys', () => {
  it('JWT_TOKEN_STORAGE_KEY is a non-empty string', () => {
    expect(typeof JWT_TOKEN_STORAGE_KEY).toBe('string');
    expect(JWT_TOKEN_STORAGE_KEY.length).toBeGreaterThan(0);
  });

  it('API_KEY_STORAGE_KEY is a non-empty string', () => {
    expect(typeof API_KEY_STORAGE_KEY).toBe('string');
    expect(API_KEY_STORAGE_KEY.length).toBeGreaterThan(0);
  });

  it('USER_INFO_STORAGE_KEY is a non-empty string', () => {
    expect(typeof USER_INFO_STORAGE_KEY).toBe('string');
    expect(USER_INFO_STORAGE_KEY.length).toBeGreaterThan(0);
  });

  it('all three storage keys are distinct', () => {
    const keys = new Set([
      JWT_TOKEN_STORAGE_KEY,
      API_KEY_STORAGE_KEY,
      USER_INFO_STORAGE_KEY,
    ]);
    expect(keys.size).toBe(3);
  });
});

// ── Sample status constants ───────────────────────────────────────────────────
describe('constants — SAMPLE_STATUSES', () => {
  it('contains all expected status keys', () => {
    expect(Object.keys(SAMPLE_STATUSES)).toEqual(
      expect.arrayContaining([
        'REGISTERED',
        'IN_ANALYSIS',
        'ANALYZED',
        'VALIDATED',
        'ARCHIVED',
      ]),
    );
  });

  it('every status key in STATUS_COLORS maps to a non-empty CSS class string', () => {
    for (const key of Object.keys(SAMPLE_STATUSES) as Array<
      keyof typeof STATUS_COLORS
    >) {
      expect(STATUS_COLORS[key]).toBeTruthy();
      expect(typeof STATUS_COLORS[key]).toBe('string');
    }
  });
});

// ── Order status constants ────────────────────────────────────────────────────
describe('constants — ORDER_STATUSES', () => {
  it('contains REQUESTED, IN_PROGRESS, COMPLETED, VALIDATED, CANCELLED', () => {
    expect(Object.keys(ORDER_STATUSES)).toEqual(
      expect.arrayContaining([
        'REQUESTED',
        'IN_PROGRESS',
        'COMPLETED',
        'VALIDATED',
        'CANCELLED',
      ]),
    );
  });

  it('every ORDER_STATUS_COLORS entry is a non-empty string', () => {
    for (const key of Object.keys(ORDER_STATUSES) as Array<
      keyof typeof ORDER_STATUS_COLORS
    >) {
      expect(typeof ORDER_STATUS_COLORS[key]).toBe('string');
      expect(ORDER_STATUS_COLORS[key].length).toBeGreaterThan(0);
    }
  });
});

// ── Order priority constants ──────────────────────────────────────────────────
describe('constants — ORDER_PRIORITIES', () => {
  it('has NORMAL, URGENT, EMERGENCY entries', () => {
    expect(Object.keys(ORDER_PRIORITIES)).toEqual(
      expect.arrayContaining(['NORMAL', 'URGENT', 'EMERGENCY']),
    );
  });

  it('ORDER_PRIORITY_COLORS covers all priority keys', () => {
    for (const key of Object.keys(ORDER_PRIORITIES) as Array<
      keyof typeof ORDER_PRIORITY_COLORS
    >) {
      expect(ORDER_PRIORITY_COLORS[key]).toBeTruthy();
    }
  });
});

// ── Result status constants ───────────────────────────────────────────────────
describe('constants — RESULT_STATUSES', () => {
  it('contains PENDING, ENTERED, VALIDATED, REJECTED, REPEATED', () => {
    expect(Object.keys(RESULT_STATUSES)).toEqual(
      expect.arrayContaining([
        'PENDING',
        'ENTERED',
        'VALIDATED',
        'REJECTED',
        'REPEATED',
      ]),
    );
  });

  it('every RESULT_STATUS_COLORS entry is a non-empty string', () => {
    for (const key of Object.keys(RESULT_STATUSES) as Array<
      keyof typeof RESULT_STATUS_COLORS
    >) {
      expect(typeof RESULT_STATUS_COLORS[key]).toBe('string');
      expect(RESULT_STATUS_COLORS[key].length).toBeGreaterThan(0);
    }
  });
});

// ── Result flag constants ─────────────────────────────────────────────────────
describe('constants — RESULT_FLAGS', () => {
  it('has NORMAL, LOW, HIGH, CRITICAL, UNDEFINED', () => {
    expect(Object.keys(RESULT_FLAGS)).toEqual(
      expect.arrayContaining(['NORMAL', 'LOW', 'HIGH', 'CRITICAL', 'UNDEFINED']),
    );
  });

  it('every RESULT_FLAG_COLORS entry is a non-empty string', () => {
    for (const key of Object.keys(RESULT_FLAGS) as Array<
      keyof typeof RESULT_FLAG_COLORS
    >) {
      expect(RESULT_FLAG_COLORS[key]).toBeTruthy();
    }
  });
});

// ── Sample transition graph ───────────────────────────────────────────────────
describe('constants — SAMPLE_TRANSITIONS', () => {
  it('REGISTERED can transition to IN_ANALYSIS and ARCHIVED', () => {
    expect(SAMPLE_TRANSITIONS['REGISTERED']).toContain('IN_ANALYSIS');
    expect(SAMPLE_TRANSITIONS['REGISTERED']).toContain('ARCHIVED');
  });

  it('ARCHIVED has no outgoing transitions', () => {
    expect(SAMPLE_TRANSITIONS['ARCHIVED']).toHaveLength(0);
  });

  it('VALIDATED can only transition to ARCHIVED', () => {
    expect(SAMPLE_TRANSITIONS['VALIDATED']).toEqual(['ARCHIVED']);
  });

  it('every transition target is itself a key in SAMPLE_TRANSITIONS', () => {
    for (const [, targets] of Object.entries(SAMPLE_TRANSITIONS)) {
      for (const t of targets) {
        expect(SAMPLE_TRANSITIONS).toHaveProperty(t);
      }
    }
  });
});

// ── Order transition graph ────────────────────────────────────────────────────
describe('constants — ORDER_TRANSITIONS', () => {
  it('REQUESTED can transition to IN_PROGRESS and CANCELLED', () => {
    expect(ORDER_TRANSITIONS['REQUESTED']).toContain('IN_PROGRESS');
    expect(ORDER_TRANSITIONS['REQUESTED']).toContain('CANCELLED');
  });

  it('VALIDATED has no outgoing transitions', () => {
    expect(ORDER_TRANSITIONS['VALIDATED']).toHaveLength(0);
  });

  it('CANCELLED has no outgoing transitions', () => {
    expect(ORDER_TRANSITIONS['CANCELLED']).toHaveLength(0);
  });
});

// ── Result transition graph ───────────────────────────────────────────────────
describe('constants — RESULT_TRANSITIONS', () => {
  it('PENDING can transition to ENTERED and REJECTED', () => {
    expect(RESULT_TRANSITIONS['PENDING']).toContain('ENTERED');
    expect(RESULT_TRANSITIONS['PENDING']).toContain('REJECTED');
  });

  it('VALIDATED has no outgoing transitions', () => {
    expect(RESULT_TRANSITIONS['VALIDATED']).toHaveLength(0);
  });

  it('REJECTED has no outgoing transitions', () => {
    expect(RESULT_TRANSITIONS['REJECTED']).toHaveLength(0);
  });
});
