import { useState, useRef } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Button } from '../components/common/Button';
import { createSample } from '../services/samples';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { useAuth } from '../context/AuthContext';

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
        i++; // skip second quote of escaped pair
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
  useDocumentTitle({ module: 'CSV Import' });
  const { user } = useAuth();
  const [rows, setRows] = useState<ImportRow[]>([]);
  const [importing, setImporting] = useState(false);
  const [fileName, setFileName] = useState('');
  const fileInputRef = useRef<HTMLInputElement>(null);

  const canImport = user?.role === 'ADMIN' || user?.role === 'OPERATOR';

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setFileName(file.name);
    setRows([]);

    const reader = new FileReader();
    reader.onload = (evt) => {
      // Strip UTF-8 BOM if present (common with Excel-exported CSVs)
      const rawText = (evt.target?.result as string).replace(/^\uFEFF/, '');
      const allLines = rawText.split(/\r?\n/).filter(l => l.trim() !== '');
      // Skip header row if it starts with known column names
      const firstLower = allLines[0]?.toLowerCase() ?? '';
      const lines = (firstLower.startsWith('sample_id') || firstLower.startsWith('id,'))
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

  const successCount = rows.filter(r => r.status === 'success').length;
  const errorCount = rows.filter(r => r.status === 'error').length;
  const pendingCount = rows.filter(r => r.status === 'pending').length;
  const isDone = rows.length > 0 && pendingCount === 0 && !importing;

  return (
    <Layout>
      <div className="space-y-6">
        <div className="border-b border-[#E2E8F0] pb-4">
          <h1 className="text-2xl font-bold text-[#1A1C20] tracking-tight uppercase">CSV Import</h1>
          <p className="text-[#5E6C84] text-sm mt-1 font-mono">
            Samples aus CSV-Datei importieren — Format: <code>sample_id,patient_id,patient_name,description</code>
          </p>
        </div>

        {!canImport && (
          <div className="bg-orange-50 border border-orange-200 rounded p-4">
            <p className="text-orange-800 text-sm">Nur ADMIN und OPERATOR können Samples importieren.</p>
          </div>
        )}

        {canImport && (
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
                  if (file && fileInputRef.current) {
                    const dt = new DataTransfer();
                    dt.items.add(file);
                    fileInputRef.current.files = dt.files;
                    fileInputRef.current.dispatchEvent(new Event('change', { bubbles: true }));
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
      </div>
    </Layout>
  );
};

export default Import;
