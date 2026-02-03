# OpenSylab LIMS - Frontend

Modern React TypeScript frontend for the OpenSylab Laboratory Information Management System.

**Version:** 0.6.0
**Last Updated:** 2026-02-03
**Status:** Production-Ready

## Tech Stack

- **Framework**: React 18 with TypeScript
- **Build Tool**: Vite 5 with Hot Module Replacement
- **Routing**: React Router v6
- **HTTP Client**: Axios with JWT interceptors
- **Styling**: Tailwind CSS 3
- **State Management**: React Context API (AuthContext)
- **Package Manager**: npm

## Project Structure

```
src/
├── components/
│   ├── Layout/          # Layout components (Header, Sidebar, Layout)
│   ├── Auth/            # Authentication components (Login, ProtectedRoute)
│   └── common/          # Reusable UI components (Button, Input, Card, Modal)
├── pages/               # Page components
│   ├── Dashboard.tsx    # Dashboard with statistics
│   ├── Samples.tsx      # Sample management
│   ├── Orders.tsx       # Order management
│   ├── Results.tsx      # Result management
│   ├── Users.tsx        # User management (Admin only)
│   ├── AuditLog.tsx     # Audit log viewer (Admin only)
│   ├── Profile.tsx      # User profile with password change
│   └── Login.tsx        # Login page
├── services/            # API services
│   ├── auth.ts          # Authentication service (login, logout)
│   ├── samples.ts       # Sample CRUD operations
│   ├── orders.ts        # Order CRUD operations
│   ├── results.ts       # Result CRUD operations
│   ├── users.ts         # User management (v0.6+)
│   ├── audit.ts         # Audit log service (v0.6+)
│   └── stats.ts         # Statistics service (v0.6+)
├── context/             # React contexts
│   └── AuthContext.tsx  # Authentication state management
├── types/               # TypeScript type definitions
│   ├── sample.ts        # Sample types
│   ├── order.ts         # Order types
│   ├── result.ts        # TestResult types
│   ├── user.ts          # User types (v0.6+)
│   ├── audit.ts         # Audit log types (v0.6+)
│   └── stats.ts         # Statistics types (v0.6+)
└── utils/               # Utility functions and constants
    └── constants.ts     # API_BASE_URL, default values
```

## Getting Started

### Prerequisites

- **Node.js**: 18+ and npm 9+
- **OpenSylab Backend**: Running on `http://localhost:8080`

### Installation

```bash
cd frontend
npm install
```

### Development

```bash
# Start development server with HMR
npm run dev
```

The application will be available at **http://localhost:5173**

Default credentials:
- **Username**: admin
- **Password**: admin
- ⚠️ **MUST be changed in production!**

### Building for Production

```bash
# Create optimized production build
npm run build
```

The production build will be in the `dist/` directory.

### Preview Production Build

```bash
npm run preview
```

### Linting

```bash
# Run ESLint
npm run lint

# Fix linting issues
npm run lint -- --fix
```

## Environment Configuration

Create environment files based on `.env.example`:

```bash
cp .env.example .env.development
cp .env.example .env.production
```

### Available Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `VITE_API_URL` | `http://localhost:8080/api/v1` | Backend API base URL |

### Environment Files

- **`.env.development`**: Development environment (HTTP, localhost)
- **`.env.production`**: Production environment (HTTPS, production domain)

**Example `.env.production`:**

```bash
VITE_API_URL=https://api.opensylab.yourdomain.com/api/v1
```

## Features

### 🔐 Authentication (v0.5+)

- **JWT-based authentication** with HS256 tokens
- **Protected routes** with automatic redirection to login
- **Token expiration** handling (60 minutes)
- **Persistent login** using localStorage
- **Automatic token refresh**
- **Logout** functionality

### 📊 Dashboard (v0.6+)

- **Multi-entity statistics**:
  - Sample counts by status (Captured, In Analysis, Analyzed, Validated, Archived)
  - Order counts by status (Requested, In Progress, Completed, Validated, Cancelled)
  - Result counts by status (Pending, Entered, Validated, Rejected, Repeated)
- **Real-time data** from backend aggregation
- **Visual status indicators** with color-coded badges

### 🧪 Sample Management

- **Paginated sample list** with filtering
- **Filter by status**
- **Search capabilities**
- **Responsive table design**
- **Detail view** for each sample
- **Create/Edit forms** (planned for v0.7.0)

### 📋 Order Management

- **Order list** with dual-filtering
  - **Status filter**: Requested, In Progress, Completed, Validated, Cancelled
  - **Priority filter**: Normal, Urgent, Emergency
- **Order details**
- **Link orders to samples**
- **Status workflow transitions** (planned for v0.7.0)

### 🔬 Result Management

- **Result list** with dual-filtering
  - **Status filter**: Pending, Entered, Validated, Rejected, Repeated
  - **Flag filter**: Normal, Low, High, Critical, Undefined
- **Color-coded flag badges**
- **Result details** with reference ranges
- **Result entry form** (planned for v0.7.0)

### 👥 User Management (v0.6+ - Admin Only)

- **CRUD operations** for users
- **Role-Based Access Control (RBAC)**:
  - ADMIN - Full system access
  - OPERATOR - Standard user operations
  - VIEWER - Read-only access
  - CUSTOM - Custom permissions
- **User activation/deactivation**
- **User profile data** (name, email, last login)
- **Modal-based create/edit forms**

### 📜 Audit Log (v0.6+ - Admin Only)

- **Complete audit trail** for compliance
- **Filtering**:
  - By user
  - By action (CREATE, UPDATE, DELETE, LOGIN, LOGOUT)
  - By entity (sample, order, result, user)
  - By date range
- **ISO 15189 compliance tracking**
- **Immutable audit entries**

### 👤 User Profile (v0.6+)

- **View user information**
- **Change password** with validation
- **Last login tracking**
- **Password strength requirements** (8+ characters)

## API Integration

The frontend communicates with the OpenSylab backend API via Axios.

### Base Configuration

- **Base URL**: `http://localhost:8080/api/v1` (development)
- **Authentication**: Bearer JWT token in Authorization header
- **Content-Type**: application/json

### Core API Endpoints

#### Authentication
- `POST /auth/login` - Login with username/password
- `POST /auth/logout` - Logout (invalidate token)

#### Samples
- `GET /samples` - List samples (pagination, filtering)
- `GET /samples/:id` - Get sample details
- `POST /samples` - Create new sample
- `PUT /samples/:id` - Update sample
- `DELETE /samples/:id` - Delete sample (planned for v0.7.0)

#### Orders
- `GET /orders` - List orders (pagination, filtering)
- `GET /orders/:id` - Get order details
- `POST /orders` - Create new order
- `PUT /orders/:id` - Update order
- `DELETE /orders/:id` - Delete order (planned for v0.7.0)

#### Results
- `GET /results` - List results (pagination, filtering)
- `GET /results/:id` - Get result details
- `POST /results` - Create new result
- `PUT /results/:id` - Update result
- `DELETE /results/:id` - Delete result (planned for v0.7.0)

#### User Management (v0.6+ - Admin Only)
- `GET /users` - List all users
- `POST /users` - Create new user
- `PUT /users/:id` - Update user
- `DELETE /users/:id` - Delete user
- `GET /users/me` - Get current user profile
- `PUT /users/me/password` - Change password

#### Audit & Statistics (v0.6+)
- `GET /audit` - Get audit log (Admin only, with filtering)
- `GET /stats` - Get dashboard statistics

## CORS Configuration

The backend must allow CORS requests from the frontend origin:

```
Access-Control-Allow-Origin: http://localhost:5173
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization
Access-Control-Allow-Credentials: true
```

## TypeScript

This project uses **strict TypeScript configuration**:

- No `any` types allowed
- Strict null checks
- Strict function types
- Full type coverage for all components

## Code Style

- **Functional components** with React hooks
- **TypeScript** for type safety
- **Tailwind CSS** for styling (utility-first)
- **Accessible UI** with ARIA labels
- **Consistent naming**:
  - Components: PascalCase (`Dashboard.tsx`)
  - Services: camelCase (`auth.ts`)
  - Types: PascalCase (`User`, `Sample`)

## Component Patterns

### Modal-Based Forms

Create/Edit forms use modal overlays:

```typescript
const [showModal, setShowModal] = useState(false);
const [selectedUser, setSelectedUser] = useState<User | null>(null);

// Open modal for edit
const handleEdit = (user: User) => {
  setSelectedUser(user);
  setShowModal(true);
};

// Close modal
const handleClose = () => {
  setShowModal(false);
  setSelectedUser(null);
};
```

### Protected Routes

Routes require authentication and optionally specific roles:

```typescript
<Route
  path="/users"
  element={
    <ProtectedRoute requiredRole="ADMIN">
      <Users />
    </ProtectedRoute>
  }
/>
```

### Service Layer Pattern

All API calls go through service functions:

```typescript
// services/users.ts
export const getUsers = async (): Promise<User[]> => {
  const response = await axios.get<User[]>('/users');
  return response.data;
};

// Component usage
const users = await getUsers();
```

## Browser Support

- **Chrome** (latest)
- **Firefox** (latest)
- **Safari** (latest)
- **Edge** (latest)
- **Minimum**: ES2020 support required

## Testing (v0.7.0+)

```bash
# Unit Tests (Jest + React Testing Library)
npm test

# Watch Mode
npm test -- --watch

# Coverage Report
npm run test:coverage

# E2E Tests (Cypress/Playwright)
npm run test:e2e
```

## Implemented Features (v0.6.0)

✅ JWT-based authentication
✅ Protected routes with RBAC
✅ Dashboard with multi-entity statistics
✅ Sample list with filtering
✅ Order list with dual-filtering
✅ Result list with dual-filtering
✅ User management (Admin only)
✅ Audit log viewer (Admin only)
✅ User profile with password change
✅ Role-based navigation
✅ Modal-based CRUD forms
✅ Color-coded status badges
✅ Responsive design
✅ TypeScript type safety

## Planned Features

### v0.7.0 (Next Release)
- [ ] Create/Edit forms for Samples, Orders, Results
- [ ] Delete confirmation dialogs
- [ ] Barcode scanning integration
- [ ] Status workflow transitions
- [ ] Frontend unit tests (Jest + RTL)
- [ ] E2E tests (Cypress)

### v0.8.0
- [ ] Data visualization (Charts.js/Recharts)
- [ ] CSV import UI
- [ ] Advanced filtering
- [ ] Export functionality (PDF, Excel, CSV)
- [ ] Real-time updates (WebSocket)
- [ ] Notification system

### v1.0+
- [ ] Multi-language support (i18n)
- [ ] Mobile app (React Native)
- [ ] Offline mode
- [ ] Advanced search
- [ ] Custom dashboards
- [ ] Report builder

## Documentation

- **[UI_EXTENSIONS_V06.md](UI_EXTENSIONS_V06.md)** - v0.6.0 UI features documentation
- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Development guide
- **[INTEGRATION.md](INTEGRATION.md)** - Backend integration guide
- **[QUICK_START.md](QUICK_START.md)** - Quick start guide

## Troubleshooting

### "Failed to fetch" errors

- Ensure backend is running on `http://localhost:8080`
- Check CORS configuration on backend
- Verify `VITE_API_URL` in `.env` file

### Authentication issues

- Clear localStorage: `localStorage.clear()`
- Check JWT token expiration
- Verify backend JWT secret matches
- Try default credentials: admin / admin

### Build errors

```bash
# Clean install
rm -rf node_modules package-lock.json
npm install

# Clear Vite cache
rm -rf dist .vite
npm run build
```

## Contributing

See [../CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines.

## License

See [../LICENSE.txt](../LICENSE.txt) for license information.

---

**Frontend Version:** 0.6.0
**Backend Compatibility:** OpenSylab v0.6.0+
**Last Updated:** 2026-02-03
