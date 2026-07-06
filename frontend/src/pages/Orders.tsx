import { useState } from 'react';
import { Link } from 'react-router-dom';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { ErrorBanner } from '../components/common/ErrorBanner';
import { StatusBadge } from '../components/common/StatusBadge';
import { DeleteConfirmDialog } from '../components/common/DeleteConfirmDialog';
import { OrderCreateModal } from '../components/Orders/OrderCreateModal';
import { OrderEditModal } from '../components/Orders/OrderEditModal';
import { getOrders, deleteOrder } from '../services/orders';
import type { Order } from '../types/order';
import { ORDER_STATUSES, ORDER_PRIORITIES } from '../utils/constants';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { useAuth } from '../context/AuthContext';
import { useEntityList } from '../hooks/useEntityList';
import { useToast } from '../hooks/useToast';
import { useListParams } from '../hooks/useListParams';

export const Orders = () => {
  useDocumentTitle({ module: 'Orders' });
  const { user } = useAuth();
  const toast = useToast();
  const canWrite = user?.role === 'ADMIN' || user?.role === 'OPERATOR';
  const { get, page: currentPage, setParam, setPage } = useListParams();
  const searchQuery = get('q');
  const selectedStatus = get('status');
  const selectedPriority = get('priority');

  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isEditModalOpen, setIsEditModalOpen] = useState(false);
  const [selectedOrderId, setSelectedOrderId] = useState<string | null>(null);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [orderToDelete, setOrderToDelete] = useState<Order | null>(null);
  const itemsPerPage = 20;

  const { data: orders, total: totalOrders, loading, error, refetch } = useEntityList(
    () =>
      getOrders({
        q: searchQuery || undefined,
        status: selectedStatus || undefined,
        priority: selectedPriority || undefined,
        limit: itemsPerPage,
        offset: (currentPage - 1) * itemsPerPage,
      }).then((r) => ({ data: r.orders, total: r.total })),
    [searchQuery, selectedStatus, selectedPriority, currentPage]
  );

  const totalPages = Math.ceil(totalOrders / itemsPerPage);

  const handlePageChange = (page: number) => {
    setPage(page);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const handleCreateSuccess = (newOrder: Order) => {
    toast.success(`Order ${newOrder.order_id} created`);
    refetch();
  };

  const handleEditClick = (orderId: string) => {
    setSelectedOrderId(orderId);
    setIsEditModalOpen(true);
  };

  const handleEditSuccess = (updatedOrder: Order) => {
    toast.success(`Order ${updatedOrder.order_id} updated`);
    refetch();
  };

  const handleDeleteClick = (order: Order) => {
    setOrderToDelete(order);
    setIsDeleteDialogOpen(true);
  };

  // Errors propagate to DeleteConfirmDialog (shown inline; dialog stays open).
  const handleDeleteConfirm = async () => {
    if (!orderToDelete) return;
    await deleteOrder(orderToDelete.order_id);
    toast.success(`Order ${orderToDelete.order_id} cancelled`);
    refetch();
  };

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex flex-wrap justify-between items-center gap-3">
          <h2 className="text-2xl md:text-3xl font-bold text-gray-900">Orders</h2>
          {canWrite && (
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
              Create Order
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
                  onChange={(e) => setParam('status', e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
                >
                  <option value="">All Statuses</option>
                  {Object.entries(ORDER_STATUSES).map(([key, label]) => (
                    <option key={key} value={key}>
                      {label}
                    </option>
                  ))}
                </select>
              </div>

              <div className="flex-1">
                <label className="block text-sm font-medium text-gray-700 mb-2">
                  Filter by Priority
                </label>
                <select
                  value={selectedPriority}
                  onChange={(e) => setParam('priority', e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
                >
                  <option value="">All Priorities</option>
                  {Object.entries(ORDER_PRIORITIES).map(([key, label]) => (
                    <option key={key} value={key}>
                      {label}
                    </option>
                  ))}
                </select>
              </div>
            </div>

            <ErrorBanner message={error || null} onRetry={error ? refetch : undefined} />

            {loading ? (
              <div role="status" aria-live="polite" className="flex items-center justify-center py-12">
                <div className="text-center">
                  <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
                  <p className="mt-4 text-gray-600">Loading orders...</p>
                </div>
              </div>
            ) : orders.length === 0 ? (
              <div className="text-center py-12">
                <p className="text-gray-500 text-lg">No orders found</p>
                <p className="text-gray-400 mt-2">Create your first order to get started</p>
              </div>
            ) : (
              <div className="overflow-x-auto">
                <table className="min-w-full">
                  <thead className="bg-[#F4F5F7]">
                    <tr>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Order ID
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Sample ID
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Test Type
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Status
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Priority
                      </th>
                      <th className="hidden md:table-cell px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Requested By
                      </th>
                      <th className="px-3 md:px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Requested Date
                      </th>
                      <th className="px-3 md:px-6 py-3 text-right text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Actions
                      </th>
                    </tr>
                  </thead>
                  <tbody className="bg-white">
                    {orders.map((order, idx) => (
                      <tr key={order.id} className={`hover:bg-[#F4F5F7] transition-colors duration-100 ${idx % 2 === 1 ? 'bg-[#FAFBFC]' : ''}`}>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-mono font-bold text-[#1A1C20] border-b border-[#E2E8F0]">
                          {order.order_id}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-mono border-b border-[#E2E8F0]">
                          <Link
                            to={`/samples?q=${encodeURIComponent(order.sample_id)}`}
                            className="text-[#0055FF] hover:underline"
                            title={`View sample ${order.sample_id}`}
                          >
                            {order.sample_id}
                          </Link>
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-medium text-[#1A1C20] border-b border-[#E2E8F0]">
                          {order.test_type}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <StatusBadge colorClass={
                            order.status === 'REQUESTED' ? 'bg-blue-50 text-blue-700 border-blue-200' :
                            order.status === 'IN_PROGRESS' ? 'bg-yellow-50 text-yellow-800 border-yellow-200' :
                            order.status === 'COMPLETED' || order.status === 'VALIDATED' ? 'bg-green-50 text-green-700 border-green-200' :
                            order.status === 'CANCELLED' ? 'bg-red-50 text-red-700 border-red-200' :
                            'bg-gray-100 text-gray-600 border-gray-300'
                          }>
                            {order.status.replace('_', ' ')}
                          </StatusBadge>
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <StatusBadge colorClass={
                            order.priority === 'EMERGENCY' ? 'bg-red-50 text-red-700 border-red-200' :
                            order.priority === 'URGENT' ? 'bg-orange-50 text-orange-700 border-orange-200' :
                            'bg-gray-100 text-gray-700 border-gray-300'
                          }>
                            {order.priority}
                          </StatusBadge>
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm text-[#5E6C84] border-b border-[#E2E8F0]">
                          {order.requested_by}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                          {new Date(order.requested_date).toLocaleDateString('de-DE')}
                        </td>
                        <td className="px-3 md:px-6 py-2 md:py-4 whitespace-nowrap text-right text-sm font-medium">
                          {canWrite && <div className="flex items-center justify-end gap-2">
                            <Button
                              variant="secondary"
                              size="sm"
                              onClick={() => handleEditClick(order.order_id)}
                              disabled={order.status === 'VALIDATED' || order.status === 'CANCELLED'}
                              title={
                                order.status === 'VALIDATED' ? 'Validated orders are immutable (ISO 15189)' :
                                order.status === 'CANCELLED' ? 'Cancelled orders cannot be edited' :
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
                              onClick={() => handleDeleteClick(order)}
                              disabled={order.status === 'CANCELLED' || order.status === 'VALIDATED' || order.status === 'COMPLETED'}
                              title={
                                order.status === 'CANCELLED' ? 'Order is already cancelled' :
                                order.status === 'VALIDATED' ? 'Validated orders cannot be cancelled' :
                                order.status === 'COMPLETED' ? 'Completed orders cannot be cancelled directly' :
                                'Cancel order'
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
            )}

            {totalPages > 1 && (
              <div className="flex items-center justify-between border-t border-gray-200 bg-white px-4 py-3 sm:px-6 mt-6">
                <div className="flex flex-1 justify-between sm:hidden">
                  <Button
                    onClick={() => handlePageChange(currentPage - 1)}
                    disabled={currentPage === 1}
                    variant="secondary"
                  >
                    Previous
                  </Button>
                  <Button
                    onClick={() => handlePageChange(currentPage + 1)}
                    disabled={currentPage === totalPages}
                    variant="secondary"
                  >
                    Next
                  </Button>
                </div>
                <div className="hidden sm:flex sm:flex-1 sm:items-center sm:justify-between">
                  <div>
                    <p className="text-sm text-gray-700">
                      Showing{' '}
                      <span className="font-medium">{(currentPage - 1) * itemsPerPage + 1}</span>{' '}
                      to{' '}
                      <span className="font-medium">
                        {Math.min(currentPage * itemsPerPage, totalOrders)}
                      </span>{' '}
                      of <span className="font-medium">{totalOrders}</span> results
                    </p>
                  </div>
                  <div>
                    <nav className="isolate inline-flex -space-x-px rounded-md shadow-sm">
                      <button
                        onClick={() => handlePageChange(currentPage - 1)}
                        disabled={currentPage === 1}
                        className="relative inline-flex items-center rounded-l-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:z-20 disabled:opacity-50 disabled:cursor-not-allowed"
                      >
                        Previous
                      </button>
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
                        <button
                          key={pageNum}
                          onClick={() => handlePageChange(pageNum)}
                          className={`relative inline-flex items-center px-4 py-2 text-sm font-semibold ${
                            currentPage === pageNum
                              ? 'z-10 bg-blue-600 text-white focus:z-20'
                              : 'text-gray-900 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:z-20'
                          }`}
                        >
                          {pageNum}
                        </button>
                      );
                    })}
                      <button
                        onClick={() => handlePageChange(currentPage + 1)}
                        disabled={currentPage === totalPages}
                        className="relative inline-flex items-center rounded-r-md px-2 py-2 text-gray-400 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:z-20 disabled:opacity-50 disabled:cursor-not-allowed"
                      >
                        Next
                      </button>
                    </nav>
                  </div>
                </div>
              </div>
            )}
          </div>
        </Card>
      </div>

      <OrderCreateModal
        isOpen={isCreateModalOpen}
        onClose={() => setIsCreateModalOpen(false)}
        onSuccess={handleCreateSuccess}
      />

      <OrderEditModal
        isOpen={isEditModalOpen}
        orderId={selectedOrderId}
        onClose={() => {
          setIsEditModalOpen(false);
          setSelectedOrderId(null);
        }}
        onSuccess={handleEditSuccess}
      />

      <DeleteConfirmDialog
        isOpen={isDeleteDialogOpen}
        onClose={() => {
          setIsDeleteDialogOpen(false);
          setOrderToDelete(null);
        }}
        onConfirm={handleDeleteConfirm}
        title="Cancel Order"
        message="Cancel this order?"
        itemName={orderToDelete?.order_id}
        confirmText="Cancel Order"
        cancelText="Keep"
        outcomeNote="The order is marked CANCELLED (soft-delete). Its audit history is retained."
      />
    </Layout>
  );
};

export default Orders;
