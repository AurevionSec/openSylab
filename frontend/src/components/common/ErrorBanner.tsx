interface ErrorBannerProps {
  message: string | null;
  onDismiss?: () => void;
  onRetry?: () => void;
}

export const ErrorBanner = ({ message, onDismiss, onRetry }: ErrorBannerProps) => {
  if (!message) return null;
  return (
    <div
      role="alert"
      className="bg-red-50 dark:bg-red-950 border border-red-200 dark:border-red-800 rounded p-4 flex justify-between items-start gap-4"
    >
      <p className="text-red-800 dark:text-red-200 text-sm">{message}</p>
      <div className="flex items-center gap-3 shrink-0">
        {onRetry && (
          <button
            onClick={onRetry}
            className="text-sm font-medium text-red-700 dark:text-red-300 underline hover:no-underline focus:outline-none focus-visible:ring-2 focus-visible:ring-[#0055FF] rounded"
          >
            Retry
          </button>
        )}
        {onDismiss && (
          <button
            onClick={onDismiss}
            className="text-red-500 hover:text-red-700 dark:hover:text-red-300 text-lg leading-none focus:outline-none focus-visible:ring-2 focus-visible:ring-[#0055FF] rounded"
            aria-label="Dismiss error"
          >
            ×
          </button>
        )}
      </div>
    </div>
  );
};
