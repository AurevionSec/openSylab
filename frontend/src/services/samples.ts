import api from './api';
import type { Sample, SampleFilter, SampleListResponse } from '../types/sample';

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
const transformSample = (backendSample: any): Sample => {
  return {
    id: backendSample.id,
    sample_id: backendSample.sample_id,
    patient_id: backendSample.patient_id,
    patient_name: backendSample.patient_name || '',
    description: backendSample.description || '',
    status: mapStatus(backendSample.status),
    created_at: new Date(backendSample.registration_date * 1000).toISOString(),
    updated_at: new Date(backendSample.registration_date * 1000).toISOString(),
  };
};

export const getSamples = async (filters?: SampleFilter): Promise<SampleListResponse> => {
  const response = await api.get<{ data: any[] }>('/samples', {
    params: filters,
  });

  // Transform backend response to frontend format
  const samples = response.data.data.map(transformSample);

  return {
    samples,
    total: samples.length,
    limit: filters?.limit || samples.length,
    offset: filters?.offset || 0,
  };
};

export const getSampleById = async (id: number): Promise<Sample> => {
  const response = await api.get<Sample>(`/samples/${id}`);
  return response.data;
};

export const createSample = async (sample: Omit<Sample, 'id' | 'created_at' | 'updated_at'>): Promise<Sample> => {
  const response = await api.post<Sample>('/samples', sample);
  return response.data;
};

export const updateSample = async (id: number, sample: Partial<Sample>): Promise<Sample> => {
  const response = await api.put<Sample>(`/samples/${id}`, sample);
  return response.data;
};

export const deleteSample = async (id: number): Promise<void> => {
  await api.delete(`/samples/${id}`);
};
