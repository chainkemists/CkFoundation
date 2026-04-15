# CkDynamic

**Purpose:** Dynamic behavior system — runtime-composable behaviors attached to entities via data assets. Provides a type-erased struct dispatch mechanism (`FScriptStructWildcard`) so behavior definitions can be content-authored.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** `CkStateMachine` (state behaviors), `CkAngelscriptGenerator`, `CkK2Nodes`, `CkDynamicEditor`.

---

## Key API

- `UCk_Utils_Dynamic_UE` — add, remove, query dynamic behaviors on entities.
- Behaviors are defined as `UDataAsset`-derived types and referenced by `FScriptStructWildcard`.

---

## Pattern

Designers create behavior data assets; the dynamic system applies them to entities at runtime. State machines use this to swap behaviors on state transitions.

---

## Anti-patterns

1. Don't hardcode behavior selection in C++ where a data asset would suffice.
2. Don't mix Dynamic behavior data with Fragment data — they're different abstraction layers.

---

## See also

- `CkStateMachine/Claude.md` — primary consumer.
- `CkEcsExt/Claude.md` — Meta Fragment infrastructure.
