import { useState, useEffect, useCallback } from 'react';

interface EntityListState<T> {
  data: T[];
  total: number;
  loading: boolean;
  error: string;
  refetch: () => void;
}

/**
 * Generic hook for loading paginated entity lists.
 * fetchFn MUST return { data: T[], total: number }.
 * Service functions that return different shapes (e.g. { samples, total }) must
 * be wrapped at the callsite:
 *   useEntityList(() => getSamples(f).then(r => ({ data: r.samples, total: r.total })), [...])
 * fetchFn must be stable across renders — either defined outside the component
 * or wrapped in useCallback — otherwise it must be included in deps.
 */
export function useEntityList<T>(
  fetchFn: () => Promise<{ data: T[]; total: number }>,
  deps: readonly unknown[]
): EntityListState<T> {
  const [data, setData] = useState<T[]>([]);
  const [total, setTotal] = useState(0);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  const load = useCallback(async () => {
    setLoading(true);
    setError('');
    try {
      const result = await fetchFn();
      setData(result.data);
      setTotal(result.total);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : 'Failed to load data';
      setError(message);
    } finally {
      setLoading(false);
    }
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, deps);

  useEffect(() => {
    load();
  }, [load]);

  return { data, total, loading, error, refetch: load };
}
