import { useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { ErrorBanner } from '../components/common/ErrorBanner';
import { DeleteConfirmDialog } from '../components/common/DeleteConfirmDialog';
import { ResultCreateModal } from '../components/Results/ResultCreateModal';
import { ResultEditModal } from '../components/Results/ResultEditModal';
import { getResults, deleteResult } from '../services/results';
import type { TestResult } from '../types/result';
import { RESULT_STATUSES, RESULT_FLAGS } from '../utils/constants';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { useAuth } from '../context/AuthContext';
import { useEntityList } from '../hooks/useEntityList';

export const Results = () => {
  useDocumentTitle({ module: 'Test Results' });
  const { user } = useAuth();
  const canWrite = user?.role === 'ADMIN' || user?.role === 'OPERATOR';
  const [selectedStatus, setSelectedStatus] = useState<string>('');
  const [selectedFlag, setSelectedFlag] = useState<string>('');
  const [currentPage, setCurrentPage] = useState(1);
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isEditModalOpen, setIsEditModalOpen] = useState(false);
  const [selectedResult, setSelectedResult] = useState<TestResult | null>(null);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [resultToDelete, setResultToDelete] = useState<TestResult | null>(null);
  const itemsPerPage = 20;

  const { data: results, total: totalResults, loading, error, refetch } = useEntityList(
    () =>
      getResults({
        status: selectedStatus || undefined,
        flag: selectedFlag || undefined,
        limit: itemsPerPage,
        offset: (currentPage - 1) * itemsPerPage,
      }).then((r) => ({ data: r.results, total: r.total })),
    [selectedStatus, selectedFlag, currentPage]
  );

  const totalPages = Math.ceil(totalResults / itemsPerPage);

  const handlePageChange = (page: number) => {
    setCurrentPage(page);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const handleCreateSuccess = (_newResult: TestResult) => {
    refetch();
  };

  const handleEditClick = (result: TestResult) => {
    setSelectedResult(result);
    setIsEditModalOpen(true);
  };

  const handleEditSuccess = (_updatedResult: TestResult) => {
    refetch();
  };

  const handleDeleteClick = (result: TestResult) => {
    setResultToDelete(result);
    setIsDeleteDialogOpen(true);
  };

  const handleDeleteConfirm = async () => {
    if (!resultToDelete) return;
    await deleteResult(resultToDelete.result_id);
    refetch();
  };

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex justify-between items-center">
          <h2 className="text-3xl font-bold text-gray-900">Test Results</h2>
          {canWrite && (          <Button onClick={() => setIsCreateModalOpen(true)}>
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
              Create Result
            </span>
          </Button>
          )}
        </div>

        <Card>
          <div className="p-6">
            <div className="flex gap-4 mb-6">
              <div className="flex-1">
                <label className="block text-sm font-medium text-gray-700 mb-2">
                  Filter by Status
                </label>
                <select
                  value={selectedStatus}
                  onChange={(e) => {
                    setSelectedStatus(e.target.value);
                    setCurrentPage(1);
                  }}
                  className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                >
                  <option value="">All Statuses</option>
                  {Object.entries(RESULT_STATUSES).map(([key, label]) => (
                    <option key={key} value={key}>
                      {label}
                    </option>
                  ))}
                </select>
              </div>

              <div className="flex-1">
                <label className="block text-sm font-medium text-gray-700 mb-2">
                  Filter by Flag
                </label>
                <select
                  value={selectedFlag}
                  onChange={(e) => {
                    setSelectedFlag(e.target.value);
                    setCurrentPage(1);
                  }}
                  className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                >
                  <option value="">All Flags</option>
                  {Object.entries(RESULT_FLAGS).map(([key, label]) => (
                    <option key={key} value={key}>
                      {label}
                    </option>
                  ))}
                </select>
              </div>
            </div>

            <ErrorBanner message={error || null} />

            {loading ? (
              <div className="flex items-center justify-center py-12">
                <div className="text-center">
                  <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
                  <p className="mt-4 text-gray-600">Loading results...</p>
                </div>
              </div>
            ) : results.length === 0 ? (
              <div className="text-center py-12">
                <p className="text-gray-500 text-lg">No results found</p>
              </div>
            ) : (
              <div className="overflow-x-auto">
                <table className="min-w-full">
                  <thead className="bg-[#F4F5F7]">
                    <tr>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Result ID</th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Order ID</th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Parameter</th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Value</th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Reference</th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Flag</th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Status</th>
                      <th className="px-6 py-3 text-right text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Actions</th>
                    </tr>
                  </thead>
                  <tbody className="bg-white">
                    {results.map((result, idx) => (
                      <tr key={result.id} className={`hover:bg-[#F4F5F7] transition-colors duration-100 ${idx % 2 === 1 ? 'bg-[#FAFBFC]' : ''}`}>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono font-bold text-[#1A1C20] border-b border-[#E2E8F0]">
                          {result.result_id}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">{result.order_id}</td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-medium text-[#1A1C20] border-b border-[#E2E8F0]">{result.parameter}</td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono text-[#1A1C20] border-b border-[#E2E8F0]">
                          {result.value} {result.unit}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                          {result.reference_min} - {result.reference_max}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <span className={`px-2 py-1 text-[10px] font-bold uppercase tracking-wider border inline-block ${
                            result.flag === 'CRITICAL' ? 'bg-red-50 text-red-700 border-red-200' :
                            result.flag === 'HIGH' ? 'bg-orange-50 text-orange-700 border-orange-200' :
                            result.flag === 'LOW' ? 'bg-yellow-50 text-yellow-800 border-yellow-200' :
                            result.flag === 'UNDEFINED' ? 'bg-gray-100 text-gray-600 border-gray-300' :
                            'bg-green-50 text-green-700 border-green-200'
                          }`}>
                            {RESULT_FLAGS[result.flag]}
                          </span>
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <span className={`px-2 py-1 text-[10px] font-bold uppercase tracking-wider border inline-block ${
                            result.status === 'PENDING' ? 'bg-yellow-50 text-yellow-800 border-yellow-200' :
                            result.status === 'VALIDATED' ? 'bg-green-50 text-green-700 border-green-200' :
                            result.status === 'REVIEWED' ? 'bg-blue-50 text-blue-700 border-blue-200' :
                            result.status === 'REJECTED' ? 'bg-red-50 text-red-700 border-red-200' :
                            'bg-gray-100 text-gray-600 border-gray-300'
                          }`}>
                            {RESULT_STATUSES[result.status]}
                          </span>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-right text-sm font-medium border-b border-[#E2E8F0]">
                          {canWrite && <div className="flex items-center justify-end gap-2">
                            <Button
                              variant="secondary"
                              size="sm"
                              onClick={() => handleEditClick(result)}
                              disabled={result.status === 'VALIDATED'}
                              title={result.status === 'VALIDATED' ? 'Validated results cannot be edited' : undefined}
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
                              onClick={() => handleDeleteClick(result)}
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
            )}

            {totalPages > 1 && (
              <div className="flex items-center justify-between border-t border-gray-200 bg-white px-4 py-3 sm:px-6 mt-6">
                <div>
                  <p className="text-sm text-gray-700">
                    Showing <span className="font-medium">{(currentPage - 1) * itemsPerPage + 1}</span> to{' '}
                    <span className="font-medium">{Math.min(currentPage * itemsPerPage, totalResults)}</span> of{' '}
                    <span className="font-medium">{totalResults}</span> results
                  </p>
                </div>
                <div>
                  <nav className="isolate inline-flex -space-x-px rounded-md shadow-sm">
                    <button
                      onClick={() => handlePageChange(currentPage - 1)}
                      disabled={currentPage === 1}
                      className="relative inline-flex items-center rounded-l-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 disabled:opacity-50 disabled:cursor-not-allowed"
                    >
                      Previous
                    </button>
                    {Array.from({ length: Math.min(5, totalPages) }, (_, i) => {
                      let pageNum;
                      if (totalPages <= 5) { pageNum = i + 1; }
                      else if (currentPage <= 3) { pageNum = i + 1; }
                      else if (currentPage >= totalPages - 2) { pageNum = totalPages - 4 + i; }
                      else { pageNum = currentPage - 2 + i; }
                      return (
                        <button key={pageNum} onClick={() => handlePageChange(pageNum)}
                          className={`relative inline-flex items-center px-4 py-2 text-sm font-semibold ${
                            currentPage === pageNum ? 'z-10 bg-blue-600 text-white' : 'text-gray-900 ring-1 ring-inset ring-gray-300 hover:bg-gray-50'
                          }`}
                        >{pageNum}</button>
                      );
                    })}
                    <button
                      onClick={() => handlePageChange(currentPage + 1)}
                      disabled={currentPage === totalPages}
                      className="relative inline-flex items-center rounded-r-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 disabled:opacity-50 disabled:cursor-not-allowed"
                    >
                      Next
                    </button>
                  </nav>
                </div>
              </div>
            )}
          </div>
        </Card>
      </div>

      <ResultCreateModal
        isOpen={isCreateModalOpen}
        onClose={() => setIsCreateModalOpen(false)}
        onSuccess={handleCreateSuccess}
      />

      <ResultEditModal
        isOpen={isEditModalOpen}
        onClose={() => {
          setIsEditModalOpen(false);
          setSelectedResult(null);
        }}
        result={selectedResult}
        onSuccess={handleEditSuccess}
      />

      <DeleteConfirmDialog
        isOpen={isDeleteDialogOpen}
        onClose={() => {
          setIsDeleteDialogOpen(false);
          setResultToDelete(null);
        }}
        onConfirm={handleDeleteConfirm}
        title="Delete Test Result"
        message="Are you sure you want to delete this test result? This will permanently remove all associated data."
        itemName={resultToDelete?.result_id}
      />
    </Layout>
  );
};

export default Results;
