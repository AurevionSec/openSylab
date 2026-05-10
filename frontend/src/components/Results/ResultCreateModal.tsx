import { useState, useEffect } from 'react';
import type { FormEvent } from 'react';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { createResult } from '../../services/results';
import { getOrders } from '../../services/orders';
import type { TestResult } from '../../types/result';
import type { Order } from '../../types/order';
import { RESULT_STATUSES, RESULT_FLAGS, RESULT_FLAG_COLORS } from '../../utils/constants';

interface ResultCreateModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSuccess?: (result: TestResult) => void;
}

function computeFlag(value: string, refMin: string, refMax: string): TestResult['flag'] {
  const v = parseFloat(value);
  if (isNaN(v)) return 'UNDEFINED';
  const min = parseFloat(refMin);
  const max = parseFloat(refMax);
  if (!isNaN(min) && v < min) return 'LOW';
  if (!isNaN(max) && v > max) return 'HIGH';
  return 'NORMAL';
}

export const ResultCreateModal = ({ isOpen, onClose, onSuccess }: ResultCreateModalProps) => {
  const [formData, setFormData] = useState({
    result_id: '',
    order_id: '',
    parameter: '',
    value: '',
    unit: '',
    reference_min: '',
    reference_max: '',
    flag: 'NORMAL' as TestResult['flag'],
    status: 'PENDING' as TestResult['status'],
    reviewed_by: '',
    notes: '',
  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [availableOrders, setAvailableOrders] = useState<Order[]>([]);
  const [ordersLoading, setOrdersLoading] = useState(false);

  useEffect(() => {
    if (!isOpen) return;
    setOrdersLoading(true);
    getOrders({ limit: 200 })
      .then((r) =>
        setAvailableOrders(
          r.orders.filter((o) => o.status === 'IN_PROGRESS' || o.status === 'COMPLETED')
        )
      )
      .catch(() => setAvailableOrders([]))
      .finally(() => setOrdersLoading(false));
  }, [isOpen]);

  useEffect(() => {
    if (!formData.value) return;
    const autoFlag = computeFlag(formData.value, formData.reference_min, formData.reference_max);
    setFormData((prev) => ({ ...prev, flag: autoFlag }));
  }, [formData.value, formData.reference_min, formData.reference_max]);

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);
    try {
      const newResult = await createResult(formData);
      setFormData({
        result_id: '',
        order_id: '',
        parameter: '',
        value: '',
        unit: '',
        reference_min: '',
        reference_max: '',
        flag: 'NORMAL',
        status: 'PENDING',
        reviewed_by: '',
        notes: '',
      });
      if (onSuccess) onSuccess(newResult);
      onClose();
    } catch (err: unknown) {
      let errorMessage = 'Failed to create result. Please try again.';
      if (err && typeof err === 'object' && 'response' in err) {
        const r = err as { response?: { data?: { error?: { message?: string } } } };
        errorMessage = r.response?.data?.error?.message ?? errorMessage;
      } else if (err instanceof Error) {
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

  const flagColorClass = RESULT_FLAG_COLORS[formData.flag] ?? 'bg-gray-100 text-gray-800';
  const valueNum = parseFloat(formData.value);
  const refMinNum = parseFloat(formData.reference_min);
  const refMaxNum = parseFloat(formData.reference_max);
  const valueOutOfRange =
    !isNaN(valueNum) &&
    ((!isNaN(refMinNum) && valueNum < refMinNum) || (!isNaN(refMaxNum) && valueNum > refMaxNum));

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50 animate-backdrop">
      <div className="bg-white rounded shadow-xl max-w-3xl w-full max-h-[90vh] overflow-y-auto">
        <div className="p-6">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-2xl font-bold text-gray-900">Create New Test Result</h2>
            <button
              onClick={onClose}
              className="text-gray-400 hover:text-gray-600 transition-colors"
              aria-label="Close modal"
            >
              <svg className="w-6 h-6" fill="none" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" viewBox="0 0 24 24" stroke="currentColor">
                <path d="M6 18L18 6M6 6l12 12"></path>
              </svg>
            </button>
          </div>

          <form onSubmit={handleSubmit} className="space-y-4">
            {error && (
              <div className="bg-red-50 border border-red-200 rounded p-4">
                <p className="text-red-800 text-sm">{error}</p>
              </div>
            )}

            {/* Basic Information */}
            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Basic Information</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <Input
                  type="text"
                  label="Result ID *"
                  placeholder="e.g., R2024-001"
                  value={formData.result_id}
                  onChange={(e) => handleChange('result_id', e.target.value)}
                  required
                  autoFocus
                />

                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">
                    Order *
                  </label>
                  {ordersLoading ? (
                    <p className="text-sm text-gray-500 py-2">Loading orders...</p>
                  ) : availableOrders.length === 0 ? (
                    <p className="text-sm text-orange-600 py-2">No active orders (IN_PROGRESS or COMPLETED) found.</p>
                  ) : (
                    <select
                      value={formData.order_id}
                      onChange={(e) => handleChange('order_id', e.target.value)}
                      className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
                      required
                    >
                      <option value="">— Select Order —</option>
                      {availableOrders.map((o) => (
                        <option key={o.id} value={String(o.id)}>
                          {o.order_id} ({o.test_type})
                        </option>
                      ))}
                    </select>
                  )}
                </div>
              </div>
            </div>

            {/* Test Parameters */}
            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Test Parameters</h3>
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                <Input
                  type="text"
                  label="Parameter *"
                  placeholder="e.g., Glucose"
                  value={formData.parameter}
                  onChange={(e) => handleChange('parameter', e.target.value)}
                  required
                />

                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">
                    Value *
                  </label>
                  <input
                    type="text"
                    placeholder="e.g., 95"
                    value={formData.value}
                    onChange={(e) => handleChange('value', e.target.value)}
                    required
                    className={`w-full px-3 py-2 border rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent ${
                      formData.value && valueOutOfRange
                        ? 'border-red-400 bg-red-50'
                        : formData.value && !isNaN(valueNum)
                        ? 'border-green-400 bg-green-50'
                        : 'border-gray-300'
                    }`}
                  />
                </div>

                <Input
                  type="text"
                  label="Unit"
                  placeholder="e.g., mg/dL"
                  value={formData.unit}
                  onChange={(e) => handleChange('unit', e.target.value)}
                />
              </div>
            </div>

            {/* Reference Range */}
            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Reference Range</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <Input
                  type="text"
                  label="Min Value"
                  placeholder="e.g., 70"
                  value={formData.reference_min}
                  onChange={(e) => handleChange('reference_min', e.target.value)}
                />
                <Input
                  type="text"
                  label="Max Value"
                  placeholder="e.g., 110"
                  value={formData.reference_max}
                  onChange={(e) => handleChange('reference_max', e.target.value)}
                />
              </div>
            </div>

            {/* Auto-Flag Preview */}
            {formData.value && (
              <div className="flex items-center gap-3 py-2 px-3 bg-gray-50 rounded border border-gray-200">
                <span className="text-sm text-gray-600 font-medium">Auto-flag:</span>
                <span className={`px-2 py-1 text-xs font-bold uppercase rounded ${flagColorClass}`}>
                  {RESULT_FLAGS[formData.flag]}
                </span>
                <span className="text-xs text-gray-400">(computed from value and reference range)</span>
              </div>
            )}

            {/* Status and Flag */}
            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Status & Flag</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">
                    Flag (auto-calculated)
                  </label>
                  <select
                    value={formData.flag}
                    onChange={(e) => handleChange('flag', e.target.value)}
                    className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
                    required
                  >
                    {Object.entries(RESULT_FLAGS).map(([key, label]) => (
                      <option key={key} value={key}>
                        {label}
                      </option>
                    ))}
                  </select>
                </div>

                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">
                    Status *
                  </label>
                  <select
                    value={formData.status}
                    onChange={(e) => handleChange('status', e.target.value)}
                    className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
                    required
                  >
                    {Object.entries(RESULT_STATUSES).map(([key, label]) => (
                      <option key={key} value={key}>
                        {label}
                      </option>
                    ))}
                  </select>
                </div>
              </div>
            </div>

            {/* Review Information */}
            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Review Information</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <Input
                  type="text"
                  label="Reviewed By"
                  placeholder="e.g., Dr. Smith"
                  value={formData.reviewed_by}
                  onChange={(e) => handleChange('reviewed_by', e.target.value)}
                />
              </div>
            </div>

            {/* Notes */}
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                Notes
              </label>
              <textarea
                value={formData.notes}
                onChange={(e) => handleChange('notes', e.target.value)}
                placeholder="Enter additional notes (optional)"
                rows={4}
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent resize-none"
              />
            </div>

            <div className="flex justify-end gap-3 pt-4 border-t border-gray-200">
              <Button type="button" variant="secondary" onClick={onClose} disabled={loading}>
                Cancel
              </Button>
              <Button
                type="submit"
                variant="primary"
                disabled={loading || !formData.result_id || !formData.order_id || !formData.parameter || !formData.value}
              >
                {loading ? 'Creating...' : 'Create Result'}
              </Button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default ResultCreateModal;
