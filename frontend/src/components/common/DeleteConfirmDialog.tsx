import { useState } from 'react';
import { Button } from './Button';

interface DeleteConfirmDialogProps {
  isOpen: boolean;
  onClose: () => void;
  onConfirm: () => Promise<void>;
  title: string;
  message: string;
  itemName?: string;
  confirmText?: string;
  cancelText?: string;
}

/**
 * Reusable delete confirmation dialog component
 *
 * @example
 * <DeleteConfirmDialog
 *   isOpen={showDeleteDialog}
 *   onClose={() => setShowDeleteDialog(false)}
 *   onConfirm={handleDeleteSample}
 *   title="Delete Sample"
 *   message="Are you sure you want to delete this sample?"
 *   itemName={sample.sample_id}
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
}: DeleteConfirmDialogProps) => {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

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
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
      <div className="bg-white rounded shadow-xl max-w-md w-full">
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
                >
                  <path d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
                </svg>
              </div>
            </div>
            <div className="ml-4 flex-1">
              <h3 className="text-lg font-semibold text-gray-900 mb-2">
                {title}
              </h3>
              <div className="text-sm text-gray-600 space-y-2">
                <p>{message}</p>
                {itemName && (
                  <p className="font-medium text-gray-900">
                    Item: <span className="text-red-600">{itemName}</span>
                  </p>
                )}
                <p className="text-red-600 font-medium">
                  This action cannot be undone.
                </p>
              </div>
            </div>
          </div>

          {/* Error message */}
          {error && (
            <div className="mb-4 bg-red-50 border border-red-200 rounded p-3">
              <p className="text-red-800 text-sm">{error}</p>
            </div>
          )}

          {/* Action buttons */}
          <div className="flex justify-end gap-3 pt-4 border-t border-gray-200">
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
              {loading ? 'Deleting...' : confirmText}
            </Button>
          </div>
        </div>
      </div>
    </div>
  );
};

export default DeleteConfirmDialog;
