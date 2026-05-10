interface ErrorBannerProps {
  message: string | null;
  onDismiss?: () => void;
}

export const ErrorBanner = ({ message, onDismiss }: ErrorBannerProps) => {
  if (!message) return null;
  return (
    <div className="bg-red-50 dark:bg-red-950 border border-red-200 dark:border-red-800 rounded p-4 flex justify-between items-start">
      <p className="text-red-800 dark:text-red-200 text-sm">{message}</p>
      {onDismiss && (
        <button
          onClick={onDismiss}
          className="ml-4 text-red-500 hover:text-red-700 dark:hover:text-red-300 text-lg leading-none"
          aria-label="Dismiss error"
        >
          ×
        </button>
      )}
    </div>
  );
};
