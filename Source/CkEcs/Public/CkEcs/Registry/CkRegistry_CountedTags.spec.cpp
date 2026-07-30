// Pins the counted-tag (ck::FTag_CountedTag) contract across ALL tag mutators, through the
// public FCk_Handle surface — no world, no processors. The asymmetry this guards against
// (AddOrGet skipping the increment, Try_Remove hard-erasing every vote) shipped once and
// silently corrupted the 2dGridCell disable refcount; it must fail HERE, not in a gameplay test.

#include "CkCore/Policy/CkPolicy.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Tag/CkTag.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_registry_counted_spec
{
    CK_DEFINE_ECS_TAG_COUNTED(FTag_Spec_Counted);
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Registry_CountedTag_MutatorContract,
    "Ck.CkEcs.Registry.CountedTagMutatorContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Registry_CountedTag_MutatorContract::RunTest(const FString&)
{
    using FTag = ck_registry_counted_spec::FTag_Spec_Counted;

    // A default FCk_Registry is a handle-less view (unset slot) — a live registry is an
    // owned entt registry registered in the slot table (what the EcsWorld subsystems do).
    auto EnttRegistry = ck::registry_table::EnttRegistryType{};
    const auto RegistryHandle = ck::registry_table::Allocate(&EnttRegistry);
    auto Registry = FCk_Registry{RegistryHandle};
    ON_SCOPE_EXIT { ck::registry_table::Free(RegistryHandle); };

    auto Handle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);

    // Add == one more vote
    Handle.Add<FTag>();
    TestTrue(TEXT("Add -> present"), Handle.Has<FTag>());
    TestEqual(TEXT("Add -> count 1"), Handle.Get<FTag>().Get_Count(), 1);

    Handle.Add<FTag>();
    TestEqual(TEXT("second Add -> count 2 (no ensure)"), Handle.Get<FTag>().Get_Count(), 2);

    // AddOrGet on a present counted tag must ALSO vote — a get-without-increment
    // silently corrupts the count (each holder still Removes its vote later)
    Handle.AddOrGet<FTag>();
    TestEqual(TEXT("AddOrGet on present -> count 3"), Handle.Get<FTag>().Get_Count(), 3);

    // Remove == drop one vote, erase only at zero
    Handle.Remove<FTag>();
    TestEqual(TEXT("Remove -> count 2"), Handle.Get<FTag>().Get_Count(), 2);

    // Try_Remove == try-decrement
    TestTrue(TEXT("Try_Remove on present -> true"), Handle.Try_Remove<FTag>());
    TestEqual(TEXT("Try_Remove -> count 1"), Handle.Get<FTag>().Get_Count(), 1);

    TestTrue(TEXT("Try_Remove at count 1 -> true"), Handle.Try_Remove<FTag>());
    TestFalse(TEXT("erased at zero"), Handle.Has<FTag>());
    TestFalse(TEXT("Try_Remove on absent -> false"), Handle.Try_Remove<FTag>());

    // AddOrGet on an absent counted tag adds with count 1
    Handle.AddOrGet<FTag>();
    TestEqual(TEXT("AddOrGet on absent -> count 1"), Handle.Get<FTag>().Get_Count(), 1);

    // ck::policy::ForceErase == wipe every vote at once (the perception-RESET shape)
    Handle.Add<FTag>();
    Handle.Add<FTag>();
    TestEqual(TEXT("wipe setup -> count 3"), Handle.Get<FTag>().Get_Count(), 3);
    TestTrue(TEXT("Try_Remove<ForceErase> on present -> true"),
        Handle.Try_Remove<FTag, ck::policy::ForceErase>());
    TestFalse(TEXT("wiped regardless of count"), Handle.Has<FTag>());
    TestFalse(TEXT("Try_Remove<ForceErase> on absent -> false"),
        Handle.Try_Remove<FTag, ck::policy::ForceErase>());

    // Remove<ForceErase> wipes too — the removal policy is independent of which mutator carries it
    Handle.Add<FTag>();
    Handle.Add<FTag>();
    TestEqual(TEXT("Remove wipe setup -> count 2"), Handle.Get<FTag>().Get_Count(), 2);
    Handle.Remove<FTag, ck::policy::ForceErase>();
    TestFalse(TEXT("Remove<ForceErase> -> wiped regardless of count"), Handle.Has<FTag>());

    // The validation policy still reaches its own slot, alone or alongside the count policy
    Handle.Add<FTag>();
    Handle.Add<FTag>();
    TestTrue(TEXT("Try_Remove<IncludePendingKill> -> decrements"),
        Handle.Try_Remove<FTag, ck::IsValid_Policy_IncludePendingKill>());
    TestEqual(TEXT("validation-policy form still decrements -> count 1"), Handle.Get<FTag>().Get_Count(), 1);

    Handle.Add<FTag>();
    TestTrue(TEXT("both policies spelled -> true"),
        (Handle.Try_Remove<FTag, ck::IsValid_Policy_IncludePendingKill, ck::policy::ForceErase>()));
    TestFalse(TEXT("both policies spelled -> wiped"), Handle.Has<FTag>());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
