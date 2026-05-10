import api from './api';
import type { TestResult, ResultFilter, ResultListResponse } from '../types/result';

// Map backend status strings to frontend enum values
const mapStatus = (backendStatus: string): TestResult['status'] => {
  const statusMap: Record<string, TestResult['status']> = {
    'Ausstehend': 'PENDING',
    'Eingegeben': 'REVIEWED',  // Backend uses "Eingegeben" for entered results
    'Geprüft': 'REVIEWED',
    'Validiert': 'VALIDATED',
    'Abgelehnt': 'REJECTED',
    'Korrigiert': 'AMENDED',
    'Wiederholung': 'AMENDED',  // Backend uses "Wiederholung" for repeat tests
    'PENDING': 'PENDING',
    'REVIEWED': 'REVIEWED',
    'VALIDATED': 'VALIDATED',
    'REJECTED': 'REJECTED',
    'AMENDED': 'AMENDED',
  };
  return statusMap[backendStatus] || 'PENDING';
};

// Map backend flag strings to frontend enum values
const mapFlag = (backendFlag: string): TestResult['flag'] => {
  const flagMap: Record<string, TestResult['flag']> = {
    'Normal': 'NORMAL',
    'Niedrig': 'LOW',
    'Hoch': 'HIGH',
    'Erhöht': 'HIGH',  // Backend uses "Erhöht" for elevated
    'Kritisch': 'CRITICAL',
    'NORMAL': 'NORMAL',
    'LOW': 'LOW',
    'HIGH': 'HIGH',
    'CRITICAL': 'CRITICAL',
    'UNDEFINED': 'UNDEFINED',
    'Unbekannt': 'UNDEFINED',
  };
  return flagMap[backendFlag] || 'NORMAL';
};

// Transform backend result to frontend result
const transformResult = (backendResult: any): TestResult => {
  return {
    id: backendResult.id,
    result_id: backendResult.result_id,
    order_id: backendResult.order_id,
    parameter: backendResult.test_parameter || backendResult.parameter || '',  // Backend uses "test_parameter"
    value: backendResult.value || '',
    unit: backendResult.unit || '',
    reference_min: backendResult.reference_low?.toString() || backendResult.min_value?.toString() || '',  // Backend uses "reference_low"
    reference_max: backendResult.reference_high?.toString() || backendResult.max_value?.toString() || '',  // Backend uses "reference_high"
    flag: mapFlag(backendResult.flag),
    status: mapStatus(backendResult.status),
    reviewed_by: backendResult.measured_by || backendResult.reviewed_by || '',  // Backend uses "measured_by"
    reviewed_date: backendResult.measured_date
      ? new Date(backendResult.measured_date * 1000).toISOString()
      : backendResult.reviewed_date
      ? new Date(backendResult.reviewed_date * 1000).toISOString()
      : '',
    notes: backendResult.comment || backendResult.notes || '',  // Backend uses "comment"
  };
};

export const getResults = async (filters?: ResultFilter): Promise<ResultListResponse> => {
  const response = await api.get<{ data: any[]; total: number }>('/results', {
    params: filters,
  });

  // Transform backend response to frontend format
  const results = response.data.data.map(transformResult);

  return {
    results,
    total: response.data.total ?? results.length,
    limit: filters?.limit || results.length,
    offset: filters?.offset || 0,
  };
};

export const getResultById = async (id: string): Promise<TestResult> => {
  const response = await api.get<{ data: any }>(`/results/${id}`);
  return transformResult(response.data.data);
};

const mapStatusToBackend = (status: string): string => {
  const statusMap: Record<string, string> = {
    'REVIEWED': 'ENTERED',
    'AMENDED': 'REPEATED',
  };
  return statusMap[status] ?? status;
};

export const createResult = async (result: Omit<TestResult, 'id' | 'reviewed_date'>): Promise<TestResult> => {
  const response = await api.post<{ data: any }>('/results', {
    result_id: result.result_id,
    order_id: result.order_id,
    test_parameter: result.parameter,
    value: result.value,
    unit: result.unit,
    reference_low: result.reference_min,
    reference_high: result.reference_max,
    flag: result.flag,
    status: mapStatusToBackend(result.status || 'PENDING'),
    reviewed_by: result.reviewed_by,
    notes: result.notes,
  });
  return transformResult(response.data.data);
};

export const updateResult = async (id: string, result: Partial<TestResult>): Promise<TestResult> => {
  const updateData: any = {};

  if (result.result_id !== undefined) updateData.result_id = result.result_id;
  if (result.order_id !== undefined) updateData.order_id = result.order_id;
  if (result.parameter !== undefined) updateData.test_parameter = result.parameter;
  if (result.value !== undefined) updateData.value = result.value;
  if (result.unit !== undefined) updateData.unit = result.unit;
  if (result.reference_min !== undefined) updateData.reference_low = result.reference_min;
  if (result.reference_max !== undefined) updateData.reference_high = result.reference_max;
  if (result.flag !== undefined) updateData.flag = result.flag;
  if (result.status !== undefined) updateData.status = mapStatusToBackend(result.status);
  if (result.reviewed_by !== undefined) updateData.reviewed_by = result.reviewed_by;
  if (result.notes !== undefined) updateData.notes = result.notes;

  const response = await api.put<{ data: any }>(`/results/${id}`, updateData);
  return transformResult(response.data.data);
};

export const deleteResult = async (id: string): Promise<void> => {
  await api.delete(`/results/${id}`);
};
