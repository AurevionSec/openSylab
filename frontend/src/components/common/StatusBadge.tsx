import type { ReactNode } from 'react';

interface StatusBadgeProps {
  /** Tailwind color classes, e.g. from STATUS_COLORS / RESULT_FLAG_COLORS. */
  colorClass?: string;
  children: ReactNode;
}

/**
 * The one status/flag/priority badge. Rectangular (2px radius), uppercase,
 * bordered — per DESIGN.md §4B ("Rectangular tags, no soft pills"). Callers pass
 * the semantic color class; this owns the shape so every badge looks the same.
 */
export const StatusBadge = ({
  colorClass = 'bg-gray-100 text-gray-700 border-gray-300',
  children,
}: StatusBadgeProps) => (
  <span
    className={`inline-flex items-center rounded-[2px] border px-2 py-0.5 text-xs font-semibold uppercase tracking-wide ${colorClass}`}
  >
    {children}
  </span>
);
