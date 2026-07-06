import { useCallback } from 'react';
import { useSearchParams } from 'react-router-dom';

/**
 * List-page filter/pagination state backed by the URL query string, so filters
 * and the current page survive navigation, are deep-linkable, and work with the
 * browser back button. Setting a filter resets the page; history uses replace so
 * a filter tweak doesn't spam the back stack.
 */
export function useListParams() {
  const [params, setParams] = useSearchParams();

  const get = (key: string, def = ''): string => params.get(key) ?? def;

  const page = Math.max(1, parseInt(params.get('page') ?? '1', 10) || 1);

  const setParam = useCallback(
    (key: string, value: string, resetPage = true) => {
      setParams(
        (prev) => {
          const next = new URLSearchParams(prev);
          if (value) next.set(key, value);
          else next.delete(key);
          if (resetPage) next.delete('page');
          return next;
        },
        { replace: true }
      );
    },
    [setParams]
  );

  const setPage = useCallback(
    (p: number) => {
      setParams(
        (prev) => {
          const next = new URLSearchParams(prev);
          if (p > 1) next.set('page', String(p));
          else next.delete('page');
          return next;
        },
        { replace: true }
      );
    },
    [setParams]
  );

  return { get, page, setParam, setPage };
}
