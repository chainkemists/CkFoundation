#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"

#include "Kismet/GameplayStatics.h"

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_slot_meta_spec
{
    constexpr auto UserIndex = 0;

    // Deliberately not a name any game would pick, so a failed cleanup cannot shadow a real slot.
    const auto SpecSlotName = FName{TEXT("ck.spec.slotmeta")};
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SlotMeta_Naming,
    "Ck.CkSnapshot.SlotMeta.Naming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SlotMeta_Naming::RunTest(const FString&)
{
    using namespace ck::snapshot::slot_meta;

    TestEqual(TEXT("A sidecar name is the slot name plus the suffix"),
        Get_MetaSlotName(FName{TEXT("Slot0")}), FString{TEXT("Slot0.meta")});

    TestTrue(TEXT("A sidecar name is recognised as one"),
        Get_IsMetaSlotName(Get_MetaSlotName(FName{TEXT("Slot0")})));

    // The whole point of the predicate: enumeration returns sidecars alongside snapshots, so a slot
    // list that does not filter shows every save twice.
    TestFalse(TEXT("A plain slot name is not a sidecar"),
        Get_IsMetaSlotName(TEXT("Slot0")));
    TestFalse(TEXT("A slot merely CONTAINING the suffix is not a sidecar"),
        Get_IsMetaSlotName(TEXT("Slot0.meta.backup")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SlotMeta_RoundTrip,
    "Ck.CkSnapshot.SlotMeta.RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SlotMeta_RoundTrip::RunTest(const FString&)
{
    using namespace ck_snapshot_slot_meta_spec;

    const auto MetaSlot = ck::snapshot::slot_meta::Get_MetaSlotName(SpecSlotName);

    // A default meta is distinguishable from a real one — the menu relies on this to tell "no
    // sidecar on disk" from "a sidecar that happens to be untitled".
    TestFalse(TEXT("A default-constructed meta reports itself unpopulated"),
        FCk_Snapshot_SlotMeta{}.Get_IsPopulated());

    auto* Written = Cast<UCk_Snapshot_SlotMetaSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SlotMetaSaveGame::StaticClass()));

    if (NOT TestNotNull(TEXT("The sidecar SaveGame object is created"), Written))
    { return false; }

    const auto Timestamp = FDateTime::UtcNow();
    auto CustomFields = TMap<FName, FString>{};
    CustomFields.Emplace(FName{TEXT("StoreLevel")}, TEXT("7"));
    CustomFields.Emplace(FName{TEXT("StoreMoney")}, TEXT("1234.50"));

    Written->_Meta.Set_SlotName(SpecSlotName)
                  .Set_Title(FText::FromString(TEXT("My Store")))
                  .Set_TimestampUTC(Timestamp)
                  .Set_WorldAssetPath(FSoftObjectPath{TEXT("/Game/Maps/Spec.Spec")})
                  .Set_ScreenshotPng(TArray<uint8>{1, 2, 3, 4})
                  .Set_CustomFields(CustomFields);

    if (NOT TestTrue(TEXT("The sidecar writes to its slot"),
            UGameplayStatics::SaveGameToSlot(Written, MetaSlot, UserIndex)))
    { return false; }

    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(MetaSlot, UserIndex); };

    auto* ReadBack = Cast<UCk_Snapshot_SlotMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(MetaSlot, UserIndex));

    if (NOT TestNotNull(TEXT("The sidecar reads back"), ReadBack))
    { return false; }

    const auto& Meta = ReadBack->_Meta;

    TestTrue(TEXT("A written meta reports itself populated"), Meta.Get_IsPopulated());
    TestEqual(TEXT("Slot name round-trips"), Meta.Get_SlotName(), SpecSlotName);
    TestEqual(TEXT("Title round-trips"), Meta.Get_Title().ToString(), FString{TEXT("My Store")});
    TestEqual(TEXT("Timestamp round-trips"), Meta.Get_TimestampUTC(), Timestamp);
    // The load path travels to this, so a lossy round-trip here sends a load to the wrong map.
    TestEqual(TEXT("World asset path round-trips"),
        Meta.Get_WorldAssetPath().ToString(), FString{TEXT("/Game/Maps/Spec.Spec")});
    TestEqual(TEXT("Screenshot bytes round-trip"), Meta.Get_ScreenshotPng().Num(), 4);
    TestEqual(TEXT("Custom fields round-trip"), Meta.Get_CustomFields().Num(), 2);

    const auto* StoreLevel = Meta.Get_CustomFields().Find(FName{TEXT("StoreLevel")});
    if (TestNotNull(TEXT("A custom field survives by key"), StoreLevel))
    { TestEqual(TEXT("A custom field survives by value"), *StoreLevel, FString{TEXT("7")}); }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
