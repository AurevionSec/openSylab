import { useState } from 'react';
import { useAuth } from '../../context/AuthContext';
import { useLocation, Link, useNavigate } from 'react-router-dom';

export const Header = () => {
  const { logout } = useAuth();
  const location = useLocation();
  const navigate = useNavigate();
  const [searchQuery, setSearchQuery] = useState('');

  const handleSearch = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key !== 'Enter') return;
    const q = searchQuery.trim();
    if (!q) return;
    // Route by the actual ID conventions: orders are "ORD-…", results are
    // "R<digit>…"; everything else searches samples.
    const upper = q.toUpperCase();
    if (upper.startsWith('ORD')) {
      navigate(`/orders?q=${encodeURIComponent(q)}`);
    } else if (/^R\d/.test(upper)) {
      navigate(`/results?q=${encodeURIComponent(q)}`);
    } else {
      navigate(`/samples?q=${encodeURIComponent(q)}`);
    }
    setSearchQuery('');
  };

  const handleLogout = () => {
    logout();
    navigate('/login', { replace: true });
  };

  // Generate breadcrumbs from current path
  const getBreadcrumbs = () => {
    const path = location.pathname;

    // Map routes to display names
    const routeNames: Record<string, string> = {
      '/': 'Dashboard',
      '/samples': 'Samples',
      '/orders': 'Orders',
      '/results': 'Results',
      '/audit-log': 'Audit Log',
      '/users': 'Users',
      '/profile': 'Profile',
      '/import': 'Import',
    };

    return routeNames[path] || 'Dashboard';
  };

  return (
    <header className="h-16 bg-white border-b border-gray-200 flex items-center justify-between px-8">
      {/* Breadcrumbs - Industrial Wayfinding */}
      <div className="flex items-center text-sm font-medium text-gray-500 font-mono">
        <Link to="/" className="hover:text-[#0055FF] transition-colors">
          <svg className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" />
          </svg>
        </Link>

        <span className="mx-2 text-gray-300">/</span>
        <span className="text-gray-900">{getBreadcrumbs()}</span>
      </div>

      {/* Right Side: Search + Logout */}
      <div className="flex items-center space-x-4">
        {/* Search Bar */}
        <div className="flex items-center border border-gray-300 rounded-sm px-3 py-1.5 w-64 gap-2 focus-within:border-[#0055FF] focus-within:ring-1 focus-within:ring-[#0055FF] bg-white">
          <svg className="h-4 w-4 text-gray-400 shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
          </svg>
          <input
            type="text"
            className="flex-1 text-sm outline-none placeholder-gray-400 bg-transparent min-w-0"
            placeholder="ID suchen… (Enter)"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            onKeyDown={handleSearch}
          />
        </div>

        {/* Logout Button */}
        <button
          onClick={handleLogout}
          className="text-sm font-medium text-gray-500 hover:text-red-600 transition-colors"
        >
          Logout
        </button>
      </div>
    </header>
  );
};
