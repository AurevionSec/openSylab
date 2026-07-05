import { createContext, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import type { ReactNode } from 'react';

export type ToastKind = 'success' | 'error' | 'info';

interface Toast {
  id: number;
  kind: ToastKind;
  message: string;
}

interface ToastApi {
  success: (message: string) => void;
  error: (message: string) => void;
  info: (message: string) => void;
}

// eslint-disable-next-line react-refresh/only-export-components
export const ToastContext = createContext<ToastApi | null>(null);

const AUTO_DISMISS_MS = 4000;

// Rectangular, sparing, "device-like" toasts per DESIGN.md — a small stack in the
// bottom-right. role="status" + aria-live so screen readers announce write results.
export const ToastProvider = ({ children }: { children: ReactNode }) => {
  const [toasts, setToasts] = useState<Toast[]>([]);
  const nextId = useRef(1);
  const timers = useRef<Record<number, ReturnType<typeof setTimeout>>>({});

  const dismiss = useCallback((id: number) => {
    setToasts((list) => list.filter((t) => t.id !== id));
    const timer = timers.current[id];
    if (timer) {
      clearTimeout(timer);
      delete timers.current[id];
    }
  }, []);

  const push = useCallback(
    (kind: ToastKind, message: string) => {
      const id = nextId.current++;
      setToasts((list) => [...list, { id, kind, message }]);
      timers.current[id] = setTimeout(() => dismiss(id), AUTO_DISMISS_MS);
    },
    [dismiss]
  );

  // push is stable (useCallback over the stable dismiss), so the api object is
  // built once and never needs re-mutation during render.
  const api = useMemo<ToastApi>(
    () => ({
      success: (m) => push('success', m),
      error: (m) => push('error', m),
      info: (m) => push('info', m),
    }),
    [push]
  );

  useEffect(() => {
    const pending = timers.current;
    return () => {
      Object.values(pending).forEach(clearTimeout);
    };
  }, []);

  const kindClass: Record<ToastKind, string> = {
    success: 'border-l-[#10B981] text-clinical-text',
    error: 'border-l-[#FF3B30] text-clinical-text',
    info: 'border-l-[#0055FF] text-clinical-text',
  };

  return (
    <ToastContext.Provider value={api}>
      {children}
      <div
        role="status"
        aria-live="polite"
        className="fixed bottom-4 right-4 z-[100] flex flex-col gap-2 w-80 max-w-[calc(100vw-2rem)]"
      >
        {toasts.map((t) => (
          <div
            key={t.id}
            className={`flex items-start justify-between gap-3 bg-white dark:bg-dark-surface border border-clinical-border dark:border-white/10 border-l-4 ${kindClass[t.kind]} dark:text-white rounded px-4 py-3 shadow-sm animate-snap-in`}
          >
            <span className="text-sm">{t.message}</span>
            <button
              onClick={() => dismiss(t.id)}
              aria-label="Dismiss notification"
              className="text-lg leading-none text-clinical-secondary hover:text-clinical-text dark:hover:text-white focus:outline-none focus-visible:ring-2 focus-visible:ring-[#0055FF] rounded"
            >
              ×
            </button>
          </div>
        ))}
      </div>
    </ToastContext.Provider>
  );
};
