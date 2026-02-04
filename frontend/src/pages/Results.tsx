import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { DeleteConfirmDialog } from '../components/common/DeleteConfirmDialog';
import { ResultCreateModal } from '../components/Results/ResultCreateModal';
import { ResultEditModal } from '../components/Results/ResultEditModal';
import { getResults, deleteResult } from '../services/results';
import type { TestResult } from '../types/result';
import { RESULT_STATUSES, RESULT_STATUS_COLORS, RESULT_FLAGS, RESULT_FLAG_COLORS } from '../utils/constants';

export const Results = () => {
  const [results, setResults] = useState<TestResult[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [selectedStatus, setSelectedStatus] = useState<string>('');
  const [selectedFlag, setSelectedFlag] = useState<string>('');
  const [currentPage, setCurrentPage] = useState(1);
  const [totalResults, setTotalResults] = useState(0);
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isEditModalOpen, setIsEditModalOpen] = useState(false);
  const [selectedResult, setSelectedResult] = useState<TestResult | null>(null);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [resultToDelete, setResultToDelete] = useState<TestResult | null>(null);
  const itemsPerPage = 20;

  useEffect(() => {
    const fetchResults = async () => {
      setLoading(true);
      try {
        const data = await getResults({
          status: selectedStatus || undefined,
          flag: selectedFlag || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setResults(data.results);
        setTotalResults(data.total);
        setError('');
      } catch (err) {
        setError('Failed to load results');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };

    fetchResults();
  }, [selectedStatus, selectedFlag, currentPage]);

  const handleCreateSuccess = (newResult: TestResult) => {
    console.log('[Results] Result created successfully:', newResult);
    // Refresh the results list
    const fetchResults = async () => {
      setLoading(true);
      try {
        const data = await getResults({
          status: selectedStatus || undefined,
          flag: selectedFlag || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setResults(data.results);
        setTotalResults(data.total);
      } catch (err) {
        setError('Failed to load results');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    fetchResults();
  };

  const handleEditClick = (result: TestResult) => {
    setSelectedResult(result);
    setIsEditModalOpen(true);
  };

  const handleEditSuccess = (updatedResult: TestResult) => {
    console.log('[Results] Result updated successfully:', updatedResult);
    // Refresh the results list
    const fetchResults = async () => {
      setLoading(true);
      try {
        const data = await getResults({
          status: selectedStatus || undefined,
          flag: selectedFlag || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setResults(data.results);
        setTotalResults(data.total);
      } catch (err) {
        setError('Failed to load results');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    fetchResults();
  };

  const handleDeleteClick = (result: TestResult) => {
    setResultToDelete(result);
    setIsDeleteDialogOpen(true);
  };

  const handleDeleteConfirm = async () => {
    if (!resultToDelete) return;

    console.log('[Results] Deleting result:', resultToDelete.id);
    await deleteResult(resultToDelete.id.toString());

    console.log('[Results] Result deleted successfully');
    // Refresh the results list
    const fetchResults = async () => {
      setLoading(true);
      try {
        const data = await getResults({
          status: selectedStatus || undefined,
          flag: selectedFlag || undefined,
          limit: itemsPerPage,
          offset: (currentPage - 1) * itemsPerPage,
        });
        setResults(data.results);
        setTotalResults(data.total);
      } catch (err) {
        setError('Failed to load results');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    await fetchResults();
  };

  const totalPages = Math.ceil(totalResults / itemsPerPage);

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex justify-between items-center">
          <h2 className="text-3xl font-bold text-gray-900">Test Results</h2>
          <Button onClick={() => setIsCreateModalOpen(true)}>
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
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
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
                  className="w-full px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
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

            {error && (
              <div className="bg-red-50 border border-red-200 rounded-lg p-4 mb-6">
                <p className="text-red-800">{error}</p>
              </div>
            )}

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
                <table className="min-w-full divide-y divide-gray-200">
                  <thead className="bg-gray-50">
                    <tr>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Result ID</th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Order ID</th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Parameter</th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Value</th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Reference</th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Flag</th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase">Status</th>
                      <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase">Actions</th>
                    </tr>
                  </thead>
                  <tbody className="bg-white divide-y divide-gray-200">
                    {results.map((result) => (
                      <tr key={result.id} className="hover:bg-gray-50 transition-colors">
                        <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                          {result.result_id}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{result.order_id}</td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{result.parameter}</td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                          {result.value} {result.unit}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                          {result.reference_min} - {result.reference_max}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap">
                          <span className={`px-2 inline-flex text-xs leading-5 font-semibold rounded-full ${RESULT_FLAG_COLORS[result.flag]}`}>
                            {RESULT_FLAGS[result.flag]}
                          </span>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap">
                          <span className={`px-2 inline-flex text-xs leading-5 font-semibold rounded-full ${RESULT_STATUS_COLORS[result.status]}`}>
                            {RESULT_STATUSES[result.status]}
                          </span>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-right text-sm font-medium">
                          <div className="flex items-center justify-end gap-2">
                            <Button
                              variant="secondary"
                              size="sm"
                              onClick={() => handleEditClick(result)}
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
                          </div>
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
                      onClick={() => setCurrentPage(currentPage - 1)}
                      disabled={currentPage === 1}
                      className="relative inline-flex items-center rounded-l-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 disabled:opacity-50 disabled:cursor-not-allowed"
                    >
                      Previous
                    </button>
                    {[...Array(totalPages)].map((_, i) => (
                      <button
                        key={i + 1}
                        onClick={() => setCurrentPage(i + 1)}
                        className={`relative inline-flex items-center px-4 py-2 text-sm font-semibold ${
                          currentPage === i + 1
                            ? 'z-10 bg-blue-600 text-white'
                            : 'text-gray-900 ring-1 ring-inset ring-gray-300 hover:bg-gray-50'
                        }`}
                      >
                        {i + 1}
                      </button>
                    ))}
                    <button
                      onClick={() => setCurrentPage(currentPage + 1)}
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
