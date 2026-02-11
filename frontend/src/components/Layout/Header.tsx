import { useAuth } from '../../context/AuthContext';
import { useLocation, Link } from 'react-router-dom';

export const Header = () => {
  const { logout } = useAuth();
  const location = useLocation();

  const handleLogout = () => {
    logout();
    window.location.href = '/login';
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
      '/audit': 'Audit Log',
      '/users': 'Users',
      '/profile': 'Profile',
    };

    return routeNames[path] || 'Dashboard';
  };

  return (
    <header className="h-16 bg-white border-b border-gray-200 flex items-center justify-between px-8">
      {/* Breadcrumbs - Industrial Wayfinding */}
      <div className="flex items-center text-sm font-medium text-gray-500 font-mono">
        <Link to="/" className="hover:text-[#2563EB] transition-colors">
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
        <div className="relative">
          <span className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none">
            <svg className="h-4 w-4 text-gray-400" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
            </svg>
          </span>
          <input
            type="text"
            className="pl-10 pr-4 py-1.5 border border-gray-300 rounded-sm text-sm focus:border-[#2563EB] focus:ring-1 focus:ring-[#2563EB] w-64 placeholder-gray-400 font-mono"
            placeholder="Search ID (e.g. S001)..."
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
