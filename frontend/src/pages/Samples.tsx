import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { DeleteConfirmDialog } from '../components/common/DeleteConfirmDialog';
import { SampleCreateModal } from '../components/Samples/SampleCreateModal';
import { SampleEditModal } from '../components/Samples/SampleEditModal';
import { getSamples, deleteSample } from '../services/samples';
import type { Sample } from '../types/sample';
import { SAMPLE_STATUSES, STATUS_COLORS } from '../utils/constants';

export const Samples = () => {
  const [samples, setSamples] = useState<Sample[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [selectedStatus, setSelectedStatus] = useState<string>('');
  const [currentPage, setCurrentPage] = useState(1);
  const [totalSamples, setTotalSamples] = useState(0);
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isEditModalOpen, setIsEditModalOpen] = useState(false);
  const [selectedSampleId, setSelectedSampleId] = useState<string | null>(null);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [sampleToDelete, setSampleToDelete] = useState<Sample | null>(null);
  const itemsPerPage = 20;

  useEffect(() => {
    const fetchSamples = async () => {
      setLoading(true);
      try {
        const data = await getSamples({
          status: selectedStatus || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setSamples(data.samples);
        setTotalSamples(data.total);
      } catch (err) {
        setError('Failed to load samples');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };

    fetchSamples();
  }, [selectedStatus, currentPage]);

  const totalPages = Math.ceil(totalSamples / itemsPerPage);

  const handleStatusFilter = (status: string) => {
    setSelectedStatus(status);
    setCurrentPage(1);
  };

  const handlePageChange = (page: number) => {
    setCurrentPage(page);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const handleCreateSuccess = (newSample: Sample) => {
    console.log('[Samples] Sample created successfully:', newSample);
    // Refresh the samples list
    const fetchSamples = async () => {
      setLoading(true);
      try {
        const data = await getSamples({
          status: selectedStatus || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setSamples(data.samples);
        setTotalSamples(data.total);
      } catch (err) {
        setError('Failed to load samples');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    fetchSamples();
  };

  const handleEditClick = (sampleId: string) => {
    setSelectedSampleId(sampleId);
    setIsEditModalOpen(true);
  };

  const handleEditSuccess = (updatedSample: Sample) => {
    console.log('[Samples] Sample updated successfully:', updatedSample);
    // Refresh the samples list
    const fetchSamples = async () => {
      setLoading(true);
      try {
        const data = await getSamples({
          status: selectedStatus || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setSamples(data.samples);
        setTotalSamples(data.total);
      } catch (err) {
        setError('Failed to load samples');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    fetchSamples();
  };

  const handleDeleteClick = (sample: Sample) => {
    setSampleToDelete(sample);
    setIsDeleteDialogOpen(true);
  };

  const handleDeleteConfirm = async () => {
    if (!sampleToDelete) return;

    console.log('[Samples] Deleting sample:', sampleToDelete.sample_id);
    await deleteSample(sampleToDelete.sample_id);

    console.log('[Samples] Sample deleted successfully');
    // Refresh the samples list
    const fetchSamples = async () => {
      setLoading(true);
      try {
        const data = await getSamples({
          status: selectedStatus || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setSamples(data.samples);
        setTotalSamples(data.total);
      } catch (err) {
        setError('Failed to load samples');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    await fetchSamples();
  };

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex justify-between items-center">
          <div>
            <h2 className="text-3xl font-bold text-gray-900">Samples</h2>
            <p className="text-gray-600 mt-1">Manage and view all laboratory samples</p>
          </div>
          <Button
            variant="primary"
            onClick={() => setIsCreateModalOpen(true)}
          >
            <span className="flex items-center gap-2">
              <svg
                className="w-5 h-5"
                fill="none"
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth="2"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path d="M12 4v16m8-8H4"></path>
              </svg>
              Create Sample
            </span>
          </Button>
        </div>

        <Card>
          <div className="mb-6">
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Filter by Status
            </label>
            <div className="flex flex-wrap gap-2">
              <Button
                variant={selectedStatus === '' ? 'primary' : 'secondary'}
                size="sm"
                onClick={() => handleStatusFilter('')}
              >
                All
              </Button>
              {Object.entries(SAMPLE_STATUSES).map(([key, label]) => (
                <Button
                  key={key}
                  variant={selectedStatus === key ? 'primary' : 'secondary'}
                  size="sm"
                  onClick={() => handleStatusFilter(key)}
                >
                  {label}
                </Button>
              ))}
            </div>
          </div>

          {loading ? (
            <div className="flex items-center justify-center py-12">
              <div className="text-center">
                <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
                <p className="mt-4 text-gray-600">Loading samples...</p>
              </div>
            </div>
          ) : error ? (
            <div className="bg-red-50 border border-red-200 rounded-lg p-4">
              <p className="text-red-800">{error}</p>
            </div>
          ) : samples.length === 0 ? (
            <div className="text-center py-12">
              <p className="text-gray-500 text-lg">No samples found</p>
              <p className="text-gray-400 mt-2">Try adjusting your filters or create a new sample</p>
            </div>
          ) : (
            <>
              <div className="overflow-x-auto">
                <table className="min-w-full divide-y divide-gray-200">
                  <thead className="bg-gray-50">
                    <tr>
                      <th
                        scope="col"
                        className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Sample ID
                      </th>
                      <th
                        scope="col"
                        className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Patient ID
                      </th>
                      <th
                        scope="col"
                        className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Patient Name
                      </th>
                      <th
                        scope="col"
                        className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Description
                      </th>
                      <th
                        scope="col"
                        className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Status
                      </th>
                      <th
                        scope="col"
                        className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Created
                      </th>
                      <th
                        scope="col"
                        className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider"
                      >
                        Actions
                      </th>
                    </tr>
                  </thead>
                  <tbody className="bg-white divide-y divide-gray-200">
                    {samples.map((sample) => (
                      <tr key={sample.id} className="hover:bg-gray-50 transition-colors">
                        <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-blue-600">
                          {sample.sample_id}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                          {sample.patient_id}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                          {sample.patient_name}
                        </td>
                        <td className="px-6 py-4 text-sm text-gray-600 max-w-xs truncate">
                          {sample.description}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap">
                          <span
                            className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${
                              STATUS_COLORS[sample.status]
                            }`}
                          >
                            {SAMPLE_STATUSES[sample.status]}
                          </span>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                          {new Date(sample.created_at).toLocaleDateString()}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-right text-sm font-medium">
                          <div className="flex items-center justify-end gap-2">
                            <Button
                              variant="secondary"
                              size="sm"
                              onClick={() => handleEditClick(sample.sample_id)}
                            >
                              <span className="flex items-center gap-1">
                                <svg
                                  className="w-4 h-4"
                                  fill="none"
                                  strokeLinecap="round"
                                  strokeLinejoin="round"
                                  strokeWidth="2"
                                  viewBox="0 0 24 24"
                                  stroke="currentColor"
                                >
                                  <path d="M11 5H6a2 2 0 00-2 2v11a2 2 0 002 2h11a2 2 0 002-2v-5m-1.414-9.414a2 2 0 112.828 2.828L11.828 15H9v-2.828l8.586-8.586z"></path>
                                </svg>
                                Edit
                              </span>
                            </Button>
                            <Button
                              variant="danger"
                              size="sm"
                              onClick={() => handleDeleteClick(sample)}
                            >
                              <span className="flex items-center gap-1">
                                <svg
                                  className="w-4 h-4"
                                  fill="none"
                                  strokeLinecap="round"
                                  strokeLinejoin="round"
                                  strokeWidth="2"
                                  viewBox="0 0 24 24"
                                  stroke="currentColor"
                                >
                                  <path d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16"></path>
                                </svg>
                                Delete
                              </span>
                            </Button>
                          </div>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>

              {totalPages > 1 && (
                <div className="mt-6 flex items-center justify-between border-t border-gray-200 pt-4">
                  <div className="text-sm text-gray-700">
                    Showing {(currentPage - 1) * itemsPerPage + 1} to{' '}
                    {Math.min(currentPage * itemsPerPage, totalSamples)} of {totalSamples} results
                  </div>
                  <div className="flex gap-2">
                    <Button
                      variant="secondary"
                      size="sm"
                      onClick={() => handlePageChange(currentPage - 1)}
                      disabled={currentPage === 1}
                    >
                      Previous
                    </Button>
                    <div className="flex items-center gap-1">
                      {Array.from({ length: Math.min(5, totalPages) }, (_, i) => {
                        let pageNum;
                        if (totalPages <= 5) {
                          pageNum = i + 1;
                        } else if (currentPage <= 3) {
                          pageNum = i + 1;
                        } else if (currentPage >= totalPages - 2) {
                          pageNum = totalPages - 4 + i;
                        } else {
                          pageNum = currentPage - 2 + i;
                        }
                        return (
                          <Button
                            key={pageNum}
                            variant={currentPage === pageNum ? 'primary' : 'secondary'}
                            size="sm"
                            onClick={() => handlePageChange(pageNum)}
                          >
                            {pageNum}
                          </Button>
                        );
                      })}
                    </div>
                    <Button
                      variant="secondary"
                      size="sm"
                      onClick={() => handlePageChange(currentPage + 1)}
                      disabled={currentPage === totalPages}
                    >
                      Next
                    </Button>
                  </div>
                </div>
              )}
            </>
          )}
        </Card>
      </div>

      <SampleCreateModal
        isOpen={isCreateModalOpen}
        onClose={() => setIsCreateModalOpen(false)}
        onSuccess={handleCreateSuccess}
      />

      <SampleEditModal
        isOpen={isEditModalOpen}
        sampleId={selectedSampleId}
        onClose={() => {
          setIsEditModalOpen(false);
          setSelectedSampleId(null);
        }}
        onSuccess={handleEditSuccess}
      />

      <DeleteConfirmDialog
        isOpen={isDeleteDialogOpen}
        onClose={() => {
          setIsDeleteDialogOpen(false);
          setSampleToDelete(null);
        }}
        onConfirm={handleDeleteConfirm}
        title="Delete Sample"
        message="Are you sure you want to delete this sample? This will permanently remove all associated data."
        itemName={sampleToDelete?.sample_id}
      />
    </Layout>
  );
};

export default Samples;
