import { useState, useEffect } from 'react';
import type { FormEvent } from 'react';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { createOrder } from '../../services/orders';
import { getSamples } from '../../services/samples';
import type { Order } from '../../types/order';
import type { Sample } from '../../types/sample';
import { ORDER_PRIORITIES } from '../../utils/constants';

interface OrderCreateModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSuccess?: (order: Order) => void;
}

export const OrderCreateModal = ({ isOpen, onClose, onSuccess }: OrderCreateModalProps) => {
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
  const [error, setError] = useState('');
  const [availableSamples, setAvailableSamples] = useState<Sample[]>([]);
  const [samplesLoading, setSamplesLoading] = useState(false);
  const [samplesError, setSamplesError] = useState('');
  const [sampleSearch, setSampleSearch] = useState('');

  useEffect(() => {
    if (!isOpen) return;
    setSamplesError('');
    setSamplesLoading(true);
    getSamples({ limit: 200 })
      .then((r) => setAvailableSamples(r.samples))
      .catch(() => { setAvailableSamples([]); setSamplesError('Fehler beim Laden der Proben.'); })
      .finally(() => setSamplesLoading(false));
  }, [isOpen]);

  const filteredSamples = availableSamples.filter(
    (s) =>
      s.sample_id.toLowerCase().includes(sampleSearch.toLowerCase()) ||
      s.patient_name.toLowerCase().includes(sampleSearch.toLowerCase())
  );

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);
    try {
      const newOrder = await createOrder(formData);
      setFormData({
        order_id: '',
        sample_id: '',
        test_type: '',
        status: 'REQUESTED',
        priority: 'NORMAL',
        requested_by: '',
        notes: '',
      });
      setSampleSearch('');
      if (onSuccess) onSuccess(newOrder);
      onClose();
    } catch (err: unknown) {
      let errorMessage = 'Failed to create order. Please try again.';
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

  const canSubmit = !loading && formData.order_id && formData.sample_id && formData.test_type && formData.requested_by;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50 animate-backdrop">
      <div className="bg-white rounded shadow-xl max-w-2xl w-full max-h-[90vh] overflow-y-auto animate-snap-in">
        <div className="p-6">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-2xl font-bold text-gray-900">Create Order</h2>
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

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Sample *
                </label>
                <Input
                  type="text"
                  placeholder="Search by Sample ID or patient name..."
                  value={sampleSearch}
                  onChange={(e) => {
                    setSampleSearch(e.target.value);
                    handleChange('sample_id', '');
                  }}
                />
                {sampleSearch && (
                  <div className="mt-1 border border-gray-200 rounded bg-white shadow-sm max-h-40 overflow-y-auto">
                    {samplesLoading ? (
                      <p className="p-2 text-sm text-gray-500">Loading samples...</p>
                    ) : filteredSamples.length === 0 ? (
                      {samplesError
                        ? <p className="p-2 text-sm text-red-600">{samplesError}</p>
                        : <p className="p-2 text-sm text-gray-500">Keine Proben gefunden. Zuerst eine Probe anlegen.</p>}
                    ) : (
                      filteredSamples.slice(0, 10).map((s) => (
                        <button
                          key={s.id}
                          type="button"
                          className="w-full text-left px-3 py-2 text-sm hover:bg-blue-50 flex justify-between items-center"
                          onClick={() => {
                            handleChange('sample_id', s.sample_id);
                            setSampleSearch(`${s.sample_id} — ${s.patient_name}`);
                          }}
                        >
                          <span className="font-mono font-bold">{s.sample_id}</span>
                          <span className="text-gray-500 truncate ml-2">{s.patient_name}</span>
                        </button>
                      ))
                    )}
                  </div>
                )}
                {formData.sample_id && (
                  <p className="mt-1 text-xs text-green-600 font-medium">✓ Selected: {formData.sample_id}</p>
                )}
              </div>
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
                  Priority *
                </label>
                <select
                  value={formData.priority}
                  onChange={(e) => handleChange('priority', e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
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
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent resize-none"
              />
            </div>

            <div className="flex justify-end gap-3 pt-4 border-t border-gray-200">
              <Button type="button" variant="secondary" onClick={onClose} disabled={loading}>
                Cancel
              </Button>
              <Button type="submit" variant="primary" disabled={!canSubmit}>
                {loading ? 'Creating...' : 'Create Order'}
              </Button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default OrderCreateModal;
