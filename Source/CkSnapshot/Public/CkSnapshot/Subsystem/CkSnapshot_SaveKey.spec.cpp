#include "CkSnapshot_Subsystem.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SaveKeyPublicationPolicy,
    "Ck.CkSnapshot.SaveKeyPublicationPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SaveKeyPublicationPolicy::RunTest(const FString&)
{
    auto EnttRegistry = ck::registry_table::EnttRegistryType{};
    const auto RegistryHandle = ck::registry_table::Allocate(&EnttRegistry);
    auto Registry = FCk_Registry{RegistryHandle};
    ON_SCOPE_EXIT { ck::registry_table::Free(RegistryHandle); };

    auto SharedFirst = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto SharedSecond = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto UniqueFirst = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto UniqueSecond = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto MixedUnique = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto AliasOwner = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto StaleFirst = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto StaleReplacement = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);

    const auto SharedIdentity = FString{TEXT("Spec.SaveKey.SharedGroup")};
    const auto UniqueIdentity = FString{TEXT("Spec.SaveKey.Unique")};
    ck::save_key::AssignSharedRendezvousGroup(SharedFirst, SharedIdentity);
    ck::save_key::AssignSharedRendezvousGroup(SharedSecond, SharedIdentity);
    ck::save_key::Assign(UniqueFirst, UniqueIdentity);
    ck::save_key::Assign(UniqueSecond, UniqueIdentity);
    ck::save_key::Assign(MixedUnique, SharedIdentity);
    ck::save_key::Assign(AliasOwner, TEXT("Spec.SaveKey.AliasOwner"));
    ck::save_key::AssignAlias(AliasOwner, SharedIdentity);
    ck::save_key::Assign(StaleFirst, TEXT("Spec.SaveKey.Stale"));
    ck::save_key::Assign(StaleReplacement, TEXT("Spec.SaveKey.Stale"));

    auto* GameInstance = NewObject<UGameInstance>();
    TestNotNull(TEXT("snapshot subsystem game-instance outer is valid"), GameInstance);
    if (GameInstance == nullptr)
    { return false; }

    auto* Subsystem = NewObject<UCk_Snapshot_Subsystem_UE>(GameInstance);
    TestNotNull(TEXT("snapshot subsystem test instance is valid"), Subsystem);
    if (Subsystem == nullptr)
    { return false; }

    const auto SharedKey = SharedFirst.Get<FFragment_SaveKey>().Get_Key();
    TestTrue(TEXT("first shared publisher is accepted"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(SharedKey, SharedFirst));
    TestTrue(TEXT("second explicitly shared publisher is accepted"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(SharedKey, SharedSecond));

    auto Resolved = FCk_Handle{};
    TestTrue(TEXT("shared key remains resolvable"), Subsystem->TryResolve_SaveKey(SharedKey, Resolved));
    TestEqual(TEXT("shared key preserves its first representative"), Resolved, SharedFirst);

    const auto UniqueKey = UniqueFirst.Get<FFragment_SaveKey>().Get_Key();
    TestTrue(TEXT("first unique publisher is accepted"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(UniqueKey, UniqueFirst));
    TestFalse(TEXT("second unique publisher is rejected"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(UniqueKey, UniqueSecond));
    TestTrue(TEXT("unique key remains resolvable"), Subsystem->TryResolve_SaveKey(UniqueKey, Resolved));
    TestEqual(TEXT("unique collision preserves its first publisher"), Resolved, UniqueFirst);

    TestFalse(TEXT("mixed unique publisher cannot join a shared key"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(SharedKey, MixedUnique));
    TestFalse(TEXT("compatibility alias cannot join a shared canonical key"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(SharedKey, AliasOwner));
    TestTrue(TEXT("rejected collisions leave the shared key resolvable"),
        Subsystem->TryResolve_SaveKey(SharedKey, Resolved));
    TestEqual(TEXT("rejected collisions preserve the shared representative"), Resolved, SharedFirst);

    const auto StaleKey = StaleFirst.Get<FFragment_SaveKey>().Get_Key();
    TestTrue(TEXT("first stale publisher is accepted"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(StaleKey, StaleFirst));
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(StaleFirst);
    StaleFirst.Add<ck::FTag_DestroyEntity_Teardown>();
    TestFalse(TEXT("stale publisher enters teardown"), ck::IsValid(StaleFirst));
    TestTrue(TEXT("replacement publisher supersedes a stale mapping"),
        Subsystem->TestOnly_TryPublish_SaveKeyWithoutDiagnostics(StaleKey, StaleReplacement));
    TestTrue(TEXT("replacement key remains resolvable"), Subsystem->TryResolve_SaveKey(StaleKey, Resolved));
    TestEqual(TEXT("replacement supersedes the stale representative"), Resolved, StaleReplacement);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
