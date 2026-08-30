#include "CkSnapshot/SaveGame/CkSnapshot_SaveGame.h"

#include "Kismet/GameplayStatics.h"

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_header_provenance_spec
{
    constexpr auto UserIndex = 0;

    // Deliberately not a name any game would pick, so a failed cleanup cannot shadow a real slot - and one
    // name PER TEST, because the toolbox sizes concurrent editor lanes to the machine and two tests sharing a
    // slot would race each other's cleanup into a red on whichever ran second.
    const auto RoundTripSlotName = FName{TEXT("ck.spec.headerprovenance.roundtrip")};
    const auto UnstampedSlotName = FName{TEXT("ck.spec.headerprovenance.unstamped")};

    // The property tag as the save archive writes it. USaveGame serialization runs through
    // FObjectAndNameAsStringProxyArchive, so a serialized property's NAME lands in the bytes as a plain ANSI
    // string. That is what lets the second test below ask whether the property was written AT ALL, rather than
    // only what it deserialized to - the difference between proving backward compatibility and assuming it.
    const auto ProjectVersionTag = FString{TEXT("_ProjectVersion")};

    auto
        Get_BytesContainAnsi(
            const TArray<uint8>& InBytes,
            const FString& InNeedle)
        -> bool
    {
        if (InNeedle.IsEmpty() || InBytes.Num() < InNeedle.Len())
        { return false; }

        for (auto Start = 0; Start <= InBytes.Num() - InNeedle.Len(); ++Start)
        {
            auto Matches = true;

            for (auto Offset = 0; Offset < InNeedle.Len(); ++Offset)
            {
                if (InBytes[Start + Offset] == static_cast<uint8>(InNeedle[Offset]))
                { continue; }

                Matches = false;
                break;
            }

            if (Matches)
            { return true; }
        }

        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

// The stamp survives a real slot round-trip. Without this the field could be written and silently dropped by the
// serializer, which is precisely how _PluginBuildHash beside it reads back as zeros in every save ever produced.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_HeaderProvenance_ProjectVersionRoundTrips,
    "Ck.CkSnapshot.HeaderProvenance.ProjectVersionRoundTrips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_HeaderProvenance_ProjectVersionRoundTrips::RunTest(const FString&)
{
    using namespace ck_snapshot_header_provenance_spec;

    const auto StampedVersion = FString{TEXT("9.9.9-spec")};
    const auto StampedBuildId = FString{TEXT("deadbeef1")};

    auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(
        UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SaveGame::StaticClass()));

    if (NOT TestNotNull(TEXT("The CkSnapshot SaveGame object is created"), SaveGame))
    { return false; }

    SaveGame->_HeaderV3.Set_FormatVersion(FCk_Snapshot_HeaderV3::CurrentFormatVersion);
    SaveGame->_HeaderV3.Set_ProjectVersion(StampedVersion);
    SaveGame->_HeaderV3.Set_BuildId(StampedBuildId);
    SaveGame->_SnapshotBytesV3 = TArray<uint8>{1, 2, 3};

    if (NOT TestTrue(TEXT("The stamped SaveGame writes to the slot"),
            UGameplayStatics::SaveGameToSlot(SaveGame, RoundTripSlotName.ToString(), UserIndex)))
    { return false; }

    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(RoundTripSlotName.ToString(), UserIndex); };

    auto* Reloaded = Cast<UCk_Snapshot_SaveGame>(
        ck::snapshot::TryLoad_SlotSaveGame_Guarded(RoundTripSlotName.ToString(), UserIndex));

    if (NOT TestNotNull(TEXT("The stamped slot reloads as a CkSnapshot SaveGame"), Reloaded))
    { return false; }

    TestEqual(TEXT("The writing game version survives the round trip"),
        Reloaded->_HeaderV3.Get_ProjectVersion(), StampedVersion);

    TestEqual(TEXT("The writing build id survives the round trip"),
        Reloaded->_HeaderV3.Get_BuildId(), StampedBuildId);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// A save written with NO project version is byte-identical in shape to one written before the field existed: tagged
// property serialization skips a property equal to its class default, so an empty stamp is not written at all. That
// makes the pre-change save reproducible here rather than something the change has to be TRUSTED not to break - the
// test asserts the tag is genuinely absent from the bytes, then that such a slot still loads and reads as empty.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_HeaderProvenance_UnstampedSaveStaysLoadable,
    "Ck.CkSnapshot.HeaderProvenance.UnstampedSaveStaysLoadable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_HeaderProvenance_UnstampedSaveStaysLoadable::RunTest(const FString&)
{
    using namespace ck_snapshot_header_provenance_spec;

    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(UnstampedSlotName.ToString(), UserIndex); };

    const auto WriteSlot = [&](const FString& InProjectVersion) -> bool
    {
        auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(
            UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SaveGame::StaticClass()));

        if (NOT TestNotNull(TEXT("The CkSnapshot SaveGame object is created"), SaveGame))
        { return false; }

        SaveGame->_HeaderV3.Set_FormatVersion(FCk_Snapshot_HeaderV3::CurrentFormatVersion);
        SaveGame->_HeaderV3.Set_ProjectVersion(InProjectVersion);
        SaveGame->_SnapshotBytesV3 = TArray<uint8>{1, 2, 3};

        return TestTrue(TEXT("The CkSnapshot SaveGame writes to the slot"),
            UGameplayStatics::SaveGameToSlot(SaveGame, UnstampedSlotName.ToString(), UserIndex));
    };

    const auto Get_SlotBytes = [&]() -> TArray<uint8>
    {
        auto Bytes = TArray<uint8>{};
        UGameplayStatics::LoadDataFromSlot(Bytes, UnstampedSlotName.ToString(), UserIndex);
        return Bytes;
    };

    // Control: a stamped save DOES carry the property tag, so a false negative below cannot pass as a pass.
    if (NOT WriteSlot(FString{TEXT("9.9.9-spec")}))
    { return false; }

    if (NOT TestTrue(TEXT("A stamped save carries the ProjectVersion property tag in its bytes"),
            Get_BytesContainAnsi(Get_SlotBytes(), ProjectVersionTag)))
    { return false; }

    // The pre-change byte shape: no stamp means no tag written.
    if (NOT WriteSlot(FString{}))
    { return false; }

    TestFalse(TEXT("An unstamped save omits the ProjectVersion tag entirely - the pre-change byte shape"),
        Get_BytesContainAnsi(Get_SlotBytes(), ProjectVersionTag));

    TestTrue(TEXT("A save with no ProjectVersion tag is still a compatible save"),
        ck::snapshot::Get_SlotHoldsCompatibleSave(UnstampedSlotName, UserIndex));

    auto* Reloaded = Cast<UCk_Snapshot_SaveGame>(
        ck::snapshot::TryLoad_SlotSaveGame_Guarded(UnstampedSlotName.ToString(), UserIndex));

    if (NOT TestNotNull(TEXT("The unstamped slot reloads as a CkSnapshot SaveGame"), Reloaded))
    { return false; }

    TestTrue(TEXT("An absent ProjectVersion tag reads back as empty, which every consumer treats as oldest"),
        Reloaded->_HeaderV3.Get_ProjectVersion().IsEmpty());

    TestTrue(TEXT("An absent BuildId tag likewise reads back as empty rather than as garbage"),
        Reloaded->_HeaderV3.Get_BuildId().IsEmpty());

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The comparator exists so nobody compares these as strings. The lexicographic trap is not hypothetical: as text
// "1.0.10" sorts BELOW "1.0.9", which inverts any gate scoping behaviour to the builds before a fix.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Snapshot_HeaderProvenance_CompareProjectVersions,
    "Ck.CkSnapshot.HeaderProvenance.CompareProjectVersions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Snapshot_HeaderProvenance_CompareProjectVersions::RunTest(const FString&)
{
    const auto Older = [&](const TCHAR* InA, const TCHAR* InB)
    {
        TestTrue(FString::Printf(TEXT("[%s] is older than [%s]"), InA, InB),
            ck::snapshot::Compare_ProjectVersions(FString{InA}, FString{InB}) < 0);
        TestTrue(FString::Printf(TEXT("...and [%s] is newer than [%s] (antisymmetric)"), InB, InA),
            ck::snapshot::Compare_ProjectVersions(FString{InB}, FString{InA}) > 0);
    };

    const auto Same = [&](const TCHAR* InA, const TCHAR* InB)
    {
        TestEqual(FString::Printf(TEXT("[%s] equals [%s]"), InA, InB),
            ck::snapshot::Compare_ProjectVersions(FString{InA}, FString{InB}), 0);
    };

    Older(TEXT("1.0.3"),  TEXT("1.0.4"));
    Older(TEXT("1.0.9"),  TEXT("1.0.10"));   // the whole reason this function exists
    Older(TEXT("1.9.0"),  TEXT("2.0.0"));
    Older(TEXT("1.0"),    TEXT("1.0.1"));

    Same(TEXT("1.0.3"), TEXT("1.0.3"));
    Same(TEXT("1.0"),   TEXT("1.0.0"));      // a missing trailing segment is zero

    // A suffix carries no order: a hotfix that must be distinguishable needs a numeric bump, not a label.
    Same(TEXT("1.0.3-hotfix1"), TEXT("1.0.3"));
    Older(TEXT("1.0.3-hotfix1"), TEXT("1.0.4"));

    // Unparseable is older than everything parseable, which keeps an unstamped save on the compensating side of
    // a gate rather than silently excluded from it.
    Older(TEXT(""),      TEXT("0.0.1"));
    Older(TEXT("beta"),  TEXT("1.0.0"));
    Same (TEXT(""),      TEXT("nonsense"));  // nothing distinguishes two unparseable versions

    // The trap stated directly: string ordering disagrees with version ordering here.
    TestTrue(TEXT("string compare would get 1.0.10 vs 1.0.9 backwards"),
        FString{TEXT("1.0.10")} < FString{TEXT("1.0.9")});
    TestTrue(TEXT("and the comparator gets it right"),
        ck::snapshot::Compare_ProjectVersions(FString{TEXT("1.0.10")}, FString{TEXT("1.0.9")}) > 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
