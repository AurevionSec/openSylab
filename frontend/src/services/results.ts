import api from './api';
import type { TestResult, ResultFilter, ResultListResponse } from '../types/result';

// Map backend status strings to frontend enum values
const mapStatus = (backendStatus: string): TestResult['status'] => {
  const statusMap: Record<string, TestResult['status']> = {
    'Ausstehend': 'PENDING',
    'Geprüft': 'REVIEWED',
    'Validiert': 'VALIDATED',
    'Abgelehnt': 'REJECTED',
    'Korrigiert': 'AMENDED',
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
    'Kritisch': 'CRITICAL',
    'NORMAL': 'NORMAL',
    'LOW': 'LOW',
    'HIGH': 'HIGH',
    'CRITICAL': 'CRITICAL',
  };
  return flagMap[backendFlag] || 'NORMAL';
};

// Transform backend result to frontend result
const transformResult = (backendResult: any): TestResult => {
  return {
    id: backendResult.id,
    result_id: backendResult.result_id,
    order_id: backendResult.order_id,
    parameter: backendResult.parameter || '',
    value: backendResult.value || '',
    unit: backendResult.unit || '',
    reference_min: backendResult.min_value?.toString() || '',
    reference_max: backendResult.max_value?.toString() || '',
    flag: mapFlag(backendResult.flag),
    status: mapStatus(backendResult.status),
    reviewed_by: backendResult.reviewed_by || '',
    reviewed_date: backendResult.reviewed_date
      ? new Date(backendResult.reviewed_date * 1000).toISOString()
      : '',
    notes: backendResult.notes || '',
  };
};

export const getResults = async (filters?: ResultFilter): Promise<ResultListResponse> => {
  const response = await api.get<{ data: any[] }>('/results', {
    params: filters,
  });

  // Transform backend response to frontend format
  const results = response.data.data.map(transformResult);

  return {
    results,
    total: results.length,
    limit: filters?.limit || results.length,
    offset: filters?.offset || 0,
  };
};

export const getResultById = async (id: string): Promise<TestResult> => {
  const response = await api.get<{ data: any }>(`/results/${id}`);
  return transformResult(response.data.data);
};

export const createResult = async (result: Omit<TestResult, 'id' | 'reviewed_date'>): Promise<TestResult> => {
  const response = await api.post<{ data: any }>('/results', {
    result_id: result.result_id,
    order_id: result.order_id,
    parameter: result.parameter,
    value: result.value,
    unit: result.unit,
    min_value: result.reference_min,
    max_value: result.reference_max,
    flag: result.flag,
    status: result.status,
    reviewed_by: result.reviewed_by,
    notes: result.notes,
  });
  return transformResult(response.data.data);
};

export const updateResult = async (id: string, result: Partial<TestResult>): Promise<TestResult> => {
  const updateData: any = {};

  if (result.result_id) updateData.result_id = result.result_id;
  if (result.order_id) updateData.order_id = result.order_id;
  if (result.parameter) updateData.parameter = result.parameter;
  if (result.value) updateData.value = result.value;
  if (result.unit) updateData.unit = result.unit;
  if (result.reference_min) updateData.min_value = result.reference_min;
  if (result.reference_max) updateData.max_value = result.reference_max;
  if (result.flag) updateData.flag = result.flag;
  if (result.status) updateData.status = result.status;
  if (result.reviewed_by) updateData.reviewed_by = result.reviewed_by;
  if (result.notes !== undefined) updateData.notes = result.notes;

  const response = await api.put<{ data: any }>(`/results/${id}`, updateData);
  return transformResult(response.data.data);
};

export const deleteResult = async (id: string): Promise<void> => {
  await api.delete(`/results/${id}`);
};
