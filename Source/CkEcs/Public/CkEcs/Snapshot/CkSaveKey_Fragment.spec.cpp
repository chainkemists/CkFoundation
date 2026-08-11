// SaveKey aliases are load-only compatibility identities. They must be unique,
// must not replace the canonical key, and must disappear when a caller assigns
// a new canonical identity.

#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_SaveKey_CompatibilityAliases,
    "Ck.CkEcs.Snapshot.SaveKeyCompatibilityAliases",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_SaveKey_CompatibilityAliases::RunTest(const FString&)
{
    const auto* KeyProperty = FindFProperty<FProperty>(
        FFragment_SaveKey::StaticStruct(), TEXT("_Key"));
    const auto* AliasesProperty = FindFProperty<FProperty>(
        FFragment_SaveKey::StaticStruct(), TEXT("_Aliases"));
    const auto* SharedGroupProperty = FindFProperty<FProperty>(
        FFragment_SaveKey::StaticStruct(), TEXT("_IsSharedRendezvousGroup"));
    TestNotNull(TEXT("canonical SaveKey property remains reflected"), KeyProperty);
    TestNotNull(TEXT("compatibility aliases property remains reflected"), AliasesProperty);
    TestNotNull(TEXT("shared-rendezvous policy remains reflected"), SharedGroupProperty);
    if (KeyProperty != nullptr)
    {
        TestTrue(TEXT("canonical SaveKey is persisted"),
            KeyProperty->HasAnyPropertyFlags(CPF_SaveGame));
    }
    if (AliasesProperty != nullptr)
    {
        TestTrue(TEXT("compatibility aliases are transient"),
            AliasesProperty->HasAnyPropertyFlags(CPF_Transient));
        TestFalse(TEXT("compatibility aliases never enter save archives"),
            AliasesProperty->HasAnyPropertyFlags(CPF_SaveGame));
    }
    if (SharedGroupProperty != nullptr)
    {
        TestTrue(TEXT("shared-rendezvous policy is transient"),
            SharedGroupProperty->HasAnyPropertyFlags(CPF_Transient));
        TestFalse(TEXT("shared-rendezvous policy never enters save archives"),
            SharedGroupProperty->HasAnyPropertyFlags(CPF_SaveGame));
    }

    auto EnttRegistry = ck::registry_table::EnttRegistryType{};
    const auto RegistryHandle = ck::registry_table::Allocate(&EnttRegistry);
    auto Registry = FCk_Registry{RegistryHandle};
    ON_SCOPE_EXIT { ck::registry_table::Free(RegistryHandle); };

    auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto SharedEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    const auto CanonicalIdentity = FString{TEXT("Spec.SaveKey.Current")};
    const auto HistoricalIdentity = FString{TEXT("Spec.SaveKey.Legacy")};
    const auto NextIdentity = FString{TEXT("Spec.SaveKey.Next")};

    ck::save_key::Assign(Entity, CanonicalIdentity);
    ck::save_key::AssignAlias(Entity, HistoricalIdentity);
    ck::save_key::AssignAlias(Entity, HistoricalIdentity);

    const auto& SaveKey = Entity.Get<FFragment_SaveKey>();
    TestEqual(TEXT("canonical key remains the current identity"),
        SaveKey.Get_Key(), FGuid::NewDeterministicGuid(CanonicalIdentity));
    TestEqual(TEXT("repeated compatibility alias is unique"), SaveKey.Get_Aliases().Num(), 1);
    if (SaveKey.Get_Aliases().Num() == 1)
    {
        TestEqual(TEXT("compatibility alias uses the historical identity"),
            SaveKey.Get_Aliases()[0], FGuid::NewDeterministicGuid(HistoricalIdentity));
    }

    ck::save_key::Assign(Entity, NextIdentity);
    const auto& Reassigned = Entity.Get<FFragment_SaveKey>();
    TestEqual(TEXT("canonical reassignment updates the persisted identity"),
        Reassigned.Get_Key(), FGuid::NewDeterministicGuid(NextIdentity));
    TestEqual(TEXT("canonical reassignment clears transient compatibility aliases"),
        Reassigned.Get_Aliases().Num(), 0);
    TestFalse(TEXT("ordinary assignment remains uniquely keyed"),
        Reassigned.Get_IsSharedRendezvousGroup());

    ck::save_key::AssignSharedRendezvousGroup(SharedEntity, CanonicalIdentity);
    const auto& Shared = SharedEntity.Get<FFragment_SaveKey>();
    TestEqual(TEXT("shared rendezvous uses the canonical identity"),
        Shared.Get_Key(), FGuid::NewDeterministicGuid(CanonicalIdentity));
    TestTrue(TEXT("shared rendezvous is explicitly marked"),
        Shared.Get_IsSharedRendezvousGroup());

    ck::save_key::Assign(SharedEntity, NextIdentity);
    TestFalse(TEXT("ordinary reassignment clears shared rendezvous policy"),
        SharedEntity.Get<FFragment_SaveKey>().Get_IsSharedRendezvousGroup());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
