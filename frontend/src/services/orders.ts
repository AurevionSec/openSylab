import api from './api';
import type { Order, OrderFilter, OrderListResponse } from '../types/order';

// Map backend status strings to frontend enum values
const mapStatus = (backendStatus: string): Order['status'] => {
  const statusMap: Record<string, Order['status']> = {
    'Angefordert': 'REQUESTED',
    'In Bearbeitung': 'IN_PROGRESS',
    'Abgeschlossen': 'COMPLETED',
    'Validiert': 'VALIDATED',
    'Storniert': 'CANCELLED',
    'REQUESTED': 'REQUESTED',
    'IN_PROGRESS': 'IN_PROGRESS',
    'COMPLETED': 'COMPLETED',
    'VALIDATED': 'VALIDATED',
    'CANCELLED': 'CANCELLED',
  };
  return statusMap[backendStatus] || 'REQUESTED';
};

// Map backend priority strings to frontend enum values
const mapPriority = (backendPriority: string): Order['priority'] => {
  const priorityMap: Record<string, Order['priority']> = {
    'Normal': 'NORMAL',
    'Dringend': 'URGENT',
    'Notfall': 'EMERGENCY',
    'NORMAL': 'NORMAL',
    'URGENT': 'URGENT',
    'EMERGENCY': 'EMERGENCY',
  };
  return priorityMap[backendPriority] || 'NORMAL';
};

// Transform backend order to frontend order
interface BackendOrder {
  id: number;
  order_id: string;
  sample_id: string;
  test_type: string;
  status: string;
  priority: string;
  requested_date: number;
  completed_date?: number;
  requested_by?: string;
  notes?: string;
}

const transformOrder = (backendOrder: BackendOrder): Order => {
  return {
    id: backendOrder.id,
    order_id: backendOrder.order_id,
    sample_id: backendOrder.sample_id,
    test_type: backendOrder.test_type || '',
    status: mapStatus(backendOrder.status),
    priority: mapPriority(backendOrder.priority),
    requested_date: new Date(backendOrder.requested_date * 1000).toISOString(),
    completed_date: backendOrder.completed_date
      ? new Date(backendOrder.completed_date * 1000).toISOString()
      : '',
    requested_by: backendOrder.requested_by || '',
    notes: backendOrder.notes || '',
  };
};

export const getOrders = async (filters?: OrderFilter): Promise<OrderListResponse> => {
  const response = await api.get<{ data: BackendOrder[]; total: number }>('/orders', {
    params: filters,
  });

  // Transform backend response to frontend format
  const orders = response.data.data.map(transformOrder);

  return {
    orders,
    total: response.data.total ?? orders.length,
    limit: filters?.limit || orders.length,
    offset: filters?.offset || 0,
  };
};

export const getOrderById = async (id: string): Promise<Order> => {
  const response = await api.get<{ data: BackendOrder }>(`/orders/${id}`);
  return transformOrder(response.data.data);
};

export const createOrder = async (order: Omit<Order, 'id' | 'requested_date' | 'completed_date'>): Promise<Order> => {
  const response = await api.post<{ data: BackendOrder }>('/orders', order);
  return transformOrder(response.data.data);
};

export const updateOrder = async (id: string, order: Partial<Order>): Promise<Order> => {
  const response = await api.put<{ data: BackendOrder }>(`/orders/${id}`, order);
  return transformOrder(response.data.data);
};

export const deleteOrder = async (id: string): Promise<void> => {
  await api.delete(`/orders/${id}`);
};
