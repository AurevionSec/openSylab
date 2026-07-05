import { useState, useEffect, useId, useRef } from 'react';
import { Button } from './Button';
import { useModalA11y } from '../../hooks/useModalA11y';

interface DeleteConfirmDialogProps {
  isOpen: boolean;
  onClose: () => void;
  onConfirm: () => Promise<void>;
  title: string;
  message: string;
  itemName?: string;
  confirmText?: string;
  cancelText?: string;
  /** Accurate description of what happens (most deletes are soft-deletes). */
  outcomeNote?: string;
}

/**
 * Reusable delete confirmation dialog component.
 *
 * @example
 * <DeleteConfirmDialog
 *   isOpen={showDeleteDialog}
 *   onClose={() => setShowDeleteDialog(false)}
 *   onConfirm={handleDeleteSample}
 *   title="Archive Sample"
 *   message="Archive this sample?"
 *   itemName={sample.sample_id}
 *   outcomeNote="The sample is marked ARCHIVED and hidden from active lists. Audit history is retained."
 * />
 */
export const DeleteConfirmDialog = ({
  isOpen,
  onClose,
  onConfirm,
  title,
  message,
  itemName,
  confirmText = 'Delete',
  cancelText = 'Cancel',
  outcomeNote = 'This action is recorded in the audit trail.',
}: DeleteConfirmDialogProps) => {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const dialogRef = useRef<HTMLDivElement>(null);
  const titleId = useId();
  useEffect(() => { if (!isOpen) setError(''); }, [isOpen]);
  useModalA11y(isOpen, onClose, dialogRef, { disableClose: loading });

  const handleConfirm = async () => {
    setError('');
    setLoading(true);

    try {
      await onConfirm();
      onClose();
    } catch (err: unknown) {
      console.error('[DeleteConfirmDialog] Error during delete:', err);
      let errorMessage = 'Failed to delete item. Please try again.';

      if (err && typeof err === 'object' && 'response' in err) {
        const r = err as {response?: {data?: {error?: {message?: string}}}};
        if (r.response?.data?.error?.message) errorMessage = r.response.data.error.message;
      } else if (err instanceof Error) {
        errorMessage = err.message;
      }

      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  if (!isOpen) return null;

  return (
    <div
      className="fixed inset-0 bg-black/50 flex items-center justify-center p-4 z-50"
      onMouseDown={(e) => { if (e.target === e.currentTarget && !loading) onClose(); }}
    >
      <div
        ref={dialogRef}
        role="dialog"
        aria-modal="true"
        aria-labelledby={titleId}
        tabIndex={-1}
        className="bg-white dark:bg-dark-surface rounded shadow-xl max-w-md w-full focus:outline-none"
      >
        <div className="p-6">
          {/* Header with warning icon */}
          <div className="flex items-start mb-4">
            <div className="flex-shrink-0">
              <div className="w-12 h-12 rounded-full bg-red-100 flex items-center justify-center">
                <svg
                  className="w-6 h-6 text-red-600"
                  fill="none"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth="2"
                  viewBox="0 0 24 24"
                  stroke="currentColor"
                  aria-hidden="true"
                >
                  <path d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
                </svg>
              </div>
            </div>
            <div className="ml-4 flex-1">
              <h3 id={titleId} className="text-lg font-semibold text-gray-900 dark:text-white mb-2">
                {title}
              </h3>
              <div className="text-sm text-gray-600 dark:text-gray-300 space-y-2">
                <p>{message}</p>
                {itemName && (
                  <p className="font-medium text-gray-900 dark:text-white">
                    Item: <span className="text-red-600 dark:text-red-400 font-mono">{itemName}</span>
                  </p>
                )}
                <p className="text-gray-500 dark:text-gray-400">{outcomeNote}</p>
              </div>
            </div>
          </div>

          {/* Error message */}
          {error && (
            <div className="mb-4 bg-red-50 border border-red-200 rounded p-3" role="alert">
              <p className="text-red-800 text-sm">{error}</p>
            </div>
          )}

          {/* Action buttons */}
          <div className="flex justify-end gap-3 pt-4 border-t border-gray-200 dark:border-white/10">
            <Button
              type="button"
              variant="secondary"
              onClick={onClose}
              disabled={loading}
            >
              {cancelText}
            </Button>
            <Button
              type="button"
              variant="danger"
              onClick={handleConfirm}
              disabled={loading}
            >
              {loading ? 'Working…' : confirmText}
            </Button>
          </div>
        </div>
      </div>
    </div>
  );
};

export default DeleteConfirmDialog;
