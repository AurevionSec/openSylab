import js from '@eslint/js'
import globals from 'globals'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'
import tseslint from 'typescript-eslint'
import { defineConfig, globalIgnores } from 'eslint/config'

export default defineConfig([
  // Build/output artefacts must not be linted.
  globalIgnores(['dist', 'coverage', '.vite', 'node_modules']),
  {
    files: ['**/*.{ts,tsx}'],
    extends: [
      js.configs.recommended,
      tseslint.configs.recommended,
      reactHooks.configs.flat.recommended,
      reactRefresh.configs.vite,
    ],
    languageOptions: {
      ecmaVersion: 2020,
      globals: globals.browser,
    },
    rules: {
      // `_`-prefixed names are an intentional "unused" convention; ignore them.
      // Unused catch bindings are common for coarse error handling — don't flag.
      '@typescript-eslint/no-unused-vars': [
        'error',
        {
          argsIgnorePattern: '^_',
          varsIgnorePattern: '^_',
          caughtErrors: 'none',
        },
      ],
      // Classic correctness rules (rules-of-hooks, exhaustive-deps) stay as
      // errors. The new experimental react-compiler advisory rules introduced in
      // eslint-plugin-react-hooks 7.x are downgraded to warnings for incremental
      // adoption on this existing, tested codebase — rewriting working effects to
      // satisfy them is tracked separately, not a blocker.
      'react-hooks/set-state-in-effect': 'warn',
      'react-hooks/immutability': 'warn',
      'react-hooks/refs': 'warn',
      // Context files intentionally co-locate provider + hook; HMR-only rule.
      'react-refresh/only-export-components': 'warn',
    },
  },
  // Ambient global type declarations legitimately use `var`.
  {
    files: ['**/*.d.ts'],
    rules: { 'no-var': 'off' },
  },
])
