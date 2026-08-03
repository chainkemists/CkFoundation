# Silent failure modes — gameplay-facing catalogue

Reference for `ck-game-angelscript-gameplay`: gameplay AS that compiles and hot-reloads but does nothing, with the tell for each.

## 4. Silent failure modes — the gameplay-facing catalogue

The binding-machinery versions of these live in `ck-angelscript-interop` §2 (13 items). Below is
the consumer's seat: what YOU see while writing gameplay, each verified against a real incident.

**4.1 "My mutation didn't happen."**
Symptom: `Request_X` then `Get_X` same frame returns the old value; no error anywhere.
Cause: requests are deferred by contract (`ckecs-architecture-contract` §3).
Fix: read from the change-signal payload or settle a frame (§2.2). If the value *never* changes,
check the entity isn't mid-destruction (requests on dying entities are never consumed) and, in MP,
that you're on the authority (`ck-game-replication-patterns`).

**4.2 "The utils function exists but doesn't resolve."**
Symptom: "No matching signatures" on a namespace-qualified call you can see defined.
Cause: it's a mixin (member-call only), or a non-const `Self&` mixin against a by-value/const
source, or a brand-new C++ util whose wrapper hasn't regenerated yet.
Fix: member form on a mutable local (§2.3); for new C++, build → boot → then compile the AS
(`ck-angelscript-interop` §1.2, item 6). Verified incidents: BusterBlock `ad077a510`, `5c4fd4572`.

**4.3 The ExposeOnSpawn positional-arg freeze.**
Symptom: after touching an EntityScript's `ExposeOnSpawn` properties, the generated `Params(...)`
call no longer matches callers — worst case the editor loops self-heal recoveries at every launch
(BusterBlock incident `01a39b58f`: declaration order diverged from caller arg order → self-heal
synthesized parallel overloads each cycle, ~724 recoveries per session, launch never converged).
Cause: the generated `Params()` signature is **positional in declaration order**. Reordering,
inserting mid-list, or exposing a new field changes the signature for every existing caller.
Fix: append new `ExposeOnSpawn` fields at the end; prefer non-exposed + auto-discover for fields
callers shouldn't pass (BusterBlock `8c7cc4f07` did exactly this to keep `Params()` stable). Treat
an EntityScript's ExposeOnSpawn list as a frozen public API.

**4.4 Runtime-only failures a green compile hides.**
Symptom: clean editor/headless boot, then a throw at PIE-start or when a code path first runs.
Canonical instance: raw handle in an f-string (§2.4). Same class: anything inside a callback that
only executes under real gameplay.
Fix: exercise the path — PIE it or autotest it — before claiming done. A `-skipcompile` boot is a
*compile* gate only.

**4.5 CDO `default` statement gotchas.**
- `default` works on class-level UPROPERTYs only; inside `asset ... of ...` blocks it does not
  compile — assign fields directly (`Script/CLAUDE.md` §13).
- EntityScripts **replicate by default**: `_Replication = ECk_Replication::Replicates` is the C++
  base default (`Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript.h:114`, verified
  2026-07-03). Omitting the `default` line does NOT give you a non-replicating script — spawning
  one under a non-replicated lifetime owner is the classic silent-flood trap
  (`ck-game-replication-patterns` owns the fix: ActorRelay channel owners).
- A parse error anywhere in the file makes every `default X = SomeClass` in it resolve to nothing,
  with symptoms far downstream (§3 step 3).

**4.6 Brand-new dynamic-handle types: first-pass errors are not failures.**
Symptom: adding `FCk_Handle_<X>` produces a wall of "not a data type" + "Hot reload failed" on the
next boot.
Cause: registration lag by design — self-heal writes a stub, the deferred regen writes the real
JSON entry, the recompile goes clean, all in one boot.
Fix: nothing — boot once and gate on the *post-regen* clean reload + the JSON entry. A **second**
boot still red is a real problem (`Script/CLAUDE.md` §7; `ck-angelscript-interop` item 1).

**4.7 Tag-query handlers firing every frame.**
Symptom: a discovery/driver handler shows up hot in `stat CkScript`; frame cost grows with entity
count.
Cause: `EntityTagQuery` continuous-update fires every evaluate pass by design (tested contract),
not only on change.
Fix: delta-gate on the payload's `_Added`/`_Removed` before rebuilding anything
(`ck-game-driver-architecture` owns the full pattern). Verified incident: BusterBlock `531b0c956`
(~250ms/frame before delta-gating).

