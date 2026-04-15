# CkCore / Log

CkCore-side log category wrappers. The actual log implementation lives in the `CkLog` module; this file is the pass-through include that lets CkCore headers emit diagnostics without a circular dep.

**Key files:** `CkLog.h`

For the full use-case table and how this folder fits with the rest of CkCore, see [`../../Claude.md`](../../Claude.md).

## See also
- `CkLog/` module — the real log infrastructure.
