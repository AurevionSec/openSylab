import '@testing-library/jest-dom';
import { Window } from 'happy-dom';

/**
 * Node 26 ships an experimental (non-functional) localStorage stub in globalThis.
 * vitest's populateGlobal() skips keys that already exist in global unless they
 * appear in its hard-coded KEYS list — so happy-dom's working implementation is
 * never injected.  We create a dedicated happy-dom Window here and manually
 * install its Storage instances, replacing the Node 26 stubs.
 */
const _storageWindow = new Window({ url: 'http://localhost:3000' });

Object.defineProperty(globalThis, 'localStorage', {
  value: _storageWindow.localStorage,
  writable: true,
  configurable: true,
  enumerable: true,
});

Object.defineProperty(globalThis, 'sessionStorage', {
  value: _storageWindow.sessionStorage,
  writable: true,
  configurable: true,
  enumerable: true,
});
