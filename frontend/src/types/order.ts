export interface Order {
  id: number;
  order_id: string;
  sample_id: string;
  test_type: string;
  status: OrderStatus;
  priority: OrderPriority;
  requested_date: string;
  completed_date: string;
  requested_by: string;
  notes: string;
}

export type OrderStatus =
  | 'REQUESTED'
  | 'IN_PROGRESS'
  | 'COMPLETED'
  | 'VALIDATED'
  | 'CANCELLED';

export type OrderPriority =
  | 'NORMAL'
  | 'URGENT'
  | 'EMERGENCY';

export interface OrderFilter {
  q?: string;
  status?: string;
  sample_id?: string;
  priority?: string;
  limit?: number;
  offset?: number;
}

export interface OrderListResponse {
  orders: Order[];
  total: number;
  limit: number;
  offset: number;
}
