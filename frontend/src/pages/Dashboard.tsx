import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { getDashboardStats } from '../services/stats';
import { getSamples } from '../services/samples';
import { getOrders } from '../services/orders';
import { getResults } from '../services/results';
import type { Sample } from '../types/sample';
import type { DashboardStats } from '../types/stats';
import { SAMPLE_STATUSES } from '../utils/constants';

interface StatusCount {
  status: keyof typeof SAMPLE_STATUSES;
  count: number;
  label: string;
}

export const Dashboard = () => {
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
          console.warn('Stats endpoint not available, using fallback', statsErr);
          // Fallback: create empty stats structure
          statsData = {
            samples: { entity_type: 'samples', total: 0, by_status: [] },
            orders: { entity_type: 'orders', total: 0, by_status: [] },
            results: { entity_type: 'results', total: 0, by_status: [] },
          };
        }

        // Fetch all entity data
        const [samplesData, ordersData, resultsData] = await Promise.all([
          getSamples({ limit: 1000 }),
          getOrders({ limit: 1000 }),
          getResults({ limit: 1000 }),
        ]);

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
        setError('Failed to load dashboard data');
        console.error(err);
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  const statusCounts: StatusCount[] = Object.keys(SAMPLE_STATUSES).map((status) => {
    const statusKey = status as keyof typeof SAMPLE_STATUSES;
    return {
      status: statusKey,
      count: samples.filter((s) => s.status === status).length,
      label: SAMPLE_STATUSES[statusKey],
    };
  });

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

  // Status badge color mapping (Neo-Clinical style)
  const getStatusColor = (status: string) => {
    const statusMap: Record<string, string> = {
      'REGISTERED': 'border-[#0055FF] text-[#0055FF] bg-[#0055FF]/5',
      'IN_ANALYSIS': 'border-[#CCFF00] text-[#1A1C20] bg-[#CCFF00]/10',
      'ANALYZED': 'border-[#5E6C84] text-[#5E6C84] bg-[#5E6C84]/5',
      'VALIDATED': 'border-[#10B981] text-[#10B981] bg-[#10B981]/5',
      'ARCHIVED': 'border-[#5E6C84] text-[#5E6C84] bg-[#5E6C84]/5',
      'PENDING': 'border-[#CCFF00] text-[#1A1C20] bg-[#CCFF00]/10',
      'COMPLETED': 'border-[#10B981] text-[#10B981] bg-[#10B981]/5',
      'REQUESTED': 'border-[#0055FF] text-[#0055FF] bg-[#0055FF]/5',
      'IN_PROGRESS': 'border-[#CCFF00] text-[#1A1C20] bg-[#CCFF00]/10',
      'CANCELLED': 'border-[#FF3B30] text-[#FF3B30] bg-[#FF3B30]/5',
    };
    return statusMap[status] || 'border-[#5E6C84] text-[#5E6C84] bg-[#5E6C84]/5';
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
