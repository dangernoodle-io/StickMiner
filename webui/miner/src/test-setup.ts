import '@testing-library/jest-dom/vitest'

// Node 25+ enables a stub Web Storage API that masks jsdom's window.localStorage
// (see vitest-dev/vitest#8757). The Node-builtin stub throws on access without
// --localstorage-file (which is not allowed in NODE_OPTIONS, only on the CLI).
// Provide an in-memory shim so component code that uses localStorage
// (e.g. ConfirmDialog skipKey) works regardless of Node version.
function makeStorageShim(): Storage {
  const store = new Map<string, string>()
  return {
    get length() { return store.size },
    clear: () => store.clear(),
    getItem: (k: string) => store.get(k) ?? null,
    setItem: (k: string, v: string) => { store.set(k, String(v)) },
    removeItem: (k: string) => { store.delete(k) },
    key: (i: number) => Array.from(store.keys())[i] ?? null,
  }
}

let lsWorks = false
try { lsWorks = typeof globalThis.localStorage?.getItem === 'function' && (globalThis.localStorage.getItem('__probe__'), true) } catch { lsWorks = false }
if (!lsWorks) {
  Object.defineProperty(globalThis, 'localStorage', { value: makeStorageShim(), configurable: true, writable: true })
}

let ssWorks = false
try { ssWorks = typeof globalThis.sessionStorage?.getItem === 'function' && (globalThis.sessionStorage.getItem('__probe__'), true) } catch { ssWorks = false }
if (!ssWorks) {
  Object.defineProperty(globalThis, 'sessionStorage', { value: makeStorageShim(), configurable: true, writable: true })
}
