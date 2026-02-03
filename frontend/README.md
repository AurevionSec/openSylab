# OpenSylab LIMS - Frontend

Modern React TypeScript frontend for the OpenSylab Laboratory Information Management System.

## Tech Stack

- **Framework**: React 18 with TypeScript
- **Build Tool**: Vite
- **Routing**: React Router v6
- **HTTP Client**: Axios
- **Styling**: Tailwind CSS
- **Package Manager**: npm

## Project Structure

```
src/
├── components/
│   ├── Layout/          # Layout components (Header, Sidebar, Layout)
│   ├── Auth/            # Authentication components
│   └── common/          # Reusable UI components (Button, Input, Card)
├── pages/               # Page components (Dashboard, Samples, Login)
├── services/            # API services (auth, samples)
├── context/             # React contexts (AuthContext)
├── types/               # TypeScript type definitions
└── utils/               # Utility functions and constants
```

## Getting Started

### Prerequisites

- Node.js 18+ and npm
- OpenSylab backend running on `http://localhost:8080`

### Installation

```bash
cd frontend
npm install
```

### Development

```bash
npm run dev
```

The application will be available at `http://localhost:5173`

### Building for Production

```bash
npm run build
```

The production build will be in the `dist/` directory.

### Preview Production Build

```bash
npm run preview
```

## Environment Configuration

Create a `.env` file based on `.env.example`:

```bash
cp .env.example .env
```

Available environment variables:

- `VITE_API_URL`: Backend API URL (default: `http://localhost:8080/api/v1`)

### Environment Files

- `.env.development`: Development environment (HTTP)
- `.env.production`: Production environment (HTTPS)

## Features

### Authentication

- API key-based authentication
- Protected routes with automatic redirection
- Persistent login state using localStorage

### Dashboard

- Overview of sample statistics
- Sample counts by status
- Recent samples list
- Visual status indicators

### Sample Management

- Paginated sample list
- Filter by status
- Search capabilities
- Responsive table design

## API Integration

The frontend communicates with the OpenSylab backend API:

- **Base URL**: `http://localhost:8080/api/v1` (development)
- **Authentication**: X-API-Key header
- **Endpoints**:
  - `GET /samples` - List samples with pagination and filtering
  - `GET /samples/:id` - Get sample details
  - `POST /samples` - Create new sample
  - `PUT /samples/:id` - Update sample
  - `DELETE /samples/:id` - Delete sample

## CORS Configuration

The backend must allow CORS requests from the frontend origin:

```
Access-Control-Allow-Origin: http://localhost:5173
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, X-API-Key
```

## TypeScript

This project uses strict TypeScript configuration. No `any` types are allowed.

## Code Style

- Functional components with hooks
- TypeScript for type safety
- Tailwind CSS for styling
- Accessible UI with ARIA labels

## Browser Support

- Chrome (latest)
- Firefox (latest)
- Safari (latest)
- Edge (latest)

## Future Enhancements

- HTTPS support (port 8443)
- Sample detail view
- Create/Edit sample forms
- Advanced filtering and search
- Export functionality
- Real-time updates
- User management
- Audit logs
