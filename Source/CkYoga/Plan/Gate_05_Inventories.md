# Gate 05: Inventories inspector

**Status:** Accepted on the production authored path with fresh real-RHI evidence. No publication is implied.

## Ownership and invariant

`FCkInspector_Inventories` authors the complete inventory, capacity, item, consume, and tag presentation from `EcsInspectorInventories.ui.html/.css` without native ports. C++ remains authoritative for live inventory/item topology, exact route identity, staged values, request admission, public request dispatch, entity selection, native capture/diff, atomic reload, and teardown.

Retained repeat routes use immutable entity identity (`entity ID:generation`) within the inspector's single registry-scoped collection. Human-readable handle strings remain labels only. This distinction is load-bearing: `FCk_Handle::ToString()` changes when an entity moves from `JustCreated` to `Valid`, so it cannot be a retained record key.

## Production-path acceptance

Fresh real-RHI `Ck.UiAuthoring.EcsDebugger.InventoriesInspector.AuthoredComposition` passed 1/1 with zero failed, skipped, or contaminated tests in `scratch/yoga-ecs-inventories-full19-20260913.log` (SHA256 `FACEC16DFB1541EB0AB4632B82A68288181EB3E7C6F6481EC20617D7D24305BB`). The fixture mounts the production inspector in a real Slate window and proves:

- the current stable-key inventory and item controls are attached to the live window and receive routed pointer down/up events;
- selection callbacks receive the exact inventory and item;
- bound override, stack consume, tag addition, and tag removal reach their production processors;
- compatible reload retains the view while a missing-action candidate is rejected atomically;
- relationship loss closes admission and held controls cannot mutate state;
- pending destruction, inspector destruction, and deactivation release or inert the retained authored state.

The compatible fresh real-RHI `Ck.UiAuthoring` family gate then passed 202/202 with zero failed, skipped, or contaminated tests in `scratch/yoga-uiauthoring-inventories-compatible20-20260913.log` (SHA256 `4B81BD80CFA91B4D11B2FE844975B9D0D6FB603526970AD6C31B45CA96EE5361`). It directly includes the Inventories fixture, retained TextInput and Int32Input owner-dispatch coverage, collection integer transport, and retained collection Select keyboard coverage.

Whole-log scans found no relevant ensure, fatal, authored-resource, CSS, parser, AngelScript, or automation failure. The known generator-without-editor warning and an unrelated Chromium USB diagnostic remain outside this gate.

## Remaining campaign boundary

Inventories is the forty-eighth accepted inspector slice in the historical ledger, completing 46 of the 47 registered ECS inspectors in the current source census. StateMachine remains the sole native-source ECS inspector. Fifteen debugger outer shells, the approved local-player/gamepad and two-Slate-user ownership gate, representative measured performance on an identified reference machine, and the production-host every-debugger teardown matrix remain required.

Packaging, keyboard-only traversal, localization, and browser/narrow-breakpoint restacking remain deferred. Resource Inspector `+` remains excluded. Network multiplayer remains conditional.
