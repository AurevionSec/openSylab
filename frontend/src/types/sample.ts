export interface Sample {
  id: number;
  sample_id: string;
  patient_id: string;
  patient_name: string;
  description: string;
  status: 'REGISTERED' | 'IN_ANALYSIS' | 'ANALYZED' | 'VALIDATED' | 'ARCHIVED';
  created_at: string;
  updated_at: string;
}

export interface SampleFilter {
  q?: string;
  status?: string;
  from?: string;
  to?: string;
  limit?: number;
  offset?: number;
}

export interface SampleListResponse {
  samples: Sample[];
  total: number;
  limit: number;
  offset: number;
}
