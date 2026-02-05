import { useState } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../../context/AuthContext';

interface NavItem {
  name: string;
  path: string;
  icon: string;
  label: string;
  adminOnly?: boolean;
}

const navItems: NavItem[] = [
  { name: 'Dashboard', path: '/', icon: '■', label: 'OVERVIEW', adminOnly: false },
  { name: 'Samples', path: '/samples', icon: '◆', label: 'SAMPLES', adminOnly: false },
  { name: 'Orders', path: '/orders', icon: '▣', label: 'ORDERS', adminOnly: false },
  { name: 'Results', path: '/results', icon: '◈', label: 'RESULTS', adminOnly: false },
  { name: 'Users', path: '/users', icon: '▲', label: 'USERS', adminOnly: true },
  { name: 'Audit Log', path: '/audit-log', icon: '▼', label: 'AUDIT', adminOnly: true },
  { name: 'Profile', path: '/profile', icon: '●', label: 'PROFILE', adminOnly: false },
];

export const Sidebar = () => {
  const location = useLocation();
  const { user } = useAuth();
  const [isExpanded, setIsExpanded] = useState(false);

  // Filter navigation items based on user role
  const visibleNavItems = navItems.filter((item) => {
    if (item.adminOnly && user?.role !== 'ADMIN') {
      return false;
    }
    return true;
  });

  return (
    <aside
      className={`bg-[#16181D] border-r border-[#1A1C20] min-h-screen transition-all duration-200 ${
        isExpanded ? 'w-56' : 'w-16'
      }`}
      onMouseEnter={() => setIsExpanded(true)}
      onMouseLeave={() => setIsExpanded(false)}
    >
      {/* Logo/Branding Area */}
      <div className="h-16 flex items-center justify-center border-b border-[#1A1C20]">
        <div className="text-[#0055FF] font-mono font-bold text-sm">
          {isExpanded ? 'OPENSYLAB' : 'OS'}
        </div>
      </div>

      {/* Navigation */}
      <nav className="py-4">
        {visibleNavItems.map((item) => {
          const isActive = location.pathname === item.path;
          return (
            <Link
              key={item.path}
              to={item.path}
              className={`
                group relative flex items-center h-12
                transition-all duration-150
                ${isActive
                  ? 'bg-[#0055FF]/10 border-l-2 border-[#0055FF]'
                  : 'border-l-2 border-transparent hover:bg-[#1A1C20] hover:border-l-2 hover:border-[#5E6C84]'
                }
              `}
              aria-current={isActive ? 'page' : undefined}
            >
              {/* Icon */}
              <div className={`
                w-16 flex items-center justify-center text-xl font-mono
                ${isActive ? 'text-[#0055FF]' : 'text-[#5E6C84] group-hover:text-[#E0E0E0]'}
              `}>
                {item.icon}
              </div>

              {/* Label (visible when expanded) */}
              {isExpanded && (
                <span className={`
                  font-mono text-xs font-bold tracking-wider uppercase
                  transition-opacity duration-150
                  ${isActive ? 'text-[#0055FF]' : 'text-[#5E6C84] group-hover:text-[#E0E0E0]'}
                `}>
                  {item.label}
                </span>
              )}

              {/* Tooltip (visible when collapsed) */}
              {!isExpanded && (
                <div className="
                  absolute left-16 ml-2 px-3 py-1.5
                  bg-[#1A1C20] border border-[#5E6C84]
                  text-[#E0E0E0] text-xs font-mono font-bold uppercase tracking-wider
                  opacity-0 group-hover:opacity-100 pointer-events-none
                  transition-opacity duration-150
                  whitespace-nowrap z-50
                ">
                  {item.label}
                  {/* Arrow */}
                  <div className="absolute left-0 top-1/2 -translate-x-1 -translate-y-1/2
                                  w-2 h-2 bg-[#1A1C20] border-l border-b border-[#5E6C84]
                                  transform rotate-45"></div>
                </div>
              )}

              {/* Active indicator line */}
              {isActive && (
                <div className="absolute right-0 top-0 bottom-0 w-0.5 bg-[#0055FF]"></div>
              )}
            </Link>
          );
        })}
      </nav>

      {/* Bottom System Info (when expanded) */}
      {isExpanded && (
        <div className="absolute bottom-0 left-0 right-0 p-4 border-t border-[#1A1C20]">
          <div className="text-[10px] font-mono text-[#5E6C84] uppercase tracking-wider">
            <div>v0.6.0</div>
            <div className="text-[8px] mt-1">{user?.role}</div>
          </div>
        </div>
      )}
    </aside>
  );
};
