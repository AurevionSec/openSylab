import type { ReactNode } from 'react';
import { Header } from './Header';
import { Sidebar } from './Sidebar';

interface LayoutProps {
  children: ReactNode;
}

export const Layout = ({ children }: LayoutProps) => {
  return (
    <div className="min-h-screen bg-[#F4F5F7]">
      <a
        href="#main-content"
        className="sr-only focus:not-sr-only focus:absolute focus:z-[200] focus:top-2 focus:left-2 focus:bg-white focus:text-[#0055FF] focus:px-4 focus:py-2 focus:rounded focus:shadow-md focus:ring-2 focus:ring-[#0055FF]"
      >
        Skip to content
      </a>
      <Sidebar />
      <div className="ml-64 flex flex-col min-h-screen">
        <Header />
        <main id="main-content" tabIndex={-1} className="flex-1 p-6 lg:p-8 page-transition">
          {children}
        </main>
      </div>
    </div>
  );
};
