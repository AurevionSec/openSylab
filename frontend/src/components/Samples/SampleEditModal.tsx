import { useState, useEffect } from 'react';
import type { FormEvent } from 'react';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { getSampleById, updateSample } from '../../services/samples';
import type { Sample } from '../../types/sample';
import { SAMPLE_STATUSES } from '../../utils/constants';

interface SampleEditModalProps {
  isOpen: boolean;
  sampleId: number | null;
  onClose: () => void;
  onSuccess?: (sample: Sample) => void;
}

export const SampleEditModal = ({ isOpen, sampleId, onClose, onSuccess }: SampleEditModalProps) => {
  const [formData, setFormData] = useState({
    sample_id: '',
    patient_id: '',
    patient_name: '',
    description: '',
    status: 'REGISTERED' as Sample['status'],
  });
  const [loading, setLoading] = useState(false);
  const [loadingData, setLoadingData] = useState(false);
  const [error, setError] = useState('');

  // Load sample data when modal opens
  useEffect(() => {
    if (isOpen && sampleId) {
      const loadSample = async () => {
        setLoadingData(true);
        setError('');
        try {
          const sample = await getSampleById(sampleId);
          setFormData({
            sample_id: sample.sample_id,
            patient_id: sample.patient_id,
            patient_name: sample.patient_name,
            description: sample.description,
            status: sample.status,
          });
        } catch (err: any) {
          console.error('[SampleEdit] Error loading sample:', err);
          setError('Failed to load sample data. Please try again.');
        } finally {
          setLoadingData(false);
        }
      };

      loadSample();
    }
  }, [isOpen, sampleId]);

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    if (!sampleId) return;

    setError('');
    setLoading(true);

    try {
      console.log('[SampleEdit] Updating sample:', sampleId, formData);
      const updatedSample = await updateSample(sampleId, formData);
      console.log('[SampleEdit] Sample updated successfully:', updatedSample);

      // Call success callback
      if (onSuccess) {
        onSuccess(updatedSample);
      }

      // Close modal
      onClose();
    } catch (err: any) {
      console.error('[SampleEdit] Error updating sample:', err);
      let errorMessage = 'Failed to update sample. Please try again.';

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
            <h2 className="text-2xl font-bold text-gray-900">Edit Sample</h2>
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
                <p className="mt-4 text-gray-600">Loading sample data...</p>
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
                  label="Sample ID *"
                  placeholder="e.g., S2024-001"
                  value={formData.sample_id}
                  onChange={(e) => handleChange('sample_id', e.target.value)}
                  required
                  autoFocus
                />

                <Input
                  type="text"
                  label="Patient ID *"
                  placeholder="e.g., P12345"
                  value={formData.patient_id}
                  onChange={(e) => handleChange('patient_id', e.target.value)}
                  required
                />
              </div>

              <Input
                type="text"
                label="Patient Name *"
                placeholder="Enter patient name"
                value={formData.patient_name}
                onChange={(e) => handleChange('patient_name', e.target.value)}
                required
              />

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
                  {Object.entries(SAMPLE_STATUSES).map(([key, label]) => (
                    <option key={key} value={key}>
                      {label}
                    </option>
                  ))}
                </select>
              </div>

              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  Description
                </label>
                <textarea
                  value={formData.description}
                  onChange={(e) => handleChange('description', e.target.value)}
                  placeholder="Enter sample description (optional)"
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
                  disabled={loading || !formData.sample_id || !formData.patient_id || !formData.patient_name}
                >
                  {loading ? 'Updating...' : 'Update Sample'}
                </Button>
              </div>
            </form>
          )}
        </div>
      </div>
    </div>
  );
};

export default SampleEditModal;
