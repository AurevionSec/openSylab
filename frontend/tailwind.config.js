/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      fontFamily: {
        'sans': ['Inter', 'system-ui', 'sans-serif'],
        'mono': ['JetBrains Mono', 'Roboto Mono', 'Courier New', 'monospace'],
      },
      colors: {
        'clinical': {
          bg: '#F4F5F7',
          surface: '#FFFFFF',
          text: '#1A1C20',
          secondary: '#5E6C84',
          border: '#E2E8F0',
          hover: '#FAFBFC',
          blue: '#0055FF',
          green: '#10B981',
          biohazard: '#CCFF00',
          critical: '#FF3B30',
        },
        'dark': {
          bg: '#0D0E12',
          surface: '#16181D',
          text: '#E0E0E0',
        },
      },
    },
  },
  plugins: [],
}
