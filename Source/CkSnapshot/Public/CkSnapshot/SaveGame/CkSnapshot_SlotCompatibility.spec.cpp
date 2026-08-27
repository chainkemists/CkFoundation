#include "CkSnapshot/SaveGame/CkSnapshot_SaveGame.h"

#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"

#include "Kismet/GameplayStatics.h"

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_slot_compat_spec
{
    constexpr auto UserIndex = 0;

    // Deliberately not a name any game would pick, so a failed cleanup cannot shadow a real slot.
    const auto SpecSlotName = FName{TEXT("ck.spec.slotcompat")};
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SlotCompatibility_AbsentSlot,
    "Ck.CkSnapshot.SlotCompatibility.AbsentSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SlotCompatibility_AbsentSlot::RunTest(const FString&)
{
    using namespace ck_snapshot_slot_compat_spec;

    TestFalse(TEXT("An absent slot holds no compatible save"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(SpecSlotName, UserIndex));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The exact shape a legacy SPUD-era save has: a file the platform save system reports as EXISTING, whose bytes are
// not an engine save envelope at all. Existence saying "occupied" while the load gate says "no" is the undead-slot
// trap this predicate exists to close.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SlotCompatibility_ForeignFile,
    "Ck.CkSnapshot.SlotCompatibility.ForeignFile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SlotCompatibility_ForeignFile::RunTest(const FString&)
{
    using namespace ck_snapshot_slot_compat_spec;

    auto ForeignBytes = TArray<uint8>{};
    for (const auto Char : FString{TEXT("SPUD-this-is-not-an-engine-save-envelope")})
    { ForeignBytes.Add(static_cast<uint8>(Char)); }

    if (NOT TestTrue(TEXT("The foreign bytes write to the slot"),
            UGameplayStatics::SaveDataToSlot(ForeignBytes, SpecSlotName.ToString(), UserIndex)))
    { return false; }

    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(SpecSlotName.ToString(), UserIndex); };

    TestTrue(TEXT("Existence still reports the slot occupied — that is the trap"),
        UGameplayStatics::DoesSaveGameExist(SpecSlotName.ToString(), UserIndex));

    TestFalse(TEXT("A foreign file is not a compatible save"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(SpecSlotName, UserIndex));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// A valid engine envelope of the WRONG USaveGame class (the sidecar class doubles as a convenient foreign class):
// the envelope parses, the cast is what must refuse it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SlotCompatibility_ForeignSaveGameClass,
    "Ck.CkSnapshot.SlotCompatibility.ForeignSaveGameClass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SlotCompatibility_ForeignSaveGameClass::RunTest(const FString&)
{
    using namespace ck_snapshot_slot_compat_spec;

    auto* Foreign = Cast<UCk_Snapshot_SlotMetaSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SlotMetaSaveGame::StaticClass()));

    if (NOT TestNotNull(TEXT("The foreign-class SaveGame object is created"), Foreign))
    { return false; }

    if (NOT TestTrue(TEXT("The foreign-class SaveGame writes to the slot"),
            UGameplayStatics::SaveGameToSlot(Foreign, SpecSlotName.ToString(), UserIndex)))
    { return false; }

    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(SpecSlotName.ToString(), UserIndex); };

    TestFalse(TEXT("A different USaveGame class is not a compatible save"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(SpecSlotName, UserIndex));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The version/payload half of the gate, exercised through a genuine slot round-trip: stale format version and empty
// payload each refuse; a current-version save with a payload accepts.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_SlotCompatibility_PayloadGate,
    "Ck.CkSnapshot.SlotCompatibility.PayloadGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_SlotCompatibility_PayloadGate::RunTest(const FString&)
{
    using namespace ck_snapshot_slot_compat_spec;

    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(SpecSlotName.ToString(), UserIndex); };

    const auto WriteSlot = [&](uint16 InFormatVersion, TArray<uint8> InPayloadBytes) -> bool
    {
        auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(
            UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SaveGame::StaticClass()));

        if (NOT TestNotNull(TEXT("The CkSnapshot SaveGame object is created"), SaveGame))
        { return false; }

        SaveGame->_HeaderV3.Set_FormatVersion(InFormatVersion);
        SaveGame->_SnapshotBytesV3 = MoveTemp(InPayloadBytes);

        return TestTrue(TEXT("The CkSnapshot SaveGame writes to the slot"),
            UGameplayStatics::SaveGameToSlot(SaveGame, SpecSlotName.ToString(), UserIndex));
    };

    if (NOT WriteSlot(FCk_Snapshot_HeaderV3::CurrentFormatVersion - 1, TArray<uint8>{1, 2, 3}))
    { return false; }
    TestFalse(TEXT("A stale format version is not compatible"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(SpecSlotName, UserIndex));

    if (NOT WriteSlot(FCk_Snapshot_HeaderV3::CurrentFormatVersion, TArray<uint8>{}))
    { return false; }
    TestFalse(TEXT("An empty payload is not compatible"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(SpecSlotName, UserIndex));

    if (NOT WriteSlot(FCk_Snapshot_HeaderV3::CurrentFormatVersion, TArray<uint8>{1, 2, 3}))
    { return false; }
    TestTrue(TEXT("A current-version save with a payload is compatible"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(SpecSlotName, UserIndex));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
