import { useState, useRef } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Button } from '../components/common/Button';
import { createSample } from '../services/samples';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { useAuth } from '../context/AuthContext';
import { importHl7, importFhir } from '../services/import';
import type { ImportSummary } from '../services/import';

interface ImportRow {
  lineNumber: number;
  raw: string;
  status: 'pending' | 'success' | 'error';
  message: string;
  sampleId?: string;
}

function parseCsvRow(line: string): string[] {
  const result: string[] = [];
  let current = '';
  let inQuotes = false;
  for (let i = 0; i < line.length; i++) {
    const ch = line[i];
    if (ch === '"') {
      if (inQuotes && i + 1 < line.length && line[i + 1] === '"') {
        current += '"';
        i++;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (ch === ',' && !inQuotes) {
      result.push(current.trim());
      current = '';
    } else {
      current += ch;
    }
  }
  result.push(current.trim());
  return result;
}

export const Import = () => {
  useDocumentTitle({ module: 'Import' });
  const { user } = useAuth();
  const [activeTab, setActiveTab] = useState<'csv' | 'hl7' | 'fhir'>('csv');

  const [rows, setRows] = useState<ImportRow[]>([]);
  const [importing, setImporting] = useState(false);
  const [fileName, setFileName] = useState('');
  const fileInputRef = useRef<HTMLInputElement>(null);

  const [hl7FileContent, setHl7FileContent] = useState('');
  const [hl7FileName, setHl7FileName] = useState('');
  const [hl7Importing, setHl7Importing] = useState(false);
  const [hl7Result, setHl7Result] = useState<ImportSummary | null>(null);
  const [hl7Error, setHl7Error] = useState('');
  const hl7FileInputRef = useRef<HTMLInputElement>(null);

  const [fhirFileContent, setFhirFileContent] = useState('');
  const [fhirFileName, setFhirFileName] = useState('');
  const [fhirImporting, setFhirImporting] = useState(false);
  const [fhirResult, setFhirResult] = useState<ImportSummary | null>(null);
  const [fhirError, setFhirError] = useState('');
  const fhirFileInputRef = useRef<HTMLInputElement>(null);

  const canImport = user?.role === 'ADMIN' || user?.role === 'OPERATOR';

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setFileName(file.name);
    setRows([]);

    const reader = new FileReader();
    reader.onload = (evt) => {
      const rawText = (evt.target?.result as string).replace(/^\uFEFF/, '');
      const allLines = rawText.split(/\r?\n/).filter(l => l.trim() !== '');
      const firstLower = allLines[0]?.toLowerCase() ?? '';
      const lines = (firstLower.startsWith('sample_id') || firstLower === 'id,patient_id' || firstLower.startsWith('id,patient_id'))
        ? allLines.slice(1)
        : allLines;
      const parsed: ImportRow[] = lines.map((line, i) => ({
        lineNumber: i + 1,
        raw: line,
        status: 'pending',
        message: '',
      }));
      setRows(parsed);
    };
    reader.readAsText(file);
  };

  const handleImport = async () => {
    if (rows.length === 0) return;
    setImporting(true);

    const updated = [...rows];
    for (let i = 0; i < updated.length; i++) {
      const row = updated[i];
      const cols = parseCsvRow(row.raw);

      if (cols.length < 2) {
        updated[i] = { ...row, status: 'error', message: 'Mindestens 2 Spalten erwartet (sample_id, patient_id)' };
        setRows([...updated]);
        continue;
      }

      const [sampleId, patientId, patientName = '', description = ''] = cols;
      if (!sampleId || !patientId) {
        updated[i] = { ...row, status: 'error', message: 'sample_id und patient_id sind Pflichtfelder' };
        setRows([...updated]);
        continue;
      }

      try {
        const created = await createSample({
          sample_id: sampleId,
          patient_id: patientId,
          patient_name: patientName,
          description,
          status: 'REGISTERED',
        });
        updated[i] = { ...row, status: 'success', message: 'Importiert', sampleId: created.sample_id };
      } catch (err: unknown) {
        let msg = 'Fehler beim Importieren';
        if (err && typeof err === 'object' && 'response' in err) {
          const r = err as { response?: { data?: { error?: { message?: string } } } };
          msg = r.response?.data?.error?.message ?? msg;
        } else if (err instanceof Error) {
          msg = err.message;
        }
        updated[i] = { ...row, status: 'error', message: msg };
      }
      setRows([...updated]);
    }
    setImporting(false);
  };

  const handleReset = () => {
    setRows([]);
    setFileName('');
    if (fileInputRef.current) fileInputRef.current.value = '';
  };

  const handleHl7FileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setHl7FileName(file.name);
    setHl7Result(null);
    setHl7Error('');

    const reader = new FileReader();
    reader.onload = (evt) => {
      setHl7FileContent((evt.target?.result as string) ?? '');
    };
    reader.readAsText(file);
  };

  const handleHl7Import = async () => {
    if (!hl7FileContent) return;
    setHl7Importing(true);
    setHl7Error('');
    setHl7Result(null);
    try {
      const result = await importHl7(hl7FileContent);
      setHl7Result(result);
    } catch (err: unknown) {
      let msg = 'Fehler beim HL7-Import';
      if (err && typeof err === 'object' && 'response' in err) {
        const r = err as { response?: { data?: { error?: { message?: string } } } };
        msg = r.response?.data?.error?.message ?? msg;
      } else if (err instanceof Error) {
        msg = err.message;
      }
      setHl7Error(msg);
    } finally {
      setHl7Importing(false);
    }
  };

  const handleFhirFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setFhirFileName(file.name);
    setFhirResult(null);
    setFhirError('');

    const reader = new FileReader();
    reader.onload = (evt) => {
      setFhirFileContent((evt.target?.result as string) ?? '');
    };
    reader.readAsText(file);
  };

  const handleFhirImport = async () => {
    if (!fhirFileContent) return;
    setFhirImporting(true);
    setFhirError('');
    setFhirResult(null);
    try {
      const result = await importFhir(fhirFileContent);
      setFhirResult(result);
    } catch (err: unknown) {
      let msg = 'Fehler beim FHIR-Import';
      if (err && typeof err === 'object' && 'response' in err) {
        const r = err as { response?: { data?: { error?: { message?: string } } } };
        msg = r.response?.data?.error?.message ?? msg;
      } else if (err instanceof Error) {
        msg = err.message;
      }
      setFhirError(msg);
    } finally {
      setFhirImporting(false);
    }
  };

  const successCount = rows.filter(r => r.status === 'success').length;
  const errorCount = rows.filter(r => r.status === 'error').length;
  const pendingCount = rows.filter(r => r.status === 'pending').length;
  const isDone = rows.length > 0 && pendingCount === 0 && !importing;

  return (
    <Layout>
      <div className="space-y-6">
        <div className="border-b border-[#E2E8F0]">
          <h1 className="text-2xl font-bold text-[#1A1C20] tracking-tight uppercase mb-4">Import</h1>
          <div className="flex space-x-6">
            <button
              onClick={() => setActiveTab('csv')}
              className={`pb-4 text-sm font-bold tracking-wider uppercase border-b-2 transition-colors ${
                activeTab === 'csv'
                  ? 'border-[#1A1C20] text-[#1A1C20]'
                  : 'border-transparent text-[#5E6C84] hover:text-[#1A1C20]'
              }`}
            >
              CSV Import
            </button>
            <button
              onClick={() => setActiveTab('hl7')}
              className={`pb-4 text-sm font-bold tracking-wider uppercase border-b-2 transition-colors ${
                activeTab === 'hl7'
                  ? 'border-[#1A1C20] text-[#1A1C20]'
                  : 'border-transparent text-[#5E6C84] hover:text-[#1A1C20]'
              }`}
            >
              HL7 v2.5.1
            </button>
            <button
              onClick={() => setActiveTab('fhir')}
              className={`pb-4 text-sm font-bold tracking-wider uppercase border-b-2 transition-colors ${
                activeTab === 'fhir'
                  ? 'border-[#1A1C20] text-[#1A1C20]'
                  : 'border-transparent text-[#5E6C84] hover:text-[#1A1C20]'
              }`}
            >
              FHIR R4
            </button>
          </div>
        </div>

        {!canImport && (
          <div className="bg-orange-50 border border-orange-200 rounded p-4">
            <p className="text-orange-800 text-sm">Nur ADMIN und OPERATOR können importieren.</p>
          </div>
        )}

        {canImport && activeTab === 'csv' && (
          <div className="bg-white border border-[#E2E8F0] p-6 space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">CSV-Datei auswählen</label>
              <div
                className="border-2 border-dashed border-gray-300 rounded p-8 text-center cursor-pointer hover:border-blue-400 transition-colors"
                onClick={() => fileInputRef.current?.click()}
                onDragOver={e => e.preventDefault()}
                onDrop={e => {
                  e.preventDefault();
                  const file = e.dataTransfer.files?.[0];
                  if (file) {
                    const fakeEvt = { target: { files: e.dataTransfer.files } } as React.ChangeEvent<HTMLInputElement>;
                    handleFileChange(fakeEvt);
                  }
                }}
              >
                <svg className="w-10 h-10 text-gray-400 mx-auto mb-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12" />
                </svg>
                {fileName ? (
                  <p className="text-sm font-medium text-blue-600">{fileName}</p>
                ) : (
                  <p className="text-sm text-gray-500">CSV hierher ziehen oder klicken zum Auswählen</p>
                )}
              </div>
              <input
                ref={fileInputRef}
                type="file"
                accept=".csv,text/csv"
                className="hidden"
                onChange={handleFileChange}
              />
            </div>

            {rows.length > 0 && (
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <p className="text-sm text-gray-600">
                    <span className="font-medium">{rows.length}</span> Zeilen geladen
                    {isDone && (
                      <span className="ml-3">
                        <span className="text-green-600 font-medium">{successCount} erfolgreich</span>
                        {errorCount > 0 && <span className="text-red-600 font-medium ml-2">{errorCount} Fehler</span>}
                      </span>
                    )}
                  </p>
                  <div className="flex gap-2">
                    <Button variant="secondary" size="sm" onClick={handleReset} disabled={importing}>
                      Zurücksetzen
                    </Button>
                    {!isDone && (
                      <Button variant="primary" size="sm" onClick={handleImport} disabled={importing || pendingCount === 0}>
                        {importing ? `Importiere... (${successCount + errorCount}/${rows.length})` : `${rows.length} Zeilen importieren`}
                      </Button>
                    )}
                  </div>
                </div>

                <div className="overflow-x-auto border border-[#E2E8F0] rounded max-h-96">
                  <table className="min-w-full text-sm">
                    <thead className="bg-[#F4F5F7] sticky top-0">
                      <tr>
                        <th className="px-4 py-2 text-left text-[10px] font-bold uppercase tracking-wider text-[#5E6C84] border-b border-[#E2E8F0] w-12">#</th>
                        <th className="px-4 py-2 text-left text-[10px] font-bold uppercase tracking-wider text-[#5E6C84] border-b border-[#E2E8F0]">Zeile</th>
                        <th className="px-4 py-2 text-left text-[10px] font-bold uppercase tracking-wider text-[#5E6C84] border-b border-[#E2E8F0] w-24">Status</th>
                        <th className="px-4 py-2 text-left text-[10px] font-bold uppercase tracking-wider text-[#5E6C84] border-b border-[#E2E8F0]">Meldung</th>
                      </tr>
                    </thead>
                    <tbody>
                      {rows.map(row => (
                        <tr key={row.lineNumber} className={
                          row.status === 'success' ? 'bg-green-50' :
                          row.status === 'error' ? 'bg-red-50' :
                          ''
                        }>
                          <td className="px-4 py-2 font-mono text-[#5E6C84] border-b border-[#E2E8F0]">{row.lineNumber}</td>
                          <td className="px-4 py-2 font-mono text-xs text-[#1A1C20] border-b border-[#E2E8F0] max-w-xs truncate">{row.raw}</td>
                          <td className="px-4 py-2 border-b border-[#E2E8F0]">
                            {row.status === 'success' && <span className="text-green-700 font-bold text-xs">✓ OK</span>}
                            {row.status === 'error' && <span className="text-red-700 font-bold text-xs">✗ Fehler</span>}
                            {row.status === 'pending' && <span className="text-gray-400 text-xs">—</span>}
                          </td>
                          <td className="px-4 py-2 text-xs border-b border-[#E2E8F0] text-gray-600">
                            {row.status === 'success' && row.sampleId ? `ID: ${row.sampleId}` : row.message}
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </div>
            )}
          </div>
        )}

        {canImport && activeTab === 'hl7' && (
          <div className="bg-white border border-[#E2E8F0] p-6 space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">HL7-Datei auswählen</label>
              <div
                className="border-2 border-dashed border-gray-300 rounded p-8 text-center cursor-pointer hover:border-blue-400 transition-colors"
                onClick={() => hl7FileInputRef.current?.click()}
                onDragOver={e => e.preventDefault()}
                onDrop={e => {
                  e.preventDefault();
                  const file = e.dataTransfer.files?.[0];
                  if (file) {
                    const fakeEvt = { target: { files: e.dataTransfer.files } } as React.ChangeEvent<HTMLInputElement>;
                    handleHl7FileChange(fakeEvt);
                  }
                }}
              >
                <svg className="w-10 h-10 text-gray-400 mx-auto mb-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12" />
                </svg>
                {hl7FileName ? (
                  <p className="text-sm font-medium text-blue-600">{hl7FileName}</p>
                ) : (
                  <p className="text-sm text-gray-500">HL7-Datei hierher ziehen oder klicken zum Auswählen</p>
                )}
              </div>
              <input
                ref={hl7FileInputRef}
                type="file"
                accept=".hl7,.txt,text/plain"
                className="hidden"
                onChange={handleHl7FileChange}
              />
            </div>

            {hl7FileName && (
              <div className="flex items-center justify-between">
                <p className="text-sm text-gray-600">{hl7FileName} ausgewählt</p>
                <Button
                  variant="primary"
                  size="sm"
                  onClick={handleHl7Import}
                  disabled={hl7Importing || !hl7FileContent}
                >
                  {hl7Importing ? 'Importiere...' : 'HL7 importieren'}
                </Button>
              </div>
            )}

            {hl7Result && (
              <p className="text-sm text-green-700 font-medium">
                Importiert: {hl7Result.imported.samples} Samples, {hl7Result.imported.orders} Orders, {hl7Result.imported.results} Results
              </p>
            )}

            {hl7Error && (
              <p className="text-sm text-red-700 font-medium">{hl7Error}</p>
            )}
          </div>
        )}

        {canImport && activeTab === 'fhir' && (
          <div className="bg-white border border-[#E2E8F0] p-6 space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">FHIR-Datei auswählen</label>
              <div
                className="border-2 border-dashed border-gray-300 rounded p-8 text-center cursor-pointer hover:border-blue-400 transition-colors"
                onClick={() => fhirFileInputRef.current?.click()}
                onDragOver={e => e.preventDefault()}
                onDrop={e => {
                  e.preventDefault();
                  const file = e.dataTransfer.files?.[0];
                  if (file) {
                    const fakeEvt = { target: { files: e.dataTransfer.files } } as React.ChangeEvent<HTMLInputElement>;
                    handleFhirFileChange(fakeEvt);
                  }
                }}
              >
                <svg className="w-10 h-10 text-gray-400 mx-auto mb-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12" />
                </svg>
                {fhirFileName ? (
                  <p className="text-sm font-medium text-blue-600">{fhirFileName}</p>
                ) : (
                  <p className="text-sm text-gray-500">FHIR-Datei hierher ziehen oder klicken zum Auswählen</p>
                )}
              </div>
              <input
                ref={fhirFileInputRef}
                type="file"
                accept=".json,application/json"
                className="hidden"
                onChange={handleFhirFileChange}
              />
            </div>

            {fhirFileName && (
              <div className="flex items-center justify-between">
                <p className="text-sm text-gray-600">{fhirFileName} ausgewählt</p>
                <Button
                  variant="primary"
                  size="sm"
                  onClick={handleFhirImport}
                  disabled={fhirImporting || !fhirFileContent}
                >
                  {fhirImporting ? 'Importiere...' : 'FHIR importieren'}
                </Button>
              </div>
            )}

            {fhirResult && (
              <p className="text-sm text-green-700 font-medium">
                Importiert: {fhirResult.imported.samples} Samples, {fhirResult.imported.orders} Orders, {fhirResult.imported.results} Results
              </p>
            )}

            {fhirError && (
              <p className="text-sm text-red-700 font-medium">{fhirError}</p>
            )}
          </div>
        )}
      </div>
    </Layout>
  );
};

export default Import;
