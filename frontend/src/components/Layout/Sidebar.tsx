import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../../context/AuthContext';

interface NavItem {
  name: string;
  path: string;
  icon: string;
  adminOnly?: boolean;
}

const navItems: NavItem[] = [
  { name: 'Dashboard', path: '/', icon: '📊' },
  { name: 'Samples', path: '/samples', icon: '🧪' },
  { name: 'Orders', path: '/orders', icon: '📋' },
  { name: 'Results', path: '/results', icon: '🔬' },
  { name: 'Users', path: '/users', icon: '👥', adminOnly: true },
  { name: 'Audit Log', path: '/audit-log', icon: '📜', adminOnly: true },
  { name: 'Profile', path: '/profile', icon: '👤' },
];

export const Sidebar = () => {
  const location = useLocation();
  const { user } = useAuth();

  // Filter navigation items based on user role
  const visibleNavItems = navItems.filter((item) => {
    if (item.adminOnly && user?.role !== 'ADMIN') {
      return false;
    }
    return true;
  });

  return (
    <aside className="w-64 bg-gray-50 border-r border-gray-200 min-h-screen">
      <nav className="p-4 space-y-2">
        {visibleNavItems.map((item) => {
          const isActive = location.pathname === item.path;
          return (
            <Link
              key={item.path}
              to={item.path}
              className={`flex items-center px-4 py-3 rounded-lg transition-colors duration-200 ${
                isActive
                  ? 'bg-blue-100 text-blue-700 font-medium'
                  : 'text-gray-700 hover:bg-gray-100'
              }`}
              aria-current={isActive ? 'page' : undefined}
            >
              <span className="text-xl mr-3">{item.icon}</span>
              <span>{item.name}</span>
            </Link>
          );
        })}
      </nav>
    </aside>
  );
};
