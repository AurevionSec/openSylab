# OpenSylab LIMS — Frontend

The React 19 + TypeScript frontend for OpenSylab. For the full project overview,
architecture, and API, see the [root README](../README.md).

## Tech stack

- **Framework:** React 19 + TypeScript (strict)
- **Build:** Vite
- **Routing:** React Router
- **HTTP:** Axios with a JWT interceptor + token-expiry guard
- **Styling:** Tailwind CSS (v4)
- **Charts:** Recharts
- **Tests:** Vitest + React Testing Library (unit) · Playwright (end-to-end)

## Development

```bash
npm install
# point the app at a running backend:
echo "VITE_API_URL=http://localhost:8080/api/v1" > .env.development
npm run dev            # dev server on http://localhost:5173
```

## Checks

```bash
npx tsc --noEmit       # type-check (strict, 0 errors expected)
npm run lint           # ESLint
npm test               # Vitest unit tests
npm run test:e2e       # Playwright end-to-end (boots backend + frontend itself)
npm run build          # production build
```

## Structure

```
src/
  components/   shared UI (common/) + per-entity modals (Samples/, Orders/, Results/)
  pages/        route views (Dashboard, Samples, Orders, Results, Audit, Users, …)
  services/     API wrappers (central Axios instance with auth-header injection)
  hooks/        useEntityList, useToast, useModalA11y, useListParams, …
  context/      Auth + Theme providers
  types/        shared TypeScript types
```

Design language: see [DESIGN.md](../DESIGN.md).
