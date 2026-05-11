// Dashboard statistics types
export interface StatusCount {
  status: string;
  count: number;
}

export interface EntityStats {
  entity_type: string;
  total: number;
  by_status: StatusCount[];
}

export interface DashboardStats {
  samples: EntityStats;
  orders: EntityStats;
  results: EntityStats;
  order_priority?: StatusCount[];
  critical_count?: number;
}

export interface StatsFilter {
  from?: number;
  to?: number;
}
