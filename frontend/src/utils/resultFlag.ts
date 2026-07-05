import type { TestResult } from '../types/result';

/**
 * Auto-flag a numeric result against its reference range. Mirrors the backend
 * margin-based rule (CRITICAL = 50% of the reference interval beyond the bounds).
 * Returns UNDEFINED when the value or range isn't a usable finite range — shared
 * by the create and edit modals so an edited value can't keep a stale flag.
 */
export function computeFlag(
  value: string,
  refMin: string,
  refMax: string
): TestResult['flag'] {
  const v = parseFloat(value);
  if (!Number.isFinite(v)) return 'UNDEFINED';
  const min = parseFloat(refMin);
  const max = parseFloat(refMax);
  if (!Number.isFinite(min) || !Number.isFinite(max) || min >= max) return 'UNDEFINED';
  const margin = (max - min) * 0.5;
  const criticalLow = min - margin;
  const criticalHigh = max + margin;
  if (v < criticalLow || v > criticalHigh) return 'CRITICAL';
  if (v < min) return 'LOW';
  if (v > max) return 'HIGH';
  return 'NORMAL';
}
