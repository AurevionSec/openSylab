import api from './api';
import type { Sample, SampleFilter, SampleListResponse } from '../types/sample';

interface BackendSample {
  id: number;
  sample_id: string;
  patient_id: string;
  patient_name?: string;
  description?: string;
  status: string;
  registration_date: number;
  updated_at?: number;
}

// Map backend status strings to frontend enum values
const mapStatus = (backendStatus: string): Sample['status'] => {
  // Backend returns German status strings
  const statusMap: Record<string, Sample['status']> = {
    'Erfasst': 'REGISTERED',
    'In Analyse': 'IN_ANALYSIS',
    'Analysiert': 'ANALYZED',
    'Validiert': 'VALIDATED',
    'Archiviert': 'ARCHIVED',
    'REGISTERED': 'REGISTERED',
    'IN_ANALYSIS': 'IN_ANALYSIS',
    'ANALYZED': 'ANALYZED',
    'VALIDATED': 'VALIDATED',
    'ARCHIVED': 'ARCHIVED',
  };
  return statusMap[backendStatus] || 'REGISTERED';
};

// Transform backend sample to frontend sample
const transformSample = (backendSample: BackendSample): Sample => {
  return {
    id: backendSample.id,
    sample_id: backendSample.sample_id,
    patient_id: backendSample.patient_id,
    patient_name: backendSample.patient_name || '',
    description: backendSample.description || '',
    status: mapStatus(backendSample.status),
    created_at: new Date(backendSample.registration_date * 1000).toISOString(),
    updated_at: new Date(((backendSample.updated_at || backendSample.registration_date) * 1000)).toISOString(),
  };
};

export const getSamples = async (filters?: SampleFilter): Promise<SampleListResponse> => {
  const response = await api.get<{ data: BackendSample[]; total: number }>('/samples', {
    params: filters,
  });

  // Transform backend response to frontend format
  const samples = response.data.data.map(transformSample);

  return {
    samples,
    total: response.data.total ?? samples.length,
    limit: filters?.limit || samples.length,
    offset: filters?.offset || 0,
  };
};

export const getSampleById = async (id: string): Promise<Sample> => {
  const response = await api.get<{ data: BackendSample }>(`/samples/${id}`);
  return transformSample(response.data.data);
};

export const createSample = async (sample: Omit<Sample, 'id' | 'created_at' | 'updated_at'>): Promise<Sample> => {
  const response = await api.post<{ data: BackendSample }>('/samples', sample);
  return transformSample(response.data.data);
};

export const updateSample = async (id: string, sample: Partial<Sample>): Promise<Sample> => {
  const response = await api.put<{ data: BackendSample }>(`/samples/${id}`, sample);
  return transformSample(response.data.data);
};

export const deleteSample = async (id: string): Promise<void> => {
  await api.delete(`/samples/${id}`);
};
