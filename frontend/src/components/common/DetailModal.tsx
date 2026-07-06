import { useId, useRef } from 'react';
import type { ReactNode } from 'react';
import { useModalA11y } from '../../hooks/useModalA11y';
import { Button } from './Button';

export interface DetailField {
  label: string;
  value: ReactNode;
  /** Render the value in the tabular data font (IDs, numbers, dates). */
  mono?: boolean;
}

interface DetailModalProps {
  isOpen: boolean;
  onClose: () => void;
  title: string;
  fields: DetailField[];
}

/**
 * Read-only record detail dialog (works for every role, incl. VIEWER). Reuses the
 * modal a11y behaviour; renders a label/value grid with the data font for values.
 */
export const DetailModal = ({ isOpen, onClose, title, fields }: DetailModalProps) => {
  const dialogRef = useRef<HTMLDivElement>(null);
  const titleId = useId();
  useModalA11y(isOpen, onClose, dialogRef);

  if (!isOpen) return null;

  return (
    <div
      className="fixed inset-0 bg-black/50 flex items-center justify-center p-4 z-50 animate-backdrop"
      onMouseDown={(e) => {
        if (e.target === e.currentTarget) onClose();
      }}
    >
      <div
        ref={dialogRef}
        role="dialog"
        aria-modal="true"
        aria-labelledby={titleId}
        tabIndex={-1}
        className="bg-white dark:bg-dark-surface rounded border border-clinical-border dark:border-white/10 w-full max-w-2xl max-h-[90vh] overflow-y-auto focus:outline-none"
      >
        <div className="flex items-center justify-between px-6 py-4 border-b border-clinical-border dark:border-white/10">
          <h2 id={titleId} className="text-lg font-bold text-clinical-text dark:text-white">
            {title}
          </h2>
          <button
            onClick={onClose}
            aria-label="Close dialog"
            className="text-2xl leading-none text-clinical-secondary hover:text-clinical-text dark:hover:text-white focus:outline-none focus-visible:ring-2 focus-visible:ring-[#0055FF] rounded"
          >
            ×
          </button>
        </div>

        <dl className="grid grid-cols-1 sm:grid-cols-2 gap-x-6 gap-y-4 p-6">
          {fields.map((f) => (
            <div key={f.label}>
              <dt className="text-xs font-semibold uppercase tracking-wide text-clinical-secondary">
                {f.label}
              </dt>
              <dd
                className={`mt-1 text-sm text-clinical-text dark:text-gray-200 break-words ${
                  f.mono ? 'font-mono' : ''
                }`}
              >
                {f.value === '' || f.value === null || f.value === undefined ? '—' : f.value}
              </dd>
            </div>
          ))}
        </dl>

        <div className="flex justify-end px-6 py-4 border-t border-clinical-border dark:border-white/10">
          <Button variant="secondary" onClick={onClose}>
            Close
          </Button>
        </div>
      </div>
    </div>
  );
};
