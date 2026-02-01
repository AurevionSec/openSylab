export interface TestResult {
  id: number;
  result_id: string;
  order_id: string;
  parameter: string;
  value: string;
  unit: string;
  reference_min: string;
  reference_max: string;
  flag: ResultFlag;
  status: ResultStatus;
  reviewed_by: string;
  reviewed_date: string;
  notes: string;
}

export type ResultFlag =
  | 'NORMAL'
  | 'LOW'
  | 'HIGH'
  | 'CRITICAL';

export type ResultStatus =
  | 'PENDING'
  | 'REVIEWED'
  | 'VALIDATED'
  | 'REJECTED'
  | 'AMENDED';

export interface ResultFilter {
  status?: string;
  order_id?: string;
  flag?: string;
  limit?: number;
  offset?: number;
}

export interface ResultListResponse {
  results: TestResult[];
  total: number;
  limit: number;
  offset: number;
}
