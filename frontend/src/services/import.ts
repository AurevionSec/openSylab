import api from './api';

export interface ImportSummary {
  imported: {
    samples: number;
    orders: number;
    results: number;
  };
}

export async function importHl7(content: string): Promise<ImportSummary> {
  const response = await api.post<ImportSummary>('/hl7/import', content, {
    headers: { 'Content-Type': 'text/plain' },
  });
  return response.data;
}

export async function importFhir(content: string): Promise<ImportSummary> {
  const response = await api.post<ImportSummary>('/fhir/import', content, {
    headers: { 'Content-Type': 'application/fhir+json' },
  });
  return response.data;
}
