# CkVfx

**Purpose:** VFX entity system — Niagara system entities (`FCk_Handle_VfxCue`) on ECS entities. Each VFX entity spawns a `UNiagaraComponent`, drives its parameters from fragment data, and fires `OnFinished` when the effect completes.

**Depends on:** `CkActorRelay`, `CkCore`, `CkCue`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTimer`.
**Used by:** Hit effects, spell VFX, environment effects.

---

## Key API

- `UCk_Utils_VfxCue_DataChannel_UE` — add VFX data channels.
- `FProcessor_VfxCue_Setup` — creates `UNiagaraComponent` (AutoDestroy=false, AutoActivate=false per CLAUDE.md).
- `FProcessor_VfxCue_EndPlay` — destroys component.
- Signal: `OnFinished` when Niagara system deactivates.

---

## Pattern

Per root CLAUDE.md section "Component Lifetime Management in ECS":
- **Setup processor** creates the `UNiagaraComponent` via `UNiagaraFunctionLibrary::SpawnSystemAtLocation` with `AutoDestroy=false, AutoActivate=false`.
- **LifetimeMonitor processor** detects completion and fires `OnFinished`.
- **EndPlay processor** calls `DestroyComponent()` during entity destruction.

Signal fires → EntityScript receives `OnFinished` → EntityScript may destroy VFX entity → EndPlay processor cleans up component.

---

## Anti-patterns

1. Never call `DestroyComponent()` in the LifetimeMonitor — only in EndPlay.
2. Don't use `AutoDestroy=true` on the Niagara component — the ECS owns the lifetime. Niagara auto-destroying
   the component leaves `FProcessor_VfxCue_EffectLifetimeMonitor`'s `IsActive()` check and
   `FProcessor_VfxCue_EndPlay`'s `DestroyComponent()` operating on a dangling pointer.
3. Don't use `AutoActivate=true` either, not even for AutoPlay mode. Activation is driven by
   `FCk_Request_VfxCue_Play` through the request queue; bypassing it skips the `OnStarted` signal and the
   effect start-time bookkeeping in `FProcessor_VfxCue_HandleRequests`.

---

## See also

- `CkCue/Claude.md`, `CkActorRelay/Claude.md`.
- Root `CLAUDE.md` section 11 — the component lifetime pattern.
