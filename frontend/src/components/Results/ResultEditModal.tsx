import { useState, useEffect } from 'react';
import type { FormEvent } from 'react';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { updateResult } from '../../services/results';
import type { TestResult } from '../../types/result';
import { RESULT_STATUSES, RESULT_FLAGS } from '../../utils/constants';

interface ResultEditModalProps {
  isOpen: boolean;
  onClose: () => void;
  result: TestResult | null;
  onSuccess?: (result: TestResult) => void;
}

export const ResultEditModal = ({ isOpen, onClose, result, onSuccess }: ResultEditModalProps) => {
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

  useEffect(() => {
    if (result) {
      setFormData({
        result_id: result.result_id,
        order_id: result.order_id,
        parameter: result.parameter,
        value: result.value,
        unit: result.unit || '',
        reference_min: result.reference_min || '',
        reference_max: result.reference_max || '',
        flag: result.flag,
        status: result.status,
        reviewed_by: result.reviewed_by || '',
        notes: result.notes || '',
      });
    }
  }, [result]);

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    if (!result) return;

    setError('');
    setLoading(true);

    try {
      console.log('[ResultEdit] Updating result:', result.id, formData);
      const updatedResult = await updateResult(result.id.toString(), formData);
      console.log('[ResultEdit] Result updated successfully:', updatedResult);

      if (onSuccess) {
        onSuccess(updatedResult);
      }

      onClose();
    } catch (err: any) {
      console.error('[ResultEdit] Error updating result:', err);
      let errorMessage = 'Failed to update result. Please try again.';

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

  if (!isOpen || !result) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50 animate-backdrop">
      <div className="bg-white rounded shadow-xl max-w-3xl w-full max-h-[90vh] overflow-y-auto">
        <div className="p-6">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-2xl font-bold text-gray-900">Edit Test Result</h2>
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

            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Basic Information</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <Input type="text" label="Result ID *" value={formData.result_id} onChange={(e) => handleChange('result_id', e.target.value)} required disabled />
                <Input type="text" label="Order ID *" value={formData.order_id} onChange={(e) => handleChange('order_id', e.target.value)} required disabled />
              </div>
            </div>

            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Test Parameters</h3>
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                <Input type="text" label="Parameter *" value={formData.parameter} onChange={(e) => handleChange('parameter', e.target.value)} required />
                <Input type="text" label="Value *" value={formData.value} onChange={(e) => handleChange('value', e.target.value)} required />
                <Input type="text" label="Unit" value={formData.unit} onChange={(e) => handleChange('unit', e.target.value)} />
              </div>
            </div>

            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Reference Range</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <Input type="text" label="Min Value" value={formData.reference_min} onChange={(e) => handleChange('reference_min', e.target.value)} />
                <Input type="text" label="Max Value" value={formData.reference_max} onChange={(e) => handleChange('reference_max', e.target.value)} />
              </div>
            </div>

            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Status & Flag</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">Flag *</label>
                  <select value={formData.flag} onChange={(e) => handleChange('flag', e.target.value)} className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent" required>
                    {Object.entries(RESULT_FLAGS).map(([key, label]) => (<option key={key} value={key}>{label}</option>))}
                  </select>
                </div>
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">Status *</label>
                  <select value={formData.status} onChange={(e) => handleChange('status', e.target.value)} className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent" required>
                    {Object.entries(RESULT_STATUSES).map(([key, label]) => (<option key={key} value={key}>{label}</option>))}
                  </select>
                </div>
              </div>
            </div>

            <div className="border-b border-gray-200 pb-4">
              <h3 className="text-lg font-semibold text-gray-900 mb-3">Review Information</h3>
              <Input type="text" label="Reviewed By" value={formData.reviewed_by} onChange={(e) => handleChange('reviewed_by', e.target.value)} />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">Notes</label>
              <textarea value={formData.notes} onChange={(e) => handleChange('notes', e.target.value)} placeholder="Enter additional notes (optional)" rows={4} className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent resize-none" />
            </div>

            <div className="flex justify-end gap-3 pt-4 border-t border-gray-200">
              <Button type="button" variant="secondary" onClick={onClose} disabled={loading}>Cancel</Button>
              <Button type="submit" variant="primary" disabled={loading || !formData.parameter || !formData.value}>{loading ? 'Updating...' : 'Update Result'}</Button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default ResultEditModal;
