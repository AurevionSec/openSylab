import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../../context/AuthContext';

export const Sidebar = () => {
  const location = useLocation();
  const { user, logout } = useAuth();

  const isActive = (path: string) => location.pathname === path;

  const canImport = user?.role === 'ADMIN' || user?.role === 'OPERATOR';

  const navItems = [
    {
      name: 'Dashboard',
      path: '/',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z" />
        </svg>
      ),
    },
    {
      name: 'Samples',
      path: '/samples',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19.428 15.428a2 2 0 00-1.022-.547l-2.387-.477a6 6 0 00-3.86.517l-.318.158a6 6 0 01-3.86.517L6.05 15.21a2 2 0 00-1.806.547M8 4h8l-1 1v5.172a2 2 0 00.586 1.414l5 5c1.26 1.26.367 3.414-1.415 3.414H4.828c-1.782 0-2.674-2.154-1.414-3.414l5-5A2 2 0 009 10.172V5L8 4z" />
        </svg>
      ),
      badge: '24',
    },
    {
      name: 'Orders',
      path: '/orders',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 5H7a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2V7a2 2 0 00-2-2h-2M9 5a2 2 0 002 2h2a2 2 0 002-2M9 5a2 2 0 012-2h2a2 2 0 012 2" />
        </svg>
      ),
    },
    {
      name: 'Results',
      path: '/results',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z" />
        </svg>
      ),
    },
  ];

  const importItem = canImport ? [{
    name: 'Import',
    path: '/import',
    icon: (
      <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-8l-4-4m0 0L8 8m4-4v12" />
      </svg>
    ),
  }] : [];

  const isAdmin = user?.role === 'ADMIN';
  const systemItems = isAdmin ? [
    {
      name: 'Audit Log',
      path: '/audit-log',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
      ),
    },
    {
      name: 'Users',
      path: '/users',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 4.354a4 4 0 110 5.292M15 21H3v-1a6 6 0 0112 0v1zm0 0h6v-1a6 6 0 00-9-5.197M13 7a4 4 0 11-8 0 4 4 0 018 0z" />
        </svg>
      ),
    },
    {
      name: 'Profile',
      path: '/profile',
      icon: (
        <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
        </svg>
      ),
    },
  ] : []

  const getUserInitials = () => {
    if (!user?.username) return 'U';
    return user.username.substring(0, 2).toUpperCase();
  };

  const getRoleDisplay = () => {
    if (!user?.role) return 'User';
    return user.role.charAt(0) + user.role.slice(1).toLowerCase();
  };

  return (
    <aside className="fixed inset-y-0 left-0 w-64 bg-[#1A1C20] border-r border-gray-800 flex flex-col z-50 transition-all duration-300">
      {/* Header */}
      <div className="h-16 flex items-center px-6 border-b border-gray-800 bg-[#15171a]">
        {/* Helix Engine Emblem */}
        <div className="flex-shrink-0 mr-3 mt-1">
          <img
            src="/assets/brand-helix-core.png"
            alt="OpenSylab Helix Engine"
            className="h-9 w-auto object-contain animate-reactor"
          />
        </div>

        <div className="font-['Inter'] font-bold text-xl tracking-tight text-white flex items-center">
          OpenSylab
          {/* Version Badge */}
          <span className="ml-2 px-1.5 py-0.5 rounded text-[10px] font-mono font-bold bg-[#2563EB]/20 text-[#3B82F6] border border-[#2563EB]/30 uppercase tracking-widest">
            v0.6
          </span>
        </div>
      </div>

      {/* Main Navigation */}
      <nav className="flex-1 overflow-y-auto py-6 space-y-1">
        {[...navItems, ...importItem].map((item) => (
          <Link
            key={item.path}
            to={item.path}
            className={`group flex items-center px-5 py-3 text-sm font-medium transition-colors border-l-4 ${
              isActive(item.path)
                ? 'text-white bg-gray-800 border-[#2563EB]'
                : 'text-gray-400 hover:text-white hover:bg-gray-800 border-transparent'
            }`}
          >
            <span
              className={`mr-3 ${
                isActive(item.path)
                  ? 'text-[#2563EB]'
                  : 'text-gray-500 group-hover:text-white transition-colors'
              }`}
            >
              {item.icon}
            </span>
            {item.name}
            
          </Link>
        ))}

        {/* System Section */}
        <div className="pt-4 pb-2">
          <div className="px-5 text-xs font-semibold text-gray-600 uppercase tracking-wider">
            System
          </div>
        </div>

        {systemItems.map((item) => (
          <Link
            key={item.path}
            to={item.path}
            className={`group flex items-center px-5 py-3 text-sm font-medium transition-colors border-l-4 ${
              isActive(item.path)
                ? 'text-white bg-gray-800 border-[#2563EB]'
                : 'text-gray-400 hover:text-white hover:bg-gray-800 border-transparent'
            }`}
          >
            <span
              className={`mr-3 ${
                isActive(item.path)
                  ? 'text-[#2563EB]'
                  : 'text-gray-500 group-hover:text-white transition-colors'
              }`}
            >
              {item.icon}
            </span>
            {item.name}
          </Link>
        ))}
      </nav>

      {/* User Footer */}
      <div className="border-t border-gray-800 p-4 bg-[#15171a]">
        <div className="flex items-center">
          <div className="flex-shrink-0">
            <div className="h-8 w-8 rounded bg-gray-700 flex items-center justify-center text-white font-bold text-xs">
              {getUserInitials()}
            </div>
          </div>
          <div className="ml-3">
            <p className="text-sm font-medium text-white">{user?.username || 'User'}</p>
            <p className="text-xs text-gray-500">{getRoleDisplay()}</p>
          </div>
          <button
            onClick={logout}
            className="ml-auto text-gray-500 hover:text-white transition-colors"
            title="Logout"
          >
            <svg className="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth="2"
                d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1"
              />
            </svg>
          </button>
        </div>
      </div>
    </aside>
  );
};
