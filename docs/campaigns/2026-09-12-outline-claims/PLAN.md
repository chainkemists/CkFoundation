# Plan

## Gate 1 - Claim contract and resolver

Add validated project settings, source-owned claims, deterministic resolution, live lifetime-chain
scope, strict failure behavior, and focused C++ policy tests.

Exit: malformed settings/requests cannot mutate or partially resolve; overlapping claims resolve and
reveal deterministically; late/reparented descendants inherit live subtree claims.

## Gate 2 - Render consumers and component ownership

Move actor, ISM, ISKM, cel-pattern, and stylize-mask precedence to the resolved fragment. Preserve
applied-state teardown and restore pre-existing primitive custom-depth state only when still owned.

Exit: focused actor/ISM/ISKM outline tests pass, removal restores state, and losing claims never leave
stale renderer state.

## Gate 3 - Selection and BusterBlock migration

Add the default-on Selection setting to the already shared panel, reconcile one selection claim over
focus lifecycle, and replace BusterBlock local ownership arbitration while retaining renderable
registration needed by raw and proxy paths.

Exit: selection focus/disable/deactivate paths release exactly their claim; BusterBlock callers use
semantic tags and no local claim refcount remains.

## Final gates

1. CkPlugins Development Editor build plus focused CkUsf outline contract/renderer tests.
2. CkPlugins test-only DebugOverlay settings/lifecycle coverage on the same binaries.
3. BusterBlock_Other Development Editor build plus focused migrated outline tests.
4. Inspect fresh logs for unexpected ensures, AngelScript errors, test contamination, and first causal
   failures; report focused evidence only.
