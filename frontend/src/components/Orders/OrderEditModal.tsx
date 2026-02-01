import { useState, useEffect } from 'react';
import type { FormEvent } from 'react';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { getOrderById, updateOrder } from '../../services/orders';
import type { Order } from '../../types/order';
import { ORDER_STATUSES, ORDER_PRIORITIES } from '../../utils/constants';

interface OrderEditModalProps {
  isOpen: boolean;
  orderId: string | null;
  onClose: () => void;
  onSuccess?: (order: Order) => void;
}

export const OrderEditModal = ({ isOpen, orderId, onClose, onSuccess }: OrderEditModalProps) => {
  const [formData, setFormData] = useState({
    order_id: '',
    sample_id: '',
    test_type: '',
    status: 'REQUESTED' as Order['status'],
    priority: 'NORMAL' as Order['priority'],
    requested_by: '',
    notes: '',
  });
  const [loading, setLoading] = useState(false);
  const [loadingData, setLoadingData] = useState(false);
  const [error, setError] = useState('');

  // Load order data when modal opens
  useEffect(() => {
    if (isOpen && orderId) {
      const loadOrder = async () => {
        setLoadingData(true);
        setError('');
        try {
          const order = await getOrderById(orderId);
          setFormData({
            order_id: order.order_id,
            sample_id: order.sample_id,
            test_type: order.test_type,
            status: order.status,
            priority: order.priority,
            requested_by: order.requested_by,
            notes: order.notes,
          });
        } catch (err: any) {
          console.error('[OrderEdit] Error loading order:', err);
          setError('Failed to load order data. Please try again.');
        } finally {
          setLoadingData(false);
        }
      };

      loadOrder();
    }
  }, [isOpen, orderId]);

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    if (!orderId) return;

    setError('');
    setLoading(true);

    try {
      console.log('[OrderEdit] Updating order:', orderId, formData);
      const updatedOrder = await updateOrder(orderId, formData);
      console.log('[OrderEdit] Order updated successfully:', updatedOrder);

      // Call success callback
      if (onSuccess) {
        onSuccess(updatedOrder);
      }

      // Close modal
      onClose();
    } catch (err: any) {
      console.error('[OrderEdit] Error updating order:', err);
      let errorMessage = 'Failed to update order. Please try again.';

      if (err.response?.data?.error?.message) {
        errorMessage = err.response.data.error.message;
      } else if (err.message) {
        errorMessage = err.message;
      }

      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  const handleChange = (field: keyof typeof formData, value: string) => {
    setFormData((prev) => ({ ...prev, [field]: value }));
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
      <div className="bg-white rounded-lg shadow-xl max-w-2xl w-full max-h-[90vh] overflow-y-auto">
        <div className="p-6">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-2xl font-bold text-gray-900">Edit Order</h2>
            <button
              onClick={onClose}
              className="text-gray-400 hover:text-gray-600 transition-colors"
              aria-label="Close modal"
            >
              <svg
                className="w-6 h-6"
                fill="none"
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth="2"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path d="M6 18L18 6M6 6l12 12"></path>
              </svg>
            </button>
          </div>

          {loadingData ? (
            <div className="flex items-center justify-center py-12">
              <div className="text-center">
                <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
                <p className="mt-4 text-gray-600">Loading order data...</p>
              </div>
            </div>
          ) : (
            <form onSubmit={handleSubmit} className="space-y-4">
              {error && (
                <div className="bg-red-50 border border-red-200 rounded-lg p-4">
                  <p className="text-red-800 text-sm">{error}</p>
                </div>
              )}

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <Input
                  type="text"
                  label="Order ID *"
                  placeholder="e.g., ORD-2024-001"
                  value={formData.order_id}
                  onChange={(e) => handleChange('order_id', e.target.value)}
                  required
                  autoFocus
                />

                <Input
                  type="text"
                  label="Sample ID *"
                  placeholder="e.g., S2024-001"
                  value={formData.sample_id}
                  onChange={(e) => handleChange('sample_id', e.target.value)}
                  required
                />
              </div>

              <Input
                type="text"
                label="Test Type *"
                placeholder="e.g., Complete Blood Count"
                value={formData.test_type}
                onChange={(e) => handleChange('test_type', e.target.value)}
                required
              />

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">
                    Status *
                  </label>
                  <select
                    value={formData.status}
                    onChange={(e) => handleChange('status', e.target.value)}
                    className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    required
                  >
                    {Object.entries(ORDER_STATUSES).map(([key, label]) => (
                      <option key={key} value={key}>
                        {label}
                      </option>
                    ))}
                  </select>
                </div>

                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">
                    Priority *
                  </label>
                  <select
                    value={formData.priority}
                    onChange={(e) => handleChange('priority', e.target.value)}
                    className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    required
                  >
                    {Object.entries(ORDER_PRIORITIES).map(([key, label]) => (
                      <option key={key} value={key}>
                        {label}
                      </option>
                    ))}
                  </select>
                </div>
              </div>

              <Input
                type="text"
                label="Requested By *"
                placeholder="e.g., Dr. Smith"
                value={formData.requested_by}
                onChange={(e) => handleChange('requested_by', e.target.value)}
                required
              />

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Notes
                </label>
                <textarea
                  value={formData.notes}
                  onChange={(e) => handleChange('notes', e.target.value)}
                  placeholder="Enter order notes (optional)"
                  rows={4}
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent resize-none"
                />
              </div>

              <div className="flex justify-end gap-3 pt-4 border-t border-gray-200">
                <Button
                  type="button"
                  variant="secondary"
                  onClick={onClose}
                  disabled={loading}
                >
                  Cancel
                </Button>
                <Button
                  type="submit"
                  variant="primary"
                  disabled={loading || !formData.order_id || !formData.sample_id || !formData.test_type || !formData.requested_by}
                >
                  {loading ? 'Updating...' : 'Update Order'}
                </Button>
              </div>
            </form>
          )}
        </div>
      </div>
    </div>
  );
};

export default OrderEditModal;
