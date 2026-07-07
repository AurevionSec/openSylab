# Versionsverwaltung — Single Source of Truth

OpenSylab nutzt zwei kanonische Quellen für die Versionsnummer, die beide bei einem
Release-Bump synchron geändert werden. Alle anderen Dateien referenzieren diese
Quellen automatisch — keine manuellen Anpassungen nötig.

---

## Architektur

```
CMakeLists.txt                     frontend/package.json
  project(OpenSylab VERSION 1.0.0)   "version": "1.0.0"
         │                                    │
         ▼ cmake configure_file              ▼ vite define (vite.config.ts)
   include/version.h               import.meta.env.VITE_APP_VERSION
         │                                    │
         ├── src/main.cpp                     └── src/components/Layout/Sidebar.tsx
         ├── src/utils/CliInterface.cpp
         └── test/unit/test_runner.cpp
```

### C++ — `CMakeLists.txt`

```cmake
project(OpenSylab
    VERSION 1.0.0          # ← HIER ändern für neues Release
    DESCRIPTION "..."
    LANGUAGES CXX
)

configure_file(
    "${CMAKE_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_SOURCE_DIR}/include/version.h"
    @ONLY
)
```

CMake generiert beim Konfigurieren `include/version.h` aus dem Template
`include/version.h.in`. Das Header definiert:

| Makro | Beispielwert |
|-------|-------------|
| `OPENSYLAB_VERSION` | `"1.0.0"` |
| `OPENSYLAB_VERSION_STRING` | `"OpenSylab v1.0.0"` |
| `OPENSYLAB_VERSION_MAJOR` | `0` |
| `OPENSYLAB_VERSION_MINOR` | `7` |
| `OPENSYLAB_VERSION_PATCH` | `0` |

Verwendung in C++:

```cpp
#include "version.h"

std::cout << OPENSYLAB_VERSION_STRING << "\n";  // "OpenSylab v1.0.0"
```

### Frontend — `package.json`

```json
{
  "name": "frontend",
  "version": "1.0.0"    ← HIER ändern für neues Release
}
```

`vite.config.ts` liest die Version zur Build-Zeit und injiziert sie:

```typescript
import pkg from './package.json'

export default defineConfig({
  define: {
    'import.meta.env.VITE_APP_VERSION': JSON.stringify(pkg.version),
  },
})
```

Verwendung im Frontend:

```tsx
<span>{import.meta.env.VITE_APP_VERSION}</span>  {/* "1.0.0" */}
```

> **Hinweis:** `VITE_APP_VERSION` ist ein Build-Zeit-Wert — er ist hartcodiert im
> Bundle und ändert sich nicht zur Laufzeit.

---

## Version bumpen (Release-Prozess)

```bash
# 1. Beide SSoTs aktualisieren
#    CMakeLists.txt: VERSION 1.0.0 → 1.1.0
#    frontend/package.json: "version": "0.8.0"

# 2. C++ neu bauen (generiert version.h)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# 3. Frontend neu bauen
cd frontend && npm run build

# 4. Verifizieren
./build/bin/OpenSylab --api --api-port 9080 --db /tmp/test.db &
grep "0.8.0" frontend/dist/assets/*.js
```

---

## Dateien, die NICHT manuell angepasst werden müssen

| Datei | Grund |
|-------|-------|
| `include/version.h` | Wird von CMake generiert — **nicht** committen ohne Rebuild |
| `src/main.cpp` | Nutzt `OPENSYLAB_VERSION_STRING` |
| `src/utils/CliInterface.cpp` | Nutzt `OPENSYLAB_VERSION` |
| `test/unit/test_runner.cpp` | Nutzt `OPENSYLAB_VERSION_STRING` |
| `frontend/src/components/Layout/Sidebar.tsx` | Nutzt `import.meta.env.VITE_APP_VERSION` |

---

## Semantic Versioning

OpenSylab folgt [Semantic Versioning 2.0.0](https://semver.org):

| Änderung | Version |
|----------|---------|
| Abwärtsinkompatible API-Änderungen, DB-Schema-Migration nötig | `MAJOR` |
| Neue Features, abwärtskompatibel | `MINOR` |
| Bugfixes, Sicherheitspatches | `PATCH` |

Aktuelle Version: **1.0.0** — stabil; die API folgt ab 1.0 SemVer (Breaking Changes nur in Major-Releases).
