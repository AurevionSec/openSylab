import { useState, useCallback } from 'react';
import type { FormEvent } from 'react';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { createSample } from '../../services/samples';
import type { Sample } from '../../types/sample';
import { SAMPLE_STATUSES } from '../../utils/constants';
import { useBarcode } from '../../hooks/useBarcode';

interface SampleCreateModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSuccess?: (sample: Sample) => void;
}

export const SampleCreateModal = ({ isOpen, onClose, onSuccess }: SampleCreateModalProps) => {
  const [formData, setFormData] = useState({
    sample_id: '',
    patient_id: '',
    patient_name: '',
    description: '',
    status: 'REGISTERED' as Sample['status'],
  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const [showBarcodeScanner, setShowBarcodeScanner] = useState(false);
  const handleBarcodeDetected = useCallback((code: string) => {
    handleChange('sample_id', code);
    setShowBarcodeScanner(false);
  }, []);
  const { isSupported: barcodeSupported, isScanning, error: barcodeError, videoRef, startScan, stopScan } = useBarcode({
    onDetected: handleBarcodeDetected,
  });

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    try {
      console.log('[SampleCreate] Submitting sample:', formData);
      const newSample = await createSample(formData);
      console.log('[SampleCreate] Sample created successfully:', newSample);

      // Reset form
      setFormData({
        sample_id: '',
        patient_id: '',
        patient_name: '',
        description: '',
        status: 'REGISTERED',
      });

      // Call success callback
      if (onSuccess) {
        onSuccess(newSample);
      }

      // Close modal
      onClose();
    } catch (err: unknown) {
      console.error('[SampleCreate] Error creating sample:', err);
      let errorMessage = 'Failed to create sample. Please try again.';

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

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50 animate-backdrop">
      <div className="bg-white rounded shadow-xl max-w-2xl w-full max-h-[90vh] overflow-y-auto animate-snap-in">
        <div className="p-6">
          <div className="flex justify-between items-center mb-6">
            <h2 className="text-2xl font-bold text-gray-900">Create New Sample</h2>
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

          <form onSubmit={handleSubmit} className="space-y-4">
            {error && (
              <div className="bg-red-50 border border-red-200 rounded p-4">
                <p className="text-red-800 text-sm">{error}</p>
              </div>
            )}

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">Sample ID *</label>
                <div className="flex gap-2">
                  <input
                    type="text"
                    placeholder="e.g., S2024-001"
                    value={formData.sample_id}
                    onChange={(e) => handleChange('sample_id', e.target.value)}
                    required
                    autoFocus
                    className="flex-1 px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent text-sm"
                  />
                  {barcodeSupported && (
                    <button
                      type="button"
                      title="Barcode scannen"
                      onClick={() => { setShowBarcodeScanner(true); startScan(); }}
                      className="px-3 py-2 border border-gray-300 rounded hover:bg-gray-50 transition-colors text-gray-600"
                    >
                      <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 4v1m6 11h2m-6 0h-2v4m0-11v3m0 0h.01M12 12h4.01M16 20h4M4 12h4m12 0h.01M5 8h2a1 1 0 001-1V5a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1zm12 0h2a1 1 0 001-1V5a1 1 0 00-1-1h-2a1 1 0 00-1 1v2a1 1 0 001 1zM5 20h2a1 1 0 001-1v-2a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1z" />
                      </svg>
                    </button>
                  )}
                </div>
                {!barcodeSupported && (
                  <p className="mt-1 text-xs text-gray-400">Barcode-Scanner nicht verfügbar — bitte manuell eingeben</p>
                )}
              </div>

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
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
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
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent resize-none"
              />
            </div>

            {showBarcodeScanner && (
              <div className="fixed inset-0 bg-black bg-opacity-75 z-60 flex flex-col items-center justify-center p-4">
                <div className="bg-black rounded-lg overflow-hidden w-full max-w-sm">
                  <div className="p-4 flex justify-between items-center bg-gray-900">
                    <span className="text-white text-sm font-medium">Barcode scannen</span>
                    <button
                      type="button"
                      onClick={() => { stopScan(); setShowBarcodeScanner(false); }}
                      className="text-gray-400 hover:text-white"
                    >
                      <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M6 18L18 6M6 6l12 12" />
                      </svg>
                    </button>
                  </div>
                  <video ref={videoRef} className="w-full" playsInline muted />
                  {barcodeError && <p className="p-3 text-red-400 text-xs">{barcodeError}</p>}
                  {isScanning && <p className="p-3 text-green-400 text-xs text-center">Scanning...</p>}
                </div>
              </div>
            )}

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
                {loading ? 'Creating...' : 'Create Sample'}
              </Button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default SampleCreateModal;
