# Development Guide

## Quick Start

```bash
cd frontend
npm install
npm run dev
```

The application will be available at http://localhost:5173

## Available Scripts

### Development
```bash
npm run dev          # Start development server with hot reload
npm run build        # Build for production
npm run preview      # Preview production build locally
npm run lint         # Run ESLint
```

## Project Architecture

### Component Structure

```
components/
├── Layout/          # Application shell components
│   ├── Header.tsx   # Top navigation bar with logout
│   ├── Sidebar.tsx  # Left sidebar navigation
│   └── Layout.tsx   # Main layout wrapper
├── Auth/            # Authentication components
│   └── ProtectedRoute.tsx  # Route guard for authenticated pages
└── common/          # Reusable UI components
    ├── Button.tsx   # Styled button component
    ├── Input.tsx    # Form input with validation
    └── Card.tsx     # Content card wrapper
```

### Pages

```
pages/
├── Login.tsx        # Login page with API key form
├── Dashboard.tsx    # Main dashboard with statistics
└── Samples.tsx      # Sample list with filtering and pagination
```

### Services (API Layer)

```
services/
├── api.ts           # Axios instance with interceptors
├── auth.ts          # Authentication service
└── samples.ts       # Sample CRUD operations
```

### Context (State Management)

```
context/
└── AuthContext.tsx  # Global authentication state
```

## TypeScript Guidelines

### Type-Only Imports
Always use type-only imports for types to comply with `verbatimModuleSyntax`:

```typescript
// Good
import type { ReactNode } from 'react';
import { useState } from 'react';

// Bad
import { ReactNode, useState } from 'react';
```

### No Any Types
Avoid using `any` type. Use proper type definitions or `unknown` if type is truly unknown.

```typescript
// Good
const handleError = (error: unknown) => {
  if (error instanceof Error) {
    console.error(error.message);
  }
};

// Bad
const handleError = (error: any) => {
  console.error(error.message);
};
```

### Type Definitions
Keep type definitions in the `types/` directory:

```typescript
// types/sample.ts
export interface Sample {
  id: number;
  sample_id: string;
  // ...
}
```

## Styling with Tailwind CSS

### Utility-First Approach
Use Tailwind utility classes instead of custom CSS:

```tsx
// Good
<div className="bg-white rounded-lg shadow-md p-6">
  <h2 className="text-2xl font-bold text-gray-900">Title</h2>
</div>

// Avoid custom CSS unless necessary
```

### Responsive Design
Use Tailwind's responsive prefixes:

```tsx
<div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
  {/* Content */}
</div>
```

### Color Palette
Stick to Tailwind's default colors for consistency:
- Primary: blue (600, 700)
- Success: green (100, 800)
- Warning: yellow (100, 800)
- Danger: red (600, 700)
- Neutral: gray (50-900)

## API Integration

### Making API Calls

```typescript
import { getSamples } from '../services/samples';

const fetchData = async () => {
  try {
    const data = await getSamples({ limit: 20, offset: 0 });
    setSamples(data.samples);
  } catch (error) {
    console.error('Failed to fetch samples:', error);
  }
};
```

### Error Handling
The API client automatically handles:
- 401/403 errors (redirects to login)
- API key injection via interceptor

### Adding New API Endpoints

1. Define types in `types/`:
```typescript
// types/order.ts
export interface Order {
  id: number;
  order_number: string;
  // ...
}
```

2. Create service in `services/`:
```typescript
// services/orders.ts
import api from './api';
import type { Order } from '../types/order';

export const getOrders = async (): Promise<Order[]> => {
  const response = await api.get<Order[]>('/orders');
  return response.data;
};
```

3. Use in components:
```typescript
import { getOrders } from '../services/orders';

const Orders = () => {
  const [orders, setOrders] = useState<Order[]>([]);

  useEffect(() => {
    getOrders().then(setOrders);
  }, []);

  // ...
};
```

## Authentication Flow

### Login Process
1. User enters API key in login form
2. `AuthContext.login()` validates key via backend
3. Valid key stored in localStorage
4. User redirected to dashboard

### Protected Routes
```tsx
<Route
  path="/samples"
  element={
    <ProtectedRoute>
      <Samples />
    </ProtectedRoute>
  }
/>
```

### Logout
```typescript
const { logout } = useAuth();

const handleLogout = () => {
  logout(); // Clears localStorage
  navigate('/login');
};
```

## Adding New Features

### Example: Adding a New Page

1. Create page component:
```typescript
// pages/Reports.tsx
import { Layout } from '../components/Layout/Layout';

export const Reports = () => {
  return (
    <Layout>
      <h2>Reports</h2>
      {/* Content */}
    </Layout>
  );
};
```

2. Add route in `App.tsx`:
```typescript
<Route
  path="/reports"
  element={
    <ProtectedRoute>
      <Reports />
    </ProtectedRoute>
  }
/>
```

3. Add navigation link in `Sidebar.tsx`:
```typescript
const navItems: NavItem[] = [
  { name: 'Dashboard', path: '/', icon: '📊' },
  { name: 'Samples', path: '/samples', icon: '🧪' },
  { name: 'Reports', path: '/reports', icon: '📄' }, // New
];
```

## Common Patterns

### Loading States
```typescript
const [loading, setLoading] = useState(true);

if (loading) {
  return (
    <div className="flex items-center justify-center h-64">
      <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600"></div>
    </div>
  );
}
```

### Error States
```typescript
const [error, setError] = useState('');

if (error) {
  return (
    <div className="bg-red-50 border border-red-200 rounded-lg p-4">
      <p className="text-red-800">{error}</p>
    </div>
  );
}
```

### Empty States
```typescript
if (items.length === 0) {
  return (
    <div className="text-center py-12">
      <p className="text-gray-500">No items found</p>
    </div>
  );
}
```

## Accessibility

### ARIA Labels
```tsx
<button aria-label="Close dialog" onClick={handleClose}>
  <XIcon />
</button>
```

### Semantic HTML
Use semantic HTML elements:
```tsx
<nav>, <header>, <main>, <aside>, <article>, <section>
```

### Keyboard Navigation
Ensure all interactive elements are keyboard accessible:
```tsx
<div
  role="button"
  tabIndex={0}
  onKeyPress={(e) => e.key === 'Enter' && handleClick()}
  onClick={handleClick}
>
  Click me
</div>
```

## Performance Optimization

### Lazy Loading Routes
```typescript
import { lazy, Suspense } from 'react';

const Samples = lazy(() => import('./pages/Samples'));

<Suspense fallback={<Loading />}>
  <Samples />
</Suspense>
```

### Memoization
```typescript
import { useMemo } from 'react';

const expensiveValue = useMemo(() => {
  return computeExpensiveValue(data);
}, [data]);
```

## Testing (Future)

Framework ready for:
- Vitest for unit tests
- React Testing Library for component tests
- Playwright for E2E tests

## Debugging

### React DevTools
Install React Developer Tools browser extension for debugging.

### API Debugging
Check Network tab in browser DevTools to inspect API requests/responses.

### Console Logging
```typescript
console.log('Debug:', { samples, loading, error });
```

## Common Issues

### CORS Errors
Ensure backend has CORS headers configured. See INTEGRATION.md.

### API Key Not Working
1. Check if backend is running
2. Verify API key is correct
3. Check browser console for errors
4. Verify X-API-Key header is sent

### Build Errors
Run `npm run build` to check for TypeScript errors before committing.
