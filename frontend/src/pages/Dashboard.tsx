import { useEffect, useState } from 'react';
import { Layout } from '../components/Layout/Layout';
import { Card } from '../components/common/Card';
import { getDashboardStats } from '../services/stats';
import { getSamples } from '../services/samples';
import { getOrders } from '../services/orders';
import { getResults } from '../services/results';
import type { Sample } from '../types/sample';
import type { DashboardStats } from '../types/stats';
import { SAMPLE_STATUSES, STATUS_COLORS } from '../utils/constants';

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

  return (
    <Layout>
      <div className="space-y-6">
        <div>
          <h2 className="text-3xl font-bold text-gray-900">Dashboard</h2>
          <p className="text-gray-600 mt-1">Overview of laboratory operations and activities</p>
        </div>

        {/* Entity Overview Cards */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <Card title="Total Samples">
            <div className="text-4xl font-bold text-blue-600">{stats?.samples.total || 0}</div>
            <p className="text-gray-600 mt-2">Samples in system</p>
            {stats && stats.samples.by_status.length > 0 && (
              <div className="mt-4 space-y-1">
                {stats.samples.by_status.slice(0, 3).map((s) => (
                  <div key={s.status} className="flex justify-between text-sm">
                    <span className="text-gray-600">{s.status}:</span>
                    <span className="font-medium">{s.count}</span>
                  </div>
                ))}
              </div>
            )}
          </Card>

          <Card title="Total Orders">
            <div className="text-4xl font-bold text-green-600">{stats?.orders.total || 0}</div>
            <p className="text-gray-600 mt-2">Orders processed</p>
            {stats && stats.orders.by_status.length > 0 && (
              <div className="mt-4 space-y-1">
                {stats.orders.by_status.slice(0, 3).map((s) => (
                  <div key={s.status} className="flex justify-between text-sm">
                    <span className="text-gray-600">{s.status}:</span>
                    <span className="font-medium">{s.count}</span>
                  </div>
                ))}
              </div>
            )}
          </Card>

          <Card title="Total Results">
            <div className="text-4xl font-bold text-purple-600">{stats?.results.total || 0}</div>
            <p className="text-gray-600 mt-2">Test results</p>
            {stats && stats.results.by_status.length > 0 && (
              <div className="mt-4 space-y-1">
                {stats.results.by_status.slice(0, 3).map((s) => (
                  <div key={s.status} className="flex justify-between text-sm">
                    <span className="text-gray-600">{s.status}:</span>
                    <span className="font-medium">{s.count}</span>
                  </div>
                ))}
              </div>
            )}
          </Card>
        </div>

        {/* Sample Status Breakdown */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {statusCounts.map(({ status, count, label }) => (
            <Card key={status} title={label}>
              <div className="text-4xl font-bold text-gray-900">{count}</div>
              <div className="mt-2">
                <span className={`inline-block px-3 py-1 rounded-full text-sm font-medium ${STATUS_COLORS[status]}`}>
                  {status.replace('_', ' ')}
                </span>
              </div>
            </Card>
          ))}
        </div>

        <Card title="Recent Samples">
          {recentSamples.length === 0 ? (
            <p className="text-gray-500 text-center py-8">No samples found</p>
          ) : (
            <div className="overflow-x-auto">
              <table className="min-w-full divide-y divide-gray-200">
                <thead>
                  <tr>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Sample ID
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Patient
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Status
                    </th>
                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Created
                    </th>
                  </tr>
                </thead>
                <tbody className="bg-white divide-y divide-gray-200">
                  {recentSamples.map((sample) => (
                    <tr key={sample.id} className="hover:bg-gray-50">
                      <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                        {sample.sample_id}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                        <div>
                          <div className="font-medium">{sample.patient_name}</div>
                          <div className="text-gray-500">{sample.patient_id}</div>
                        </div>
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap">
                        <span className={`px-3 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${STATUS_COLORS[sample.status]}`}>
                          {SAMPLE_STATUSES[sample.status]}
                        </span>
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {new Date(sample.created_at).toLocaleDateString()}
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

export default Dashboard;
