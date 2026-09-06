#include "CkEntityTag/CkEntityTag_Processor.h"
#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_EntityTag_FirstMatchingGameplayTag,
    "CkFoundation.Poi.FirstMatchingGameplayTag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_EntityTag_FirstMatchingGameplayTag::RunTest(const FString&)
{
    auto EnttRegistry = ck::registry_table::EnttRegistryType{};
    const auto RegistryHandle = ck::registry_table::Allocate(&EnttRegistry);
    auto Registry = FCk_Registry{RegistryHandle};
    ON_SCOPE_EXIT { ck::registry_table::Free(RegistryHandle); };

    const auto TransientEntityId = FCk_Entity{EnttRegistry.create()};
    Registry.SetContext<ck::FCtx_TransientEntity>(ck::FCtx_TransientEntity{TransientEntityId});

    auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    auto RequestProcessor = ck::FProcessor_EntityTag_HandleRequests{Registry};

    const auto Parent = FGameplayTag::RequestGameplayTag(FName{TEXT("Poi.Category")});
    const auto Area = FGameplayTag::RequestGameplayTag(FName{TEXT("Poi.Category.Area")});
    const auto Danger = FGameplayTag::RequestGameplayTag(FName{TEXT("Poi.Category.Danger")});
    const auto Ping = FGameplayTag::RequestGameplayTag(FName{TEXT("Poi.Category.Ping")});
    const auto NonmatchingParent = FGameplayTag::RequestGameplayTag(FName{TEXT("Poi.State")});

    TestTrue(TEXT("Poi.Category root is registered"), Parent.IsValid());
    TestTrue(TEXT("Poi.Category.Area is registered"), Area.IsValid());
    TestTrue(TEXT("Poi.Category.Danger is registered"), Danger.IsValid());
    TestTrue(TEXT("Poi.Category.Ping is registered"), Ping.IsValid());
    TestTrue(TEXT("Poi.State root is registered"), NonmatchingParent.IsValid());

    const auto TestMatchesLegacyExpression = [this, &Entity](const TCHAR* InContext, FGameplayTag InParent)
    {
        const auto Expected = UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(Entity)
            .Filter(FGameplayTagContainer{InParent})
            .First();
        TestEqual(InContext, UCk_Utils_EntityTag_UE::Get_FirstMatchingGameplayTag(Entity, InParent), Expected);
    };

    // No EntityTag store exists until the real deferred request processor drains a queued add.
    TestMatchesLegacyExpression(TEXT("absent store returns the legacy empty result"), Parent);
    TestMatchesLegacyExpression(TEXT("empty parent returns the legacy empty result"), FGameplayTag{});

    // Root and descendants deliberately arrive out of hierarchy order. The new API must preserve the exact
    // result of the existing Container -> Filter -> First expression rather than imposing another ordering.
    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(Entity, Danger);
    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(Entity, Parent);
    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(Entity, Area);
    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(Entity, Ping);
    RequestProcessor.Pump();
    TestMatchesLegacyExpression(TEXT("root plus descendants preserves legacy first match"), Parent);
    TestEqual(TEXT("first populated match is the first added descendant"),
        UCk_Utils_EntityTag_UE::Get_FirstMatchingGameplayTag(Entity, Parent), Danger);
    TestMatchesLegacyExpression(TEXT("populated empty parent returns the legacy empty result"), FGameplayTag{});
    TestMatchesLegacyExpression(TEXT("nonmatching parent returns the legacy empty result"), NonmatchingParent);

    // Counted mutations are also deferred. Exercise the duplicate and remove transitions through the same
    // processor path, including the swap-removal order that callers previously observed through First().
    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(Entity, Danger);
    RequestProcessor.Pump();
    TestMatchesLegacyExpression(TEXT("duplicate add preserves legacy first match"), Parent);

    UCk_Utils_EntityTag_UE::Request_TryRemove_UsingGameplayTag(Entity, Danger, {});
    RequestProcessor.Pump();
    TestMatchesLegacyExpression(TEXT("first duplicate remove preserves legacy first match"), Parent);
    TestEqual(TEXT("first duplicate remove retains Danger"),
        UCk_Utils_EntityTag_UE::Get_FirstMatchingGameplayTag(Entity, Parent), Danger);

    UCk_Utils_EntityTag_UE::Request_TryRemove_UsingGameplayTag(Entity, Danger, {});
    RequestProcessor.Pump();
    TestMatchesLegacyExpression(TEXT("final duplicate remove preserves legacy first match"), Parent);
    TestNotEqual(TEXT("final duplicate remove changes the first match"),
        UCk_Utils_EntityTag_UE::Get_FirstMatchingGameplayTag(Entity, Parent), Danger);

    UCk_Utils_EntityTag_UE::Request_TryRemove_UsingGameplayTag(Entity, Parent, {});
    UCk_Utils_EntityTag_UE::Request_TryRemove_UsingGameplayTag(Entity, Area, {});
    UCk_Utils_EntityTag_UE::Request_TryRemove_UsingGameplayTag(Entity, Ping, {});
    RequestProcessor.Pump();
    TestMatchesLegacyExpression(TEXT("removed store returns the legacy empty result"), Parent);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
