import { useEffect, useRef, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { Input } from '../components/common/Input';
import { getAuditLog } from '../services/audit';
import type { AuditEntry, AuditLogFilter, AuditAction, AuditEntity } from '../types/audit';
import { AUDIT_ACTIONS, AUDIT_ENTITIES, ACTION_COLORS } from '../types/audit';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { ErrorBanner } from '../components/common/ErrorBanner';

export const AuditLog = () => {
  useDocumentTitle({ module: 'Audit Log' });
  const [entries, setEntries] = useState<AuditEntry[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [filter, setFilter] = useState<AuditLogFilter>({
    limit: 50,
  });
  const mountedRef = useRef(true);
  useEffect(() => {
    mountedRef.current = true;
    return () => { mountedRef.current = false; };
  }, []);

  const extractMsg = (err: unknown): string => {
    if (err && typeof err === 'object' && 'response' in err)
      return (err as { response?: { data?: { error?: { message?: string } } } }).response?.data?.error?.message || 'Failed to load audit log';
    return err instanceof Error ? err.message : 'Failed to load audit log';
  };

  const runFetch = (f: AuditLogFilter) => {
    let cancelled = false;
    setLoading(true);
    setError('');
    getAuditLog(f)
      .then(data => { if (!cancelled && mountedRef.current) setEntries(data); })
      .catch(err => { if (!cancelled && mountedRef.current) setError(extractMsg(err)); })
      .finally(() => { if (!cancelled && mountedRef.current) setLoading(false); });
    return () => { cancelled = true; };
  };

  useEffect(() => {
    const cancel = runFetch(filter);
    return cancel;
  }, []); // eslint-disable-line react-hooks/exhaustive-deps

  const handleApplyFilter = () => runFetch(filter);

  const handleResetFilter = () => {
    const resetFilter = { limit: 50 };
    setFilter(resetFilter);
    runFetch(resetFilter);
  };

  if (loading) {
    return (
      <Layout>
        <div className="flex items-center justify-center h-64">
          <div className="text-center">
            <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
            <p className="mt-4 text-gray-600">Loading audit log...</p>
          </div>
        </div>
      </Layout>
    );
  }

  return (
    <Layout>
      <div className="space-y-6">
        <div>
          <div className="flex flex-wrap items-center justify-between gap-3">
            <h2 className="text-2xl md:text-3xl font-bold text-gray-900">Audit Log</h2>
            <button
              onClick={() => {
                const token = localStorage.getItem('opensylab_jwt_token');
                fetch(`${import.meta.env.VITE_API_URL}/audit/export`, {
                  headers: token ? { Authorization: `Bearer ${token}` } : {}
                }).then(r => r.blob()).then(blob => {
                  const url = URL.createObjectURL(blob);
                  const a = document.createElement('a');
                  a.href = url; a.download = 'audit-log.csv'; a.click();
                  URL.revokeObjectURL(url);
                });
              }}
              className="inline-flex items-center gap-2 px-3 py-1.5 text-sm font-medium text-gray-700 bg-white border border-gray-300 rounded hover:bg-gray-50 transition-colors"
            >
              <svg className="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4"/>
              </svg>
              Export CSV
            </button>
          </div>
          <p className="text-gray-600 mt-1">System activity audit trail for compliance and monitoring</p>
        </div>

        <ErrorBanner message={error || null} />

        {/* Filters */}
        <Card title="Filters">
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
            <Input
              type="text"
              label="User"
              placeholder="Filter by username"
              value={filter.user || ''}
              onChange={(e) => setFilter({ ...filter, user: e.target.value || undefined })}
            />

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                Action
              </label>
              <select
                value={filter.action || ''}
                onChange={(e) => setFilter({ ...filter, action: (e.target.value as AuditAction) || undefined })}
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
              >
                <option value="">All Actions</option>
                {Object.entries(AUDIT_ACTIONS).map(([value, label]) => (
                  <option key={value} value={value}>{label}</option>
                ))}
              </select>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                Entity
              </label>
              <select
                value={filter.entity || ''}
                onChange={(e) => setFilter({ ...filter, entity: (e.target.value as AuditEntity) || undefined })}
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
              >
                <option value="">All Entities</option>
                {Object.entries(AUDIT_ENTITIES).map(([value, label]) => (
                  <option key={value} value={value}>{label}</option>
                ))}
              </select>
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                Limit
              </label>
              <select
                value={filter.limit || 50}
                onChange={(e) => setFilter({ ...filter, limit: parseInt(e.target.value) })}
                className="w-full px-3 py-2 border border-gray-300 rounded focus:outline-none focus:ring-2 focus:ring-[#0055FF] focus:border-transparent"
              >
                <option value="25">25 entries</option>
                <option value="50">50 entries</option>
                <option value="100">100 entries</option>
                <option value="250">250 entries</option>
              </select>
            </div>
          </div>

          <div className="flex justify-end gap-3 mt-4">
            <button
              onClick={handleResetFilter}
              className="px-4 py-2 text-sm font-medium text-gray-700 bg-white border border-gray-300 rounded hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-[#0055FF]"
            >
              Reset
            </button>
            <button
              onClick={handleApplyFilter}
              className="px-4 py-2 text-sm font-medium text-white bg-blue-600 border border-transparent rounded hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-[#0055FF]"
            >
              Apply Filters
            </button>
          </div>
        </Card>

        {/* Audit Log Table */}
        <Card title={`Audit Entries (${entries.length})`}>
          {entries.length === 0 ? (
            <p className="text-gray-500 text-center py-8">No audit entries found</p>
          ) : (
            <div className="overflow-x-auto">
              <table className="min-w-full divide-y divide-gray-200">
                <thead>
                  <tr>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Timestamp
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      User
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Action
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Entity
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Entity ID
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Details
                    </th>
                  </tr>
                </thead>
                <tbody className="bg-white divide-y divide-gray-200">
                  {entries.map((entry) => (
                    <tr key={entry.id} className="hover:bg-gray-50">
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                        {new Date(entry.timestamp * 1000).toLocaleString()}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                        {entry.user}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap">
                        <span className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${ACTION_COLORS[entry.action]}`}>
                          {AUDIT_ACTIONS[entry.action]}
                        </span>
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                        {AUDIT_ENTITIES[entry.entity]}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600 font-mono">
                        {entry.entity_id}
                      </td>
                      <td className="px-6 py-4 text-sm text-gray-600 max-w-md truncate" title={entry.details}>
                        {entry.details || '-'}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </Card>
      </div>
    </Layout>
  );
};

export default AuditLog;
