#include "CkEntityLifetime_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        IsDestructionPhase::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, _DestructionPhase);
    }

    auto
        Is_NOT_DestructionPhase::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return NOT UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, _DestructionPhase);
    }
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Entity Destruction Pipeline
 * ===========================
 *
 * When Destroy(Entity) is called, the entity goes through a multi-frame destruction process
 * to ensure all systems have proper opportunity to clean up before the entity is invalidated.
 *
 * Overview:
 * ---------
 * Frame N:     DestroyEntity_Initiate → DestroyEntity_EndPlay → DestroyEntity_Teardown (at end of frame)
 * Frame N+1:   DestroyEntity_Teardown → DestroyEntity_Await (guaranteed full frame)
 * Frame N+2:   DestroyEntity_Await → DestroyEntity_Finalize → Entity Destroyed (sequential at start of frame)
 *
 *
 * Detailed Flow:
 * ==============
 *
 * Frame N (End of Frame)         Frame N+1                    Frame N+2 (Start of Frame)
 * ┌──────────────────────┐      ┌──────────────────────┐     ┌────────────────────────┐
 * │ DestroyEntity_       │      │ DestroyEntity_       │     │ Sequential processing: │
 * │ Initiate             │      │ Teardown             │     │                        │
 * │        ↓             │      │        ↓             │     │ Teardown →             │
 * │ DestroyEntity_       │      │ DestroyEntity_       │     │ Await →                │
 * │ EndPlay              │      │ Await                │ ───►│ Finalize →             │
 * │        ↓             │      │                      │     │ Entity Destroyed       │
 * │ EntityScript_EndPlay │      │ Entity handle VALID  │     │                        │
 * │ (injected)           │      │ but component data   │     │ Entity INVALID         │
 * │        ↓             │      │ may be torn down     │     │ Memory reclaimed       │
 * │ DestroyEntity_       │      │                      │     │                        │
 * │ Teardown             │      │ GUARANTEED full      │     │                        │
 * │                      │      │ frame phase          │     │                        │
 * │ Tags accumulate:     │      │                      │     │                        │
 * │ Initiate + EndPlay + │      │ Processors can work  │     │                        │
 * │ Teardown             │      │ with Await entities  │     │                        │
 * │                      │      │ but must handle      │     │                        │
 * │                      │      │ missing/invalid data │     │                        │
 * └──────────────────────┘      └──────────────────────┘     └────────────────────────┘
 *
 *    Destroy(Entity)              Await processor runs         Finalize → DestroyEntity
 *    called anytime               at start of Frame N+1        processors run sequentially
 *    during frame
 *
 *
 * Processor Execution Order:
 * ==========================
 *
 * Start of Frame Processors:
 * ├── FProcessor_OwningActor_Destroy
 * ├── FProcessor_EntityLifetime_DestructionPhase_Finalize
 * └── FProcessor_EntityLifetime_DestructionPhase_Await
 *
 * ... (other regular processors and gameplay pump) ...
 *
 * End of Frame Processors:
 * ├── FProcessor_EntityLifetime_EntityJustCreated
 * ├── FProcessor_EntityLifetime_DestroyEntity
 * ├── FProcessor_EntityLifetime_DestructionPhase_Endplay
 * ├── FProcessor_EntityScript_EndPlay (injected before Teardown)
 * └── FProcessor_EntityLifetime_DestructionPhase_Teardown
 *
 *
 * Tag Definitions & Processor Macros:
 * ====================================
 *
 * Tags (accumulate during destruction):
 * - FTag_DestroyEntity_Initiate
 * - FTag_DestroyEntity_EndPlay
 * - FTag_DestroyEntity_Teardown
 * - FTag_DestroyEntity_Await
 * - FTag_DestroyEntity_Finalize
 *
 * Common Processor Macros:
 * - CK_IGNORE_PENDING_KILL: Excludes EndPlay, Teardown, Await, Finalize tags
 * - CK_IF_END_PLAY: Requires EndPlay, excludes Teardown/Await/Finalize
 * - CK_IF_TEARING_DOWN: Requires Teardown, excludes Await/Finalize
 *
 *
 * Key Guarantees & Design Rationale:
 * ==================================
 *
 * 1. INITIATE Phase (Frame N):
 *    - Entity remains valid, gets Initiate tag
 *    - Destroy() can be called anytime during frame
 *    - Not all regular processors guaranteed to run with Initiate state (timing dependent)
 *
 * 2. ENDPLAY Phase (Frame N - End of Frame):
 *    - DestructionPhase_Endplay processor adds EndPlay tag
 *    - EntityScript_EndPlay processor runs (uses CK_IF_END_PLAY macro)
 *    - This is where EndPlay events are triggered for systems that care (currently EntityScripts)
 *
 * 3. TEARDOWN Phase (Frame N - End of Frame):
 *    - "Helper phase" for cascading destruction edge case (renamed from old Initiate_Confirm)
 *    - Problem: When teardown processors run, they may trigger destruction of OTHER entities
 *    - Without Teardown phase, newly-destroyed entities would skip to Await immediately
 *    - Solution: Teardown phase ensures cascading destructions still get proper processing
 *    - DestructionPhase_Teardown processor adds Teardown tag
 *    - Tags accumulated: Initiate + EndPlay + Teardown
 *
 * 4. AWAIT Phase (Frame N+1):
 *    - The ONLY guaranteed full-frame destruction phase
 *    - DestructionPhase_Await processor (runs at start of N+1) adds Await tag
 *    - Entity handle remains VALID but component data may be torn down
 *    - Processors can work with Await entities but must handle missing/invalid data gracefully
 *    - Entity data has no guarantees after teardown processors have run
 *
 * 5. FINALIZE → DESTROY Phase (Frame N+2 - Start of Frame):
 *    - Sequential processing:
 *      1. DestructionPhase_Finalize processes Await → adds Finalize tag
 *      2. DestroyEntity processor processes Finalize → destroys entity completely
 *    - Entity becomes INVALID and memory is reclaimed
 *    - No processors run between these sequential operations
 *
 */

// --------------------------------------------------------------------------------------------------------------------
