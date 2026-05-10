import { useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Button } from '../components/common/Button';
import { ErrorBanner } from '../components/common/ErrorBanner';
import { DeleteConfirmDialog } from '../components/common/DeleteConfirmDialog';
import { OrderCreateModal } from '../components/Orders/OrderCreateModal';
import { OrderEditModal } from '../components/Orders/OrderEditModal';
import { getOrders, deleteOrder } from '../services/orders';
import type { Order } from '../types/order';
import { ORDER_STATUSES, ORDER_PRIORITIES } from '../utils/constants';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { useEntityList } from '../hooks/useEntityList';

export const Orders = () => {
  useDocumentTitle({ module: 'Orders' });
  const [selectedStatus, setSelectedStatus] = useState<string>('');
  const [selectedPriority, setSelectedPriority] = useState<string>('');
  const [currentPage, setCurrentPage] = useState(1);
  const [isCreateModalOpen, setIsCreateModalOpen] = useState(false);
  const [isEditModalOpen, setIsEditModalOpen] = useState(false);
  const [selectedOrderId, setSelectedOrderId] = useState<string | null>(null);
  const [isDeleteDialogOpen, setIsDeleteDialogOpen] = useState(false);
  const [orderToDelete, setOrderToDelete] = useState<Order | null>(null);
  const itemsPerPage = 20;

  const { data: orders, total: totalOrders, loading, error, refetch } = useEntityList(
    () =>
      getOrders({
        status: selectedStatus || undefined,
        priority: selectedPriority || undefined,
        limit: itemsPerPage,
        offset: (currentPage - 1) * itemsPerPage,
      }).then((r) => ({ data: r.orders, total: r.total })),
    [selectedStatus, selectedPriority, currentPage]
  );

  const totalPages = Math.ceil(totalOrders / itemsPerPage);

  const handlePageChange = (page: number) => {
    setCurrentPage(page);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const handleCreateSuccess = (newOrder: Order) => {
    console.log('[Orders] Order created successfully:', newOrder);
    refetch();
  };

  const handleEditClick = (orderId: string) => {
    setSelectedOrderId(orderId);
    setIsEditModalOpen(true);
  };

  const handleEditSuccess = (updatedOrder: Order) => {
    console.log('[Orders] Order updated successfully:', updatedOrder);
    refetch();
  };

  const handleDeleteClick = (order: Order) => {
    setOrderToDelete(order);
    setIsDeleteDialogOpen(true);
  };

  const handleDeleteConfirm = async () => {
    if (!orderToDelete) return;
    console.log('[Orders] Deleting order:', orderToDelete.id);
    await deleteOrder(orderToDelete.order_id);
    console.log('[Orders] Order deleted successfully');
    refetch();
  };

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex justify-between items-center">
          <h2 className="text-3xl font-bold text-gray-900">Orders</h2>
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
                  onChange={(e) => {
                    setSelectedPriority(e.target.value);
                    setCurrentPage(1);
                  }}
                  className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
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

            <ErrorBanner message={error || null} />

            {loading ? (
              <div className="flex items-center justify-center py-12">
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
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Order ID
                      </th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Sample ID
                      </th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Test Type
                      </th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Status
                      </th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Priority
                      </th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Requested By
                      </th>
                      <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Requested Date
                      </th>
                      <th className="px-6 py-3 text-right text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                        Actions
                      </th>
                    </tr>
                  </thead>
                  <tbody className="bg-white">
                    {orders.map((order, idx) => (
                      <tr key={order.id} className={`hover:bg-[#F4F5F7] transition-colors duration-100 ${idx % 2 === 1 ? 'bg-[#FAFBFC]' : ''}`}>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono font-bold text-[#1A1C20] border-b border-[#E2E8F0]">
                          {order.order_id}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                          {order.sample_id}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-medium text-[#1A1C20] border-b border-[#E2E8F0]">
                          {order.test_type}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <span className={`px-2 py-1 text-[10px] font-bold uppercase tracking-wider border inline-block ${
                            order.status === 'REQUESTED' ? 'bg-blue-50 text-blue-700 border-blue-200' :
                            order.status === 'IN_PROGRESS' ? 'bg-yellow-50 text-yellow-800 border-yellow-200' :
                            order.status === 'COMPLETED' ? 'bg-green-50 text-green-700 border-green-200' :
                            order.status === 'VALIDATED' ? 'bg-green-50 text-green-700 border-green-200' :
                            order.status === 'CANCELLED' ? 'bg-red-50 text-red-700 border-red-200' :
                            'bg-gray-100 text-gray-600 border-gray-300'
                          }`}>
                            {order.status.replace('_', ' ')}
                          </span>
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                          <span className={`px-2 py-1 text-[10px] font-bold uppercase tracking-wider border inline-block ${
                            order.priority === 'EMERGENCY' ? 'bg-red-50 text-red-700 border-red-200' :
                            order.priority === 'URGENT' ? 'bg-orange-50 text-orange-700 border-orange-200' :
                            'bg-gray-100 text-gray-700 border-gray-300'
                          }`}>
                            {order.priority}
                          </span>
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm text-[#5E6C84] border-b border-[#E2E8F0]">
                          {order.requested_by}
                        </td>
                        <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                          {new Date(order.requested_date).toLocaleDateString('de-DE')}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-right text-sm font-medium">
                          <div className="flex items-center justify-end gap-2">
                            <Button
                              variant="secondary"
                              size="sm"
                              onClick={() => handleEditClick(order.order_id)}
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
                      {[...Array(totalPages)].map((_, i) => (
                        <button
                          key={i + 1}
                          onClick={() => handlePageChange(i + 1)}
                          className={`relative inline-flex items-center px-4 py-2 text-sm font-semibold ${
                            currentPage === i + 1
                              ? 'z-10 bg-blue-600 text-white focus:z-20'
                              : 'text-gray-900 ring-1 ring-inset ring-gray-300 hover:bg-gray-50 focus:z-20'
                          }`}
                        >
                          {i + 1}
                        </button>
                      ))}
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
        title="Delete Order"
        message="Are you sure you want to delete this order? This will permanently remove all associated data."
        itemName={orderToDelete?.order_id}
      />
    </Layout>
  );
};

export default Orders;
