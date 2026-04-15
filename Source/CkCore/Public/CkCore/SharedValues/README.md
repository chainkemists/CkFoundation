# CkCore / SharedValues

`FCk_SharedBool` / `FCk_SharedInt` / etc. — `USTRUCT`-wrapped primitives so they can be BP-visible and passed by reference across boundaries. Use when a plain `bool` won't cross a UFUNCTION boundary correctly.

**Key files:** `CkSharedValues.h`, `CkSharedValues_Utils.h`

For the full use-case table and how this folder fits with the rest of CkCore, see [`../../Claude.md`](../../Claude.md).
