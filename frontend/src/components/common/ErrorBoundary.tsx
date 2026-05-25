import { Component } from 'react';
import type { ReactNode, ErrorInfo } from 'react';

interface Props {
  children: ReactNode;
}

interface State {
  hasError: boolean;
  message: string;
}

export class ErrorBoundary extends Component<Props, State> {
  state: State = { hasError: false, message: '' };

  static getDerivedStateFromError(error: unknown): State {
    const message = error instanceof Error ? error.message : String(error);
    return { hasError: true, message };
  }

  componentDidCatch(_error: unknown, info: ErrorInfo) {
    console.error('[ErrorBoundary]', info.componentStack);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="min-h-screen flex items-center justify-center bg-gray-50">
          <div className="max-w-md w-full p-8 bg-white border border-red-200 shadow-sm">
            <h1 className="text-lg font-bold text-red-700 mb-2 uppercase tracking-wider">Unexpected Error</h1>
            <p className="text-sm text-gray-600 font-mono break-words">{this.state.message}</p>
            <button
              className="mt-6 px-4 py-2 text-xs font-bold uppercase tracking-wider bg-gray-900 text-white hover:bg-gray-700"
              onClick={() => window.location.reload()}
            >
              Reload
            </button>
          </div>
        </div>
      );
    }
    return this.props.children;
  }
}
