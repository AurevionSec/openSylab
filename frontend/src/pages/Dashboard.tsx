import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { getDashboardStats } from '../services/stats';
import { getSamples } from '../services/samples';
import { getOrders } from '../services/orders';
import { getResults } from '../services/results';
import type { Sample } from '../types/sample';
import type { DashboardStats } from '../types/stats';
import { useDocumentTitle } from '../hooks/useDocumentTitle';
import { ErrorBanner } from '../components/common/ErrorBanner';
import {
  BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, Rectangle,
} from 'recharts';
import type { BarShapeProps } from 'recharts';

const STATUS_COLORS: Record<string, string> = {
  REGISTERED: '#3B82F6',
  IN_ANALYSIS: '#EAB308',
  ANALYZED: '#9CA3AF',
  VALIDATED: '#22C55E',
  ARCHIVED: '#6B7280',
  REQUESTED: '#3B82F6',
  IN_PROGRESS: '#EAB308',
  COMPLETED: '#22C55E',
  CANCELLED: '#EF4444',
  PENDING: '#EAB308',
  ENTERED: '#60A5FA',
  REJECTED: '#EF4444',
};

export const Dashboard = () => {
  useDocumentTitle({ module: 'Dashboard' });
  const [stats, setStats] = useState<DashboardStats | null>(null);
  const [samples, setSamples] = useState<Sample[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [warning, setWarning] = useState('');

  useEffect(() => {
    let cancelled = false;
    const fetchData = async () => {
      try {
        let statsData: DashboardStats | null = null;
        try {
          statsData = await getDashboardStats();
        } catch (err) {
          console.error('Failed to load dashboard stats:', err);
          statsData = {
            samples: { entity_type: 'samples', total: 0, by_status: [] },
            orders: { entity_type: 'orders', total: 0, by_status: [] },
            results: { entity_type: 'results', total: 0, by_status: [] },
          };
        }

        const [samplesResult, ordersResult, resultsResult] = await Promise.allSettled([
          getSamples({ limit: 100 }),
          getOrders({ limit: 100 }),
          getResults({ limit: 100 }),
        ]);

        const samplesData = samplesResult.status === 'fulfilled' ? samplesResult.value : null;
        const ordersData = ordersResult.status === 'fulfilled' ? ordersResult.value : null;
        const resultsData = resultsResult.status === 'fulfilled' ? resultsResult.value : null;

        const fetchErrors: string[] = [];
        if (samplesResult.status === 'rejected') fetchErrors.push('samples');
        if (ordersResult.status === 'rejected') fetchErrors.push('orders');
        if (resultsResult.status === 'rejected') fetchErrors.push('results');
        if (fetchErrors.length > 0 && !cancelled) {
          // Inline warning — partial data is still shown; only a full failure triggers error
          setWarning(`Failed to load: ${fetchErrors.join(', ')}. Displayed data may be incomplete.`);
        }

        if (statsData && samplesData && statsData.samples.total === 0 && samplesData.total > 0) {
          statsData.samples.total = samplesData.total;
        }
        if (statsData && ordersData && statsData.orders.total === 0 && ordersData.total > 0) {
          statsData.orders.total = ordersData.total;
        }
        if (statsData && resultsData && statsData.results.total === 0 && resultsData.total > 0) {
          statsData.results.total = resultsData.total;
        }

        if (!cancelled) setStats(statsData);
        if (!cancelled) setSamples(samplesData?.samples ?? []);
      } catch (err) {
        console.error('Dashboard error:', err);
        if (!cancelled) setError('Failed to load dashboard data');
      } finally {
        if (!cancelled) setLoading(false);
      }
    };
    fetchData();
    return () => { cancelled = true; };
  }, []);

  const recentSamples = [...samples]
    .sort((a, b) => new Date(b.created_at).getTime() - new Date(a.created_at).getTime())
    .slice(0, 5);

  // Chart data
  const sampleStatusChart = (stats?.samples.by_status ?? []).map(s => ({
    name: s.status.replace(/_/g, ' '),
    count: s.count,
    fill: STATUS_COLORS[s.status] ?? '#6B7280',
  }));

  const orderStatusChart = (stats?.orders.by_status ?? []).map(s => ({
    name: s.status.replace(/_/g, ' '),
    count: s.count,
    fill: STATUS_COLORS[s.status] ?? '#6B7280',
  }));

  // Priority distribution from loaded orders
  const priorityColors: Record<string, string> = { NORMAL: '#9CA3AF', URGENT: '#F97316', EMERGENCY: '#EF4444' };
  const orderPriorityChart = (stats?.order_priority ?? []).map(s => ({
    name: s.status,
    count: s.count,
    fill: priorityColors[s.status] ?? '#6B7280',
  }));

  // Critical results
  const criticalCount = stats?.critical_count ?? 0;

  const getStatusColor = (status: string) => {
    const statusMap: Record<string, string> = {
      'REGISTERED': 'bg-blue-50 text-blue-700 border border-blue-200',
      'IN_ANALYSIS': 'bg-yellow-50 text-yellow-800 border border-yellow-200',
      'ANALYZED': 'bg-gray-100 text-gray-700 border border-gray-300',
      'VALIDATED': 'bg-green-50 text-green-700 border border-green-200',
      'ARCHIVED': 'bg-gray-100 text-gray-600 border border-gray-300',
      'PENDING': 'bg-yellow-50 text-yellow-800 border border-yellow-200',
      'COMPLETED': 'bg-green-50 text-green-700 border border-green-200',
      'REQUESTED': 'bg-blue-50 text-blue-700 border border-blue-200',
      'IN_PROGRESS': 'bg-yellow-50 text-yellow-800 border border-yellow-200',
      'CANCELLED': 'bg-red-50 text-red-700 border border-red-200',
      'REJECTED': 'bg-red-50 text-red-700 border border-red-200',
      'CRITICAL': 'bg-red-50 text-red-700 border border-red-200',
      'HIGH': 'bg-orange-50 text-orange-700 border border-orange-200',
      'LOW': 'bg-yellow-50 text-yellow-800 border border-yellow-200',
      'NORMAL': 'bg-green-50 text-green-700 border border-green-200',
      'EMERGENCY': 'bg-red-50 text-red-700 border border-red-200',
      'URGENT': 'bg-orange-50 text-orange-700 border border-orange-200',
    };
    return statusMap[status] || 'bg-gray-100 text-gray-600 border border-gray-300';
  };

  if (loading) {
    return (
      <Layout>
        <div className="flex items-center justify-center h-64">
          <div className="text-center">
            <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mx-auto"></div>
            <p className="mt-4 text-gray-600">Loading dashboard...</p>
          </div>
        </div>
      </Layout>
    );
  }

  if (error) {
    return (
      <Layout>
        <ErrorBanner message={error} />
      </Layout>
    );
  }

  return (
    <Layout>
      <div className="space-y-4">
        {warning && <ErrorBanner message={warning} />}
        {/* Header */}
        <div className="border-b border-[#E2E8F0] pb-4">
          <h1 className="text-2xl font-bold text-[#1A1C20] tracking-tight uppercase">System Overview</h1>
          <p className="text-[#5E6C84] text-sm mt-1 font-mono">Real-time laboratory operations monitoring</p>
        </div>

        {/* Primary Metrics */}
        <div className="grid grid-cols-4 gap-4">
          <div className="bg-white border border-[#E2E8F0] p-6 hover:border-[#0055FF] transition-colors duration-150">
            <div className="flex items-baseline justify-between">
              <span className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Samples</span>
              <span className="text-4xl font-mono font-bold text-[#1A1C20] tabular-nums">
                {String(stats?.samples.total || 0).padStart(3, '0')}
              </span>
            </div>
            {stats && stats.samples.by_status.length > 0 && (
              <div className="mt-4 space-y-2 pt-4 border-t border-[#E2E8F0]">
                {stats.samples.by_status.slice(0, 3).map(s => (
                  <div key={s.status} className="flex justify-between items-center">
                    <span className={`px-2 py-0.5 text-[10px] font-bold uppercase tracking-wider border ${getStatusColor(s.status)}`}>
                      {s.status.replace('_', ' ')}
                    </span>
                    <span className="font-mono text-sm font-bold tabular-nums text-[#1A1C20]">{s.count}</span>
                  </div>
                ))}
              </div>
            )}
          </div>

          <div className="bg-white border border-[#E2E8F0] p-6 hover:border-[#0055FF] transition-colors duration-150">
            <div className="flex items-baseline justify-between">
              <span className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Orders</span>
              <span className="text-4xl font-mono font-bold text-[#1A1C20] tabular-nums">
                {String(stats?.orders.total || 0).padStart(3, '0')}
              </span>
            </div>
            {stats && stats.orders.by_status.length > 0 && (
              <div className="mt-4 space-y-2 pt-4 border-t border-[#E2E8F0]">
                {stats.orders.by_status.slice(0, 3).map(s => (
                  <div key={s.status} className="flex justify-between items-center">
                    <span className={`px-2 py-0.5 text-[10px] font-bold uppercase tracking-wider border ${getStatusColor(s.status)}`}>
                      {s.status.replace('_', ' ')}
                    </span>
                    <span className="font-mono text-sm font-bold tabular-nums text-[#1A1C20]">{s.count}</span>
                  </div>
                ))}
              </div>
            )}
          </div>

          <div className="bg-white border border-[#E2E8F0] p-6 hover:border-[#0055FF] transition-colors duration-150">
            <div className="flex items-baseline justify-between">
              <span className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Results</span>
              <span className="text-4xl font-mono font-bold text-[#1A1C20] tabular-nums">
                {String(stats?.results.total || 0).padStart(3, '0')}
              </span>
            </div>
            {stats && stats.results.by_status.length > 0 && (
              <div className="mt-4 space-y-2 pt-4 border-t border-[#E2E8F0]">
                {stats.results.by_status.slice(0, 3).map(s => (
                  <div key={s.status} className="flex justify-between items-center">
                    <span className={`px-2 py-0.5 text-[10px] font-bold uppercase tracking-wider border ${getStatusColor(s.status)}`}>
                      {s.status.replace('_', ' ')}
                    </span>
                    <span className="font-mono text-sm font-bold tabular-nums text-[#1A1C20]">{s.count}</span>
                  </div>
                ))}
              </div>
            )}
          </div>

          {/* Critical Results Alert Kachel */}
          <div className={`p-6 border transition-colors duration-150 ${
            criticalCount > 0
              ? 'bg-red-50 border-red-300 hover:border-red-500'
              : 'bg-white border-[#E2E8F0] hover:border-[#0055FF]'
          }`}>
            <div className="flex items-baseline justify-between">
              <span className={`text-xs font-bold uppercase tracking-wider ${criticalCount > 0 ? 'text-red-700' : 'text-[#5E6C84]'}`}>
                Critical
              </span>
              <span className={`text-4xl font-mono font-bold tabular-nums ${criticalCount > 0 ? 'text-red-700' : 'text-[#1A1C20]'}`}>
                {String(criticalCount).padStart(3, '0')}
              </span>
            </div>
            <div className="mt-4 pt-4 border-t border-opacity-30 border-current">
              <p className={`text-xs font-mono ${criticalCount > 0 ? 'text-red-600' : 'text-[#5E6C84]'}`}>
                {criticalCount > 0 ? '⚠ CRITICAL results require immediate review' : 'No critical results'}
              </p>
            </div>
          </div>
        </div>

        {/* Charts Section */}
        <div className="grid grid-cols-3 gap-4">
          {/* Sample Status Distribution */}
          <div className="bg-white border border-[#E2E8F0] p-6">
            <h2 className="text-xs font-bold uppercase tracking-wider text-[#5E6C84] mb-4">Sample Status</h2>
            {sampleStatusChart.length > 0 ? (
              <ResponsiveContainer width="100%" height={160}>
                <BarChart data={sampleStatusChart} margin={{ top: 0, right: 0, left: -20, bottom: 0 }}>
                  <XAxis dataKey="name" tick={{ fontSize: 9, fill: '#5E6C84' }} />
                  <YAxis tick={{ fontSize: 9, fill: '#5E6C84' }} allowDecimals={false} />
                  <Tooltip
                    contentStyle={{ fontSize: 11, border: '1px solid #E2E8F0' }}
                    formatter={(v) => [v, 'Count']}
                  />
                  <Bar
                    dataKey="count"
                    shape={(props: BarShapeProps) => <Rectangle {...props} radius={[2, 2, 0, 0]} />}
                  />
                </BarChart>
              </ResponsiveContainer>
            ) : (
              <p className="text-center text-[#5E6C84] font-mono text-xs py-8">NO DATA</p>
            )}
          </div>

          {/* Order Status Distribution */}
          <div className="bg-white border border-[#E2E8F0] p-6">
            <h2 className="text-xs font-bold uppercase tracking-wider text-[#5E6C84] mb-4">Order Status</h2>
            {orderStatusChart.length > 0 ? (
              <ResponsiveContainer width="100%" height={160}>
                <BarChart data={orderStatusChart} margin={{ top: 0, right: 0, left: -20, bottom: 0 }}>
                  <XAxis dataKey="name" tick={{ fontSize: 9, fill: '#5E6C84' }} />
                  <YAxis tick={{ fontSize: 9, fill: '#5E6C84' }} allowDecimals={false} />
                  <Tooltip
                    contentStyle={{ fontSize: 11, border: '1px solid #E2E8F0' }}
                    formatter={(v) => [v, 'Count']}
                  />
                  <Bar
                    dataKey="count"
                    shape={(props: BarShapeProps) => <Rectangle {...props} radius={[2, 2, 0, 0]} />}
                  />
                </BarChart>
              </ResponsiveContainer>
            ) : (
              <p className="text-center text-[#5E6C84] font-mono text-xs py-8">NO DATA</p>
            )}
          </div>

          {/* Order Priority Distribution */}
          <div className="bg-white border border-[#E2E8F0] p-6">
            <h2 className="text-xs font-bold uppercase tracking-wider text-[#5E6C84] mb-4">Order Priority</h2>
            {orderPriorityChart.length > 0 ? (
              <ResponsiveContainer width="100%" height={160}>
                <BarChart data={orderPriorityChart} margin={{ top: 0, right: 0, left: -20, bottom: 0 }}>
                  <XAxis dataKey="name" tick={{ fontSize: 9, fill: '#5E6C84' }} />
                  <YAxis tick={{ fontSize: 9, fill: '#5E6C84' }} allowDecimals={false} />
                  <Tooltip
                    contentStyle={{ fontSize: 11, border: '1px solid #E2E8F0' }}
                    formatter={(v) => [v, 'Count']}
                  />
                  <Bar
                    dataKey="count"
                    shape={(props: BarShapeProps) => <Rectangle {...props} radius={[2, 2, 0, 0]} />}
                  />
                </BarChart>
              </ResponsiveContainer>
            ) : (
              <p className="text-center text-[#5E6C84] font-mono text-xs py-8">NO DATA</p>
            )}
          </div>
        </div>

        {/* Recent Activity Table */}
        <div className="bg-white border border-[#E2E8F0]">
          <div className="px-6 py-4 border-b border-[#E2E8F0]">
            <h2 className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Recent Samples</h2>
          </div>
          {recentSamples.length === 0 ? (
            <div className="px-6 py-12 text-center">
              <p className="text-[#5E6C84] font-mono text-sm">NO DATA AVAILABLE</p>
            </div>
          ) : (
            <div className="overflow-x-auto">
              <table className="min-w-full">
                <thead className="bg-[#F4F5F7]">
                  <tr>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Sample ID</th>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Patient</th>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Status</th>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">Created</th>
                  </tr>
                </thead>
                <tbody className="bg-white">
                  {recentSamples.map((sample, idx) => (
                    <tr key={sample.id} className={`hover:bg-[#F4F5F7] transition-colors duration-100 ${idx % 2 === 1 ? 'bg-[#FAFBFC]' : ''}`}>
                      <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono font-bold text-[#1A1C20] border-b border-[#E2E8F0]">{sample.sample_id}</td>
                      <td className="px-6 py-2.5 whitespace-nowrap text-sm border-b border-[#E2E8F0]">
                        <div className="font-medium text-[#1A1C20]">{sample.patient_name}</div>
                        <div className="font-mono text-xs text-[#5E6C84]">{sample.patient_id}</div>
                      </td>
                      <td className="px-6 py-2.5 whitespace-nowrap border-b border-[#E2E8F0]">
                        <span className={`px-2 py-1 text-[10px] font-bold uppercase tracking-wider border inline-block ${getStatusColor(sample.status)}`}>
                          {sample.status.replace('_', ' ')}
                        </span>
                      </td>
                      <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono text-[#5E6C84] border-b border-[#E2E8F0]">
                        {new Date(sample.created_at).toLocaleDateString('de-DE')}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>
    </Layout>
  );
};

export default Dashboard;
