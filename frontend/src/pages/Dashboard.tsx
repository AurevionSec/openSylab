import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { getDashboardStats } from '../services/stats';
import { getSamples } from '../services/samples';
import { getOrders } from '../services/orders';
import { getResults } from '../services/results';
import type { Sample } from '../types/sample';
import type { DashboardStats } from '../types/stats';
import { useDocumentTitle } from '../hooks/useDocumentTitle';

export const Dashboard = () => {
  useDocumentTitle({ module: 'Dashboard' });
  const [stats, setStats] = useState<DashboardStats | null>(null);
  const [samples, setSamples] = useState<Sample[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    const fetchData = async () => {
      try {
        // Try to fetch stats, but continue even if it fails
        let statsData: DashboardStats | null = null;
        try {
          statsData = await getDashboardStats();
        } catch (statsErr) {
          console.warn('Stats endpoint not available, using fallback');
          // Fallback: create empty stats structure
          statsData = {
            samples: { entity_type: 'samples', total: 0, by_status: [] },
            orders: { entity_type: 'orders', total: 0, by_status: [] },
            results: { entity_type: 'results', total: 0, by_status: [] },
          };
        }

        // Fetch all entity data
        const samplesData = await getSamples({ limit: 1000 });
        const ordersData = await getOrders({ limit: 1000 });
        const resultsData = await getResults({ limit: 1000 });

        // If stats weren't available, calculate from actual data
        if (statsData && statsData.samples.total === 0 && samplesData.samples.length > 0) {
          statsData.samples.total = samplesData.samples.length;
          const sampleStatusCounts = new Map<string, number>();
          samplesData.samples.forEach(s => {
            sampleStatusCounts.set(s.status, (sampleStatusCounts.get(s.status) || 0) + 1);
          });
          statsData.samples.by_status = Array.from(sampleStatusCounts.entries()).map(([status, count]) => ({
            status,
            count,
          }));
        }

        if (statsData && statsData.orders.total === 0 && ordersData.orders.length > 0) {
          statsData.orders.total = ordersData.orders.length;
          const orderStatusCounts = new Map<string, number>();
          ordersData.orders.forEach(o => {
            orderStatusCounts.set(o.status, (orderStatusCounts.get(o.status) || 0) + 1);
          });
          statsData.orders.by_status = Array.from(orderStatusCounts.entries()).map(([status, count]) => ({
            status,
            count,
          }));
        }

        if (statsData && statsData.results.total === 0 && resultsData.results.length > 0) {
          statsData.results.total = resultsData.results.length;
          const resultStatusCounts = new Map<string, number>();
          resultsData.results.forEach(r => {
            resultStatusCounts.set(r.status, (resultStatusCounts.get(r.status) || 0) + 1);
          });
          statsData.results.by_status = Array.from(resultStatusCounts.entries()).map(([status, count]) => ({
            status,
            count,
          }));
        }

        setStats(statsData);
        setSamples(samplesData.samples);
      } catch (err) {
        console.error('Dashboard error:', err);
        setError('Failed to load dashboard data');
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  const recentSamples = samples
    .sort((a, b) => new Date(b.created_at).getTime() - new Date(a.created_at).getTime())
    .slice(0, 5);

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
        <div className="bg-red-50 border border-red-200 rounded-lg p-4">
          <p className="text-red-800">{error}</p>
        </div>
      </Layout>
    );
  }

  // Status badge color mapping (Neo-Clinical style with Subtle Fill)
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
      'ENTERED': 'bg-blue-50 text-blue-700 border border-blue-200',
      'REJECTED': 'bg-red-50 text-red-700 border border-red-200',
      'NORMAL': 'bg-green-50 text-green-700 border border-green-200',
      'LOW': 'bg-yellow-50 text-yellow-800 border border-yellow-200',
      'HIGH': 'bg-orange-50 text-orange-700 border border-orange-200',
      'CRITICAL': 'bg-red-50 text-red-700 border border-red-200',
      'EMERGENCY': 'bg-red-50 text-red-700 border border-red-200',
      'URGENT': 'bg-orange-50 text-orange-700 border border-orange-200',
      'NORMAL_PRIORITY': 'bg-gray-100 text-gray-700 border border-gray-300',
    };
    return statusMap[status] || 'bg-gray-100 text-gray-600 border border-gray-300';
  };

  return (
    <Layout>
      <div className="space-y-4">
        {/* Header - Neo-Clinical style */}
        <div className="border-b border-[#E2E8F0] pb-4">
          <h1 className="text-2xl font-bold text-[#1A1C20] tracking-tight uppercase">System Overview</h1>
          <p className="text-[#5E6C84] text-sm mt-1 font-mono">Real-time laboratory operations monitoring</p>
        </div>

        {/* Bento Box Grid - Primary Metrics */}
        <div className="grid grid-cols-3 gap-4">
          {/* Samples Metric */}
          <div className="bg-white border border-[#E2E8F0] p-6 hover:border-[#0055FF] transition-colors duration-150">
            <div className="flex items-baseline justify-between">
              <span className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Samples</span>
              <span className="text-4xl font-mono font-bold text-[#1A1C20] tabular-nums">
                {String(stats?.samples.total || 0).padStart(3, '0')}
              </span>
            </div>
            {stats && stats.samples.by_status.length > 0 && (
              <div className="mt-4 space-y-2 pt-4 border-t border-[#E2E8F0]">
                {stats.samples.by_status.slice(0, 3).map((s) => (
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

          {/* Orders Metric */}
          <div className="bg-white border border-[#E2E8F0] p-6 hover:border-[#0055FF] transition-colors duration-150">
            <div className="flex items-baseline justify-between">
              <span className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Orders</span>
              <span className="text-4xl font-mono font-bold text-[#1A1C20] tabular-nums">
                {String(stats?.orders.total || 0).padStart(3, '0')}
              </span>
            </div>
            {stats && stats.orders.by_status.length > 0 && (
              <div className="mt-4 space-y-2 pt-4 border-t border-[#E2E8F0]">
                {stats.orders.by_status.slice(0, 3).map((s) => (
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

          {/* Results Metric */}
          <div className="bg-white border border-[#E2E8F0] p-6 hover:border-[#0055FF] transition-colors duration-150">
            <div className="flex items-baseline justify-between">
              <span className="text-xs font-bold uppercase tracking-wider text-[#5E6C84]">Results</span>
              <span className="text-4xl font-mono font-bold text-[#1A1C20] tabular-nums">
                {String(stats?.results.total || 0).padStart(3, '0')}
              </span>
            </div>
            {stats && stats.results.by_status.length > 0 && (
              <div className="mt-4 space-y-2 pt-4 border-t border-[#E2E8F0]">
                {stats.results.by_status.slice(0, 3).map((s) => (
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
        </div>

        {/* Recent Activity Table - Dense, Clinical Style */}
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
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                      Sample ID
                    </th>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                      Patient
                    </th>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                      Status
                    </th>
                    <th className="px-6 py-3 text-left text-[10px] font-bold text-[#5E6C84] uppercase tracking-wider border-b border-[#E2E8F0]">
                      Created
                    </th>
                  </tr>
                </thead>
                <tbody className="bg-white">
                  {recentSamples.map((sample, idx) => (
                    <tr
                      key={sample.id}
                      className={`hover:bg-[#F4F5F7] transition-colors duration-100 ${idx % 2 === 1 ? 'bg-[#FAFBFC]' : ''}`}
                    >
                      <td className="px-6 py-2.5 whitespace-nowrap text-sm font-mono font-bold text-[#1A1C20] border-b border-[#E2E8F0]">
                        {sample.sample_id}
                      </td>
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
