# OpenSylab v0.6 - UI Extensions

## Overview

Version 0.6 introduces comprehensive user management, audit logging, and profile management features to the OpenSylab frontend.

## New Features

### 1. User Management (Admin Only)

**Route**: `/users`
**Access**: ADMIN role required

**Features**:
- List all system users with detailed information
- Create new users with role assignment
- Edit existing users (username is immutable)
- Delete users with confirmation
- Toggle user active/inactive status
- Role-based access control (ADMIN, OPERATOR, VIEWER, CUSTOM)

**Components**:
- `frontend/src/pages/Users.tsx` - Main user management page
- `frontend/src/services/users.ts` - User API service layer
- `frontend/src/types/user.ts` - User type definitions

### 2. Audit Log Viewer (Admin Only)

**Route**: `/audit-log`
**Access**: ADMIN role required

**Features**:
- View complete system audit trail
- Filter by user, action, entity type
- Adjustable result limit (25, 50, 100, 250 entries)
- Color-coded action badges for visual clarity
- Timestamp, user, action, entity, and details display

**Components**:
- `frontend/src/pages/AuditLog.tsx` - Audit log viewer
- `frontend/src/services/audit.ts` - Audit log API service
- `frontend/src/types/audit.ts` - Audit entry type definitions

### 3. User Profile & Settings

**Route**: `/profile`
**Access**: All authenticated users

**Features**:
- View account information (username, role, email, etc.)
- Display account status and last login
- Change password functionality
- Password validation (minimum 8 characters)
- Current password verification required
- Password confirmation matching

**Components**:
- `frontend/src/pages/Profile.tsx` - User profile page
- Password change form with validation

### 4. Enhanced Dashboard

**Features**:
- Multi-entity statistics (Samples, Orders, Results)
- Status breakdown by entity type
- Server-side statistics aggregation
- Real-time data from stats API endpoint

**Updated Components**:
- `frontend/src/pages/Dashboard.tsx` - Enhanced with stats API
- `frontend/src/services/stats.ts` - Statistics service
- `frontend/src/types/stats.ts` - Statistics type definitions

## Backend API Endpoints

### User Management
- `GET /api/v1/users` - List all users (admin only)
- `POST /api/v1/users` - Create new user (admin only)
- `PUT /api/v1/users/:id` - Update user (admin only)
- `DELETE /api/v1/users/:id` - Delete user (admin only)
- `GET /api/v1/users/me` - Get current user profile
- `PUT /api/v1/users/me/password` - Change password

### Audit Log
- `GET /api/v1/audit` - Get audit log entries with filters (admin only)

### Statistics
- `GET /api/v1/stats` - Get dashboard statistics

## Navigation & Routing

**Sidebar Navigation** (`frontend/src/components/Layout/Sidebar.tsx`):
- Role-based menu filtering
- Admin-only items hidden from non-admin users
- New menu items:
  - 👥 Users (admin only)
  - 📜 Audit Log (admin only)
  - 👤 Profile (all users)

**Protected Routes** (`frontend/src/components/Auth/ProtectedRoute.tsx`):
- Enhanced with `requiredRole` prop
- Role-based access control
- Access denied message for unauthorized access

**App Routing** (`frontend/src/App.tsx`):
- `/users` - User management (ADMIN required)
- `/audit-log` - Audit log viewer (ADMIN required)
- `/profile` - User profile (authenticated users)

## Security Features

### Role-Based Access Control (RBAC)
- Frontend route protection
- Backend API endpoint protection
- Menu item visibility based on user role
- Access denied feedback for unauthorized access

### Password Security
- Minimum 8 character requirement
- Current password verification for changes
- Password confirmation matching
- Server-side password hashing

### Audit Trail
- All user actions logged
- Login/logout tracking
- CRUD operations tracked
- Admin-only access to logs

## Default Credentials

**⚠️ IMPORTANT: Change immediately after first login!**

- **Username**: `admin`
- **Password**: `admin`

Use the Profile page to change your password after first login.

## Usage Guide

### For Administrators

1. **User Management**:
   - Navigate to "Users" in sidebar
   - Click "+ Create User" to add new users
   - Click "Edit" to modify user details
   - Click "Delete" to remove users (with confirmation)

2. **Audit Log**:
   - Navigate to "Audit Log" in sidebar
   - Use filters to narrow down entries
   - Export or review compliance data

3. **Profile Management**:
   - Navigate to "Profile" in sidebar
   - Click "Change Password" to update credentials
   - View account details and last login

### For Regular Users

1. **Profile Access**:
   - Navigate to "Profile" in sidebar
   - View your account information
   - Change your password as needed

## Technical Details

### TypeScript Type Safety
- Comprehensive type definitions for all entities
- Strict type checking enabled
- Payload validation types for create/update operations

### Error Handling
- Graceful error display
- API error messages surfaced to UI
- Loading states during async operations
- Form validation feedback

### UI/UX Patterns
- Modal-based forms for create/edit
- Confirmation dialogs for destructive actions
- Color-coded badges for status/roles
- Responsive table layouts
- Loading spinners for async operations

## Development

### File Structure
```
frontend/src/
├── pages/
│   ├── Users.tsx          # User management
│   ├── AuditLog.tsx       # Audit log viewer
│   ├── Profile.tsx        # User profile
│   └── Dashboard.tsx      # Enhanced dashboard
├── services/
│   ├── users.ts           # User API
│   ├── audit.ts           # Audit API
│   └── stats.ts           # Statistics API
├── types/
│   ├── user.ts            # User types
│   ├── audit.ts           # Audit types
│   └── stats.ts           # Statistics types
└── components/
    ├── Layout/
    │   └── Sidebar.tsx    # Navigation
    └── Auth/
        └── ProtectedRoute.tsx  # Route protection
```

### API Configuration
- API base URL: `http://localhost:8080/api/v1` (default)
- Configurable via `VITE_API_URL` environment variable
- JWT token authentication
- Bearer token in Authorization header

## Testing

1. **Start Backend**:
   ```bash
   ./build/bin/OpenSylab --api --api-port 8080
   ```

2. **Start Frontend**:
   ```bash
   cd frontend && npm run dev
   ```

3. **Access Application**:
   - URL: http://localhost:5173
   - Login with default credentials
   - Test all new features

## Migration Notes

### From v0.5 to v0.6

1. **New Dependencies**: None - uses existing React, TypeScript, Axios stack
2. **Breaking Changes**: None - all changes are additive
3. **Database**: Automatically creates user table if not exists
4. **Configuration**: No new configuration required

## Future Enhancements

Potential improvements for future versions:

- User profile photo upload
- Two-factor authentication (2FA)
- Advanced audit log filtering (date ranges, CSV export)
- User activity dashboard
- Session management
- Password complexity requirements
- Password reset via email
- Bulk user import/export

## Support

For issues or questions:
- GitHub Issues: https://github.com/AurevionSec/openSylab/issues
- Documentation: See project README.md

---

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Last Updated: 2026-02-02
Version: 0.6
