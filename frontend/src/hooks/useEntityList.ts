import { useState, useEffect, useCallback, useRef } from 'react';

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

  const loadFnRef = useRef(fetchFn);
  // Keep the ref pointing at the latest fetchFn without writing it during
  // render (refs must only be mutated in effects/handlers).
  useEffect(() => {
    loadFnRef.current = fetchFn;
  });

  const mountedRef = useRef(true);
  useEffect(() => {
    mountedRef.current = true;
    return () => { mountedRef.current = false; };
  }, []);

  const refetch = useCallback(() => {
    setLoading(true);
    setError('');
    loadFnRef.current()
      .then(result => {
        if (mountedRef.current) { setData(result.data); setTotal(result.total); }
      })
      .catch((err: unknown) => {
        if (mountedRef.current) {
          const message = err instanceof Error ? err.message : 'Failed to load data';
          setError(message);
        }
      })
      .finally(() => { if (mountedRef.current) setLoading(false); });
   
  }, []);

  useEffect(() => {
    let cancelled = false;
    setLoading(true);
    setError('');
    loadFnRef.current()
      .then(result => {
        if (!cancelled) { setData(result.data); setTotal(result.total); }
      })
      .catch((err: unknown) => {
        if (!cancelled) {
          const message = err instanceof Error ? err.message : 'Failed to load data';
          setError(message);
        }
      })
      .finally(() => { if (!cancelled) setLoading(false); });
    return () => { cancelled = true; };
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, deps);

  return { data, total, loading, error, refetch };
}
