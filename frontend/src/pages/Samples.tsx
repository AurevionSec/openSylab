import { useState, useEffect } from 'react';
import { useLocation } from 'react-router-dom';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { ErrorBanner } from '../components/common/ErrorBanner';
import { DeleteConfirmDialog } from '../components/common/DeleteConfirmDialog';
import { SampleCreateModal } from '../components/Samples/SampleCreateModal';
import { SampleEditModal } from '../components/Samples/SampleEditModal';
import { getSamples, deleteSample } from '../services/samples';
import type { Sample } from '../types/sample';
import { SAMPLE_STATUSES } from '../utils/constants';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { useAuth } from '../context/AuthContext';
import { useEntityList } from '../hooks/useEntityList';
import { useToast } from '../hooks/useToast';

export const Samples = () => {
  useDocumentTitle({ module: 'Samples' });
  const { user } = useAuth();
  const toast = useToast();
  const canWrite = user?.role === 'ADMIN' || user?.role === 'OPERATOR';
  const location = useLocation();
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [selectedStatus, setSelectedStatus] = useState<string>('');
  const [currentPage, setCurrentPage] = useState(1);

  // Read ?q= from URL on mount and when URL changes
  useEffect(() => {
    const params = new URLSearchParams(location.search);
    const q = params.get('q') || '';
    setSearchQuery(q);
    setCurrentPage(1);
  }, [location.search]);
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isEditModalOpen, setIsEditModalOpen] = useState(false);
  const [selectedSampleId, setSelectedSampleId] = useState<string | null>(null);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [sampleToDelete, setSampleToDelete] = useState<Sample | null>(null);
  const itemsPerPage = 20;

  const { data: samples, total: totalSamples, loading, error, refetch } = useEntityList(
    () =>
      getSamples({
        q: searchQuery || undefined,
        status: selectedStatus || undefined,
        limit: itemsPerPage,
        offset: (currentPage - 1) * itemsPerPage,
      }).then((r) => ({ data: r.samples, total: r.total })),
    [searchQuery, selectedStatus, currentPage]
  );

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
    toast.success(`Sample ${newSample.sample_id} created`);
    refetch();
  };

  const handleEditClick = (sampleId: string) => {
    setSelectedSampleId(sampleId);
    setIsEditModalOpen(true);
  };

  const handleEditSuccess = (updatedSample: Sample) => {
    toast.success(`Sample ${updatedSample.sample_id} updated`);
    refetch();
  };

  const handleDeleteClick = (sample: Sample) => {
    setSampleToDelete(sample);
    setIsDeleteDialogOpen(true);
  };

  // Errors propagate to DeleteConfirmDialog, which shows them inline and keeps
  // itself open; on success the dialog calls onClose (which clears the state).
  const handleDeleteConfirm = async () => {
    if (!sampleToDelete) return;
    await deleteSample(sampleToDelete.sample_id);
    toast.success(`Sample ${sampleToDelete.sample_id} archived`);
    refetch();
  };

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex flex-wrap justify-between items-center gap-3">
          <div>
            <h2 className="text-2xl md:text-3xl font-bold text-gray-900">Samples</h2>
            <p className="text-gray-600 mt-1">Manage and view all laboratory samples</p>
          </div>
          {canWrite && (
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
          )}
        </div>

        <Card>
          {searchQuery && (
            <div className="mb-4 flex items-center gap-2 text-sm text-blue-700 bg-blue-50 border border-blue-200 rounded px-3 py-2">
              <svg className="w-4 h-4 shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"/></svg>
              <span>Suche: <strong>{searchQuery}</strong></span>
              <button className="ml-auto text-blue-500 hover:text-blue-700" onClick={() => { setSearchQuery(''); setCurrentPage(1); }}>✕</button>
            </div>
          )}
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

          <ErrorBanner message={error || null} onRetry={error ? refetch : undefined} />

          {loading ? (
            <div role="status" aria-live="polite" className="flex items-center justify-center py-12">
              <div className="text-center">
                <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
                <p className="mt-4 text-gray-600">Loading samples...</p>
              </div>
            </div>
          ) : samples.length === 0 ? (
            <div className="text-center py-12">
              <p className="text-gray-500 text-lg">No samples found</p>
              <p className="text-gray-400 mt-2">Try adjusting your filters or create a new sample</p>
            </div>
          ) : (
            <>
              <div className="overflow-x-auto">
                <table className="min-w-full">
                  <thead className="bg-[#F4F5F7]">
                    <tr>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Sample ID
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Patient ID
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Patient Name
                      </th>
                      <th className="hidden md:table-cell px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Description
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Status
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Created
                      </th>
                      <th className="px-3 md:px-6 py-3 text-right text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Actions
                      </th>
                    </tr>
                  </thead>
                  <tbody className="bg-white">
                    {samples.map((sample, idx) => (
                      <tr key={sample.id} className={`hover:bg-[#F4F5F7] transition-colors duration-100 ${idx % 2 === 1 ? 'bg-[#FAFBFC]' : ''}`}>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-mono font-bold text-[#1A1C20] border-b border-[#E2E8F0]">
                          {sample.sample_id}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                          {sample.patient_id}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-medium text-[#1A1C20] border-b border-[#E2E8F0]">
                          {sample.patient_name}
                        </td>
                        <td className="hidden md:table-cell px-3 md:px-6 py-2 md:py-2.5 text-sm text-[#5E6C84] max-w-xs truncate border-b border-[#E2E8F0]">
                          {sample.description}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <span className={`px-2 py-1 text-[10px] font-bold uppercase tracking-wider border inline-block ${
                            sample.status === 'REGISTERED' ? 'bg-blue-50 text-blue-700 border-blue-200' :
                            sample.status === 'IN_ANALYSIS' ? 'bg-yellow-50 text-yellow-800 border-yellow-200' :
                            sample.status === 'ANALYZED' ? 'bg-gray-100 text-gray-700 border-gray-300' :
                            sample.status === 'VALIDATED' ? 'bg-green-50 text-green-700 border-green-200' :
                            sample.status === 'ARCHIVED' ? 'bg-gray-100 text-gray-600 border-gray-300' :
                            'bg-gray-100 text-gray-600 border-gray-300'
                          }`}>
                            {sample.status.replace('_', ' ')}
                          </span>
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                          {new Date(sample.created_at).toLocaleDateString('de-DE')}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-4 whitespace-nowrap text-right text-sm font-medium">
                          {canWrite && <div className="flex items-center justify-end gap-2">
                            <Button
                              variant="secondary"
                              size="sm"
                              onClick={() => handleEditClick(sample.sample_id)}
                              disabled={sample.status === 'VALIDATED' || sample.status === 'ARCHIVED'}
                              title={
                                sample.status === 'VALIDATED' ? 'Validated samples are immutable (ISO 15189)' :
                                sample.status === 'ARCHIVED' ? 'Archived samples are read-only' :
                                undefined
                              }
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
                              variant="ghost"
                              size="sm"
                              onClick={() => handleDeleteClick(sample)}
                              disabled={sample.status === 'ARCHIVED' || sample.status === 'VALIDATED' || sample.status === 'IN_ANALYSIS' || sample.status === 'ANALYZED'}
                              title={
                                sample.status === 'ARCHIVED' ? 'Archived samples cannot be deleted' :
                                sample.status === 'VALIDATED' ? 'Validated samples are immutable (ISO 15189)' :
                                sample.status === 'IN_ANALYSIS' ? 'Sample is currently in analysis' :
                                sample.status === 'ANALYZED' ? 'Analyzed samples must be validated or archived first' :
                                'Delete sample'
                              }
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
                          </div>}
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
        title="Archive Sample"
        message="Archive this sample? It will be hidden from active lists."
        itemName={sampleToDelete?.sample_id}
        confirmText="Archive"
        outcomeNote="The sample is marked ARCHIVED (soft-delete). Its audit history is retained."
      />
    </Layout>
  );
};

export default Samples;
