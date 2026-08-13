#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Whole-registry destruction window.
//
// Deliberately free of any entt include: TFragment_Signal_Delegate's destructor consults this on
// every signal-delegate teardown, so it is pulled into every signal TU. The guarded owning-pointer
// that opens the window lives in CkRegistry_SlotTable.h, which already carries the entt types.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_teardown
{
    /**
     * True while an entt registry is being destroyed wholesale.
     *
     * Destructors that reach ACROSS storage pools cannot run safely in that window. entt's
     * ~basic_registry is `= default`, so its members die in reverse declaration order
     * (`vars, pools, groups, entities` -> entities, groups, pools, vars). The pools themselves die
     * in the dense-map's packed order, and because the registry never erases a pool that order IS
     * insertion order — so this is systematic, not a race: Bind assures the signal pool before the
     * delegate pool (CkSignal_Utils.inl.h), which means the sigh is destroyed first EVERY time.
     * That is why packaged builds died on every save/load rather than intermittently.
     *
     * The case this exists for: a signal's entt::sigh lives in TFragment_Signal while the
     * entt::connection into it lives in TFragment_Signal_Delegate, a different fragment in a
     * different pool. Whichever pool dies second reaches into freed memory. Skipping the release
     * is not a papering-over — the sigh owns its subscriber list outright and is being destroyed in
     * the same operation, so there is genuinely nothing to disconnect from and nothing leaks.
     *
     * NOT solvable by clearing first: registry.clear() walks pools in reverse insertion order,
     * which today happens to clear delegate pools before signal pools only because Bind assures the
     * signal pool first (CkSignal_Utils.inl.h) — an accident no contract preserves. It would also
     * fire on_destroy listeners across the whole registry mid-teardown, and cost an O(entities)
     * erase pass, to buy an ordering it does not actually guarantee.
     *
     * Exported rather than an inline variable so modular (editor) builds share ONE counter across
     * DLLs; a per-module copy would read false wherever the destructor happened to be instantiated.
     */
    CKECS_API auto Get_IsInProgress() -> bool;

    // --------------------------------------------------------------------------------------------------------------------

    /**
     * Opens the window for its scope. Nestable — a nested private world (a 2dGridSystem cell
     * registry owned by a main-registry entity) legitimately tears down inside its owner's window,
     * and suppression is correct for both because both registries are dying.
     *
     * INVARIANT this relies on, and which nothing enforces: the flag is process-global, so it is
     * only sound because no fragment destructor mutates a DIFFERENT registry. That holds today —
     * TFragment_Signal_Delegate is the only fragment destructor in the codebase carrying
     * logic, and it touches nothing but its own connection. A fragment destructor that reached into
     * another, still-live registry would have its release wrongly suppressed here, leaving that
     * registry's sigh holding a dangling connection. If you write one, make this per-registry.
     *
     * The window also cannot reach payloads held in the registry's ctx (`vars`), which is declared
     * first and therefore destroyed LAST, after the pools. FCtx_DebugFeatureFlags holds
     * entt::scoped_connections into per-pool sighs; ~scoped_connection is library code that never
     * consults this flag, so that is the same use-after-free class left uncovered. Debugger-gated
     * today (nothing on a teardown path calls debug_feature_flags::Disable).
     *
     * Prefer ck::registry_table::TGuardedRegistryPtr over placing this by hand: the owning pointer
     * takes the window on EVERY destruction path, including ones nobody has written yet.
     */
    struct CKECS_API FScopedGuard
    {
    public:
        FScopedGuard();
        ~FScopedGuard();

        FScopedGuard(const FScopedGuard&) = delete;
        auto operator=(const FScopedGuard&) -> FScopedGuard& = delete;
        FScopedGuard(FScopedGuard&&) = delete;
        auto operator=(FScopedGuard&&) -> FScopedGuard& = delete;
    };
}

// --------------------------------------------------------------------------------------------------------------------
