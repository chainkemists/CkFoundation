# CkActorProxy

**Purpose:** Actor proxy pattern — a lightweight ECS entity that 'proxies' an `AActor` into the ECS world without making the actor itself ECS-aware. Used for actors created by external systems that need to participate in CkFoundation features.

**Depends on:** `CkCore`, `CkEcs`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Integration with non-ECS actor systems (third-party plugins, legacy actors).

---

## Key API

- No `_Utils.h`. Proxy is set up via fragment-direct patterns — see `CkActorProxy`'s fragment files.

---

## Pattern

Create a proxy entity, associate the external actor via OwningActor fragment, then use the proxy entity handle as the ECS entry point for CkFoundation features.

---

## Anti-patterns

Don't create proxy entities for actors that can be refactored to use the ECS directly.

---

## See also

- `CkActor/Claude.md`, `CkEcsExt/Claude.md` — EntityHolder for lifetime binding.
