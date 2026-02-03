# Quick Start Guide

## First Time Setup

```bash
cd /home/eddy/projekte/openSylab/frontend
npm install
```

## Start Development Server

```bash
npm run dev
```

Visit http://localhost:5173

## Build for Production

```bash
npm run build
```

Output in `dist/` directory

## Preview Production Build

```bash
npm run preview
```

## Project Commands

| Command | Description |
|---------|-------------|
| `npm run dev` | Start dev server with hot reload |
| `npm run build` | Build for production |
| `npm run preview` | Preview production build |
| `npm run lint` | Run ESLint |

## Environment Variables

### Development
Edit `.env.development`:
```
VITE_API_URL=http://localhost:8080/api/v1
```

### Production
Edit `.env.production`:
```
VITE_API_URL=https://localhost:8443/api/v1
```

## Login

Default authentication uses API key:
1. Start backend on http://localhost:8080
2. Start frontend: `npm run dev`
3. Visit http://localhost:5173
4. Enter your API key
5. Click "Sign In"

## Testing Backend Integration

### Check if backend is running:
```bash
curl http://localhost:8080/api/v1/samples?limit=1
```

### Test with API key:
```bash
curl -H "X-API-Key: your-key-here" http://localhost:8080/api/v1/samples?limit=1
```

### Test CORS:
```bash
curl -H "Origin: http://localhost:5173" \
     -H "Access-Control-Request-Method: GET" \
     -H "Access-Control-Request-Headers: X-API-Key" \
     -X OPTIONS \
     http://localhost:8080/api/v1/samples
```

## File Structure

```
src/
├── components/      # Reusable UI components
├── pages/          # Page components (Login, Dashboard, Samples)
├── services/       # API calls (auth, samples)
├── context/        # React contexts (AuthContext)
├── types/          # TypeScript types
└── utils/          # Constants and utilities
```

## Common Tasks

### Add a new page
1. Create component in `src/pages/MyPage.tsx`
2. Add route in `src/App.tsx`
3. Add navigation link in `src/components/Layout/Sidebar.tsx`

### Add API endpoint
1. Define types in `src/types/mytype.ts`
2. Create service in `src/services/myservice.ts`
3. Use in component with `useEffect` or event handler

### Update styling
Use Tailwind classes in `className`:
```tsx
<div className="bg-white rounded-lg shadow-md p-6">
  Content
</div>
```

## Troubleshooting

### Port 5173 already in use
```bash
# Kill existing process
lsof -ti:5173 | xargs kill -9
# Or use different port
npm run dev -- --port 3000
```

### CORS errors
Check backend CORS configuration (see INTEGRATION.md)

### API key not working
1. Verify backend is running
2. Check browser console for errors
3. Verify API key in localStorage: `localStorage.getItem('opensylab_api_key')`

### Build fails
```bash
# Clear cache and rebuild
rm -rf node_modules dist
npm install
npm run build
```

## Documentation

- `README.md` - Project overview and features
- `DEVELOPMENT.md` - Developer guide and best practices
- `INTEGRATION.md` - Backend integration specifications
- `IMPLEMENTATION_REPORT.md` - Complete implementation details

## Getting Help

1. Check browser console for errors
2. Check Network tab for failed API requests
3. Review documentation files
4. Check TypeScript errors: `npm run build`

## Next Steps

1. Ensure backend implements CORS (see INTEGRATION.md)
2. Ensure backend implements API endpoints (see INTEGRATION.md)
3. Test login flow with valid API key
4. Verify dashboard loads sample data
5. Test sample filtering and pagination

## Production Deployment

### Build
```bash
npm run build
```

### Deploy
Deploy `dist/` directory to:
- Static hosting (Netlify, Vercel, GitHub Pages)
- Web server (Nginx, Apache)
- CDN

### Environment
Set production API URL in deployment environment:
```
VITE_API_URL=https://your-production-api.com/api/v1
```

## Support

For issues or questions, refer to:
- DEVELOPMENT.md for coding guidelines
- INTEGRATION.md for API specifications
- IMPLEMENTATION_REPORT.md for architecture details
