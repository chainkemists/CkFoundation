#include "CkSnapshot/Inspection/CkSnapshot_Inspection_Diff.h"

#include "CkSnapshot/Inspection/CkSnapshot_Inspection.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Misc/Guid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_SnapshotInspection_Diff::
    Get_GroupCountOfKind(
        ECk_SnapshotInspection_DiffGroupKind InKind) const
    -> int32
{
    auto Count = 0;
    for (const auto& Group : _EntityGroups)
    {
        if (Group.Get_Kind() == InKind)
        { ++Count; }
    }
    return Count;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_inspection_diff
    {
        constexpr auto k_RootMarker = TEXT("~");
        constexpr auto k_CycleMarker = TEXT("cycle~");
        constexpr auto k_PathSeparator = TEXT("/");

        enum class ECk_DiffSide : uint8
        {
            Baseline,
            Current,
        };

        // ------------------------------------------------------------------------------------------------------------

        auto
            Get_LocalIdentity(
                const FCk_Snapshot_V3_EntityEntry& InEntry)
            -> FString
        {
            switch (InEntry.Get_Provenance())
            {
                case ECk_Snapshot_V3_Provenance::EngineOwned:
                {
                    if (InEntry.Get_SaveKey().IsValid())
                    { return ck::Format_UE(TEXT("key:{}"), InEntry.Get_SaveKey().ToString(EGuidFormats::Digits)); }

                    if (NOT InEntry.Get_PlayerId().IsEmpty())
                    { return ck::Format_UE(TEXT("player:{}"), InEntry.Get_PlayerId()); }

                    return FString{TEXT("engine:?")};
                }
                case ECk_Snapshot_V3_Provenance::ConstructSpawned:
                {
                    return ck::Format_UE(TEXT("label:{}"), InEntry.Get_Label());
                }
                case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                {
                    return ck::Format_UE(TEXT("script:{}|actor:{}|label:{}"),
                        InEntry.Get_ScriptClassPath(), InEntry.Get_ActorClassPath(), InEntry.Get_Label());
                }
                case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                {
                    auto StepPaths = TArray<FString>{};
                    StepPaths.Reserve(InEntry.Get_BuildRecipe().Num());
                    for (const auto& Step : InEntry.Get_BuildRecipe())
                    { StepPaths.Add(Step.Get_ScriptClassPath()); }

                    return ck::Format_UE(TEXT("built:{}|label:{}"),
                        FString::Join(StepPaths, TEXT("+")), InEntry.Get_Label());
                }
            }
            return {};
        }

        auto
            Get_OwnerHopIndex(
                const FCk_SnapshotInspection_Document& InDocument,
                const FCk_Snapshot_V3_EntityEntry& InEntry)
            -> int32
        {
            const auto LifetimeOwnerIndex = InEntry.Get_LifetimeOwnerSavedId() == k_NoSavedEntity
                ? INDEX_NONE
                : InDocument.TryGet_EntityIndexForSavedId(InEntry.Get_LifetimeOwnerSavedId());

            if (LifetimeOwnerIndex != INDEX_NONE)
            { return LifetimeOwnerIndex; }

            // An entity that is its own context root is the NORM, not a hop — without this, every
            // transient-owned row whose context owner names itself reads as a self-cycle.
            if (InEntry.Get_ContextOwnerSavedId() == InEntry.Get_SavedId())
            { return INDEX_NONE; }

            return InEntry.Get_ContextOwnerSavedId() == k_NoSavedEntity
                ? INDEX_NONE
                : InDocument.TryGet_EntityIndexForSavedId(InEntry.Get_ContextOwnerSavedId());
        }

        // Every row's path in ONE pass with a shared memo, so a cyclic table resolves the same way no matter which row
        // is asked for first — a per-call walk would let the entry point decide where a cycle is cut.
        auto
            Get_IdentityPaths(
                const FCk_SnapshotInspection_Document& InDocument)
            -> TArray<FString>
        {
            const auto& Entities = InDocument.Get_Entities();

            auto Paths = TArray<FString>{};
            Paths.SetNum(Entities.Num());

            auto Resolved = TArray<bool>{};
            Resolved.Init(false, Entities.Num());

            auto Walk = TArray<int32>{};
            auto OnWalk = TSet<int32>{};

            for (auto Start = 0; Start < Entities.Num(); ++Start)
            {
                if (Resolved[Start])
                { continue; }

                Walk.Reset();
                OnWalk.Reset();

                auto Base = FString{k_RootMarker};
                auto Current = Start;

                while (true)
                {
                    if (Resolved[Current])
                    {
                        Base = Paths[Current];
                        break;
                    }

                    if (OnWalk.Contains(Current))
                    {
                        Base = FString{k_CycleMarker};
                        break;
                    }

                    OnWalk.Add(Current);
                    Walk.Add(Current);

                    const auto OwnerIndex = Get_OwnerHopIndex(InDocument, Entities[Current].Get_Entry());
                    if (OwnerIndex == INDEX_NONE)
                    {
                        Base = FString{k_RootMarker};
                        break;
                    }

                    Current = OwnerIndex;
                }

                for (auto Index = Walk.Num() - 1; Index >= 0; --Index)
                {
                    Base = Base + k_PathSeparator + Get_LocalIdentity(Entities[Walk[Index]].Get_Entry());
                    Paths[Walk[Index]] = Base;
                    Resolved[Walk[Index]] = true;
                }
            }

            return Paths;
        }

        // ------------------------------------------------------------------------------------------------------------

        auto
            Get_LastDotSegment(
                const FString& InPath)
            -> FString
        {
            auto Left = FString{};
            auto Right = FString{};

            if (InPath.Split(TEXT("."), &Left, &Right, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
            { return Right; }

            return InPath;
        }

        auto
            Get_DisplayName(
                const FCk_Snapshot_V3_EntityEntry& InEntry)
            -> FString
        {
            if (NOT InEntry.Get_Label().IsEmpty())
            { return InEntry.Get_Label(); }

            auto ClassPath = InEntry.Get_ActorClassPath();

            if (ClassPath.IsEmpty())
            { ClassPath = InEntry.Get_ScriptClassPath(); }

            if (ClassPath.IsEmpty())
            {
                for (const auto& Step : InEntry.Get_BuildRecipe())
                {
                    if (NOT Step.Get_ScriptClassPath().IsEmpty())
                    {
                        ClassPath = Step.Get_ScriptClassPath();
                        break;
                    }
                }
            }

            if (NOT ClassPath.IsEmpty())
            { return Get_LastDotSegment(ClassPath); }

            if (InEntry.Get_SaveKey().IsValid())
            { return InEntry.Get_SaveKey().ToString(EGuidFormats::Digits); }

            if (NOT InEntry.Get_PlayerId().IsEmpty())
            { return InEntry.Get_PlayerId(); }

            return FString{Get_ProvenanceText(InEntry.Get_Provenance())};
        }

        // ------------------------------------------------------------------------------------------------------------

        struct FCk_PayloadTypeAccumulator
        {
            FString _TypePath;
            int32   _CountBaseline = 0;
            int32   _CountCurrent = 0;
            int64   _BytesBaseline = 0;
            int64   _BytesCurrent = 0;
            TArray<FString> _HashesBaseline;
            TArray<FString> _HashesCurrent;
        };

        struct FCk_PayloadTypeTable
        {
            TArray<FCk_PayloadTypeAccumulator> _Rows;
            TMap<FString, int32>               _RowIndexByTypePath;
        };

        auto
            DoAdd_Payload(
                FCk_PayloadTypeTable& InOutTable,
                const FCk_SnapshotInspection_PayloadSummary& InPayload,
                ECk_DiffSide InSide)
            -> void
        {
            const auto& TypePath = InPayload.Get_Entry().Get_TypePath();

            auto RowIndex = int32{INDEX_NONE};
            if (const auto* Found = InOutTable._RowIndexByTypePath.Find(TypePath))
            {
                RowIndex = *Found;
            }
            else
            {
                auto NewRow = FCk_PayloadTypeAccumulator{};
                NewRow._TypePath = TypePath;

                RowIndex = InOutTable._Rows.Add(MoveTemp(NewRow));
                InOutTable._RowIndexByTypePath.Add(TypePath, RowIndex);
            }

            auto& Row = InOutTable._Rows[RowIndex];

            if (InSide == ECk_DiffSide::Baseline)
            {
                ++Row._CountBaseline;
                Row._BytesBaseline += InPayload.Get_ByteCount();
                Row._HashesBaseline.Add(InPayload.Get_PayloadHashHex());
                return;
            }

            ++Row._CountCurrent;
            Row._BytesCurrent += InPayload.Get_ByteCount();
            Row._HashesCurrent.Add(InPayload.Get_PayloadHashHex());
        }

        auto
            DoSort_Hashes(
                TArray<FString>& InOutHashes)
            -> void
        {
            InOutHashes.Sort([](const FString& InLhs, const FString& InRhs) -> bool
            {
                return InLhs.Compare(InRhs, ESearchCase::CaseSensitive) < 0;
            });
        }

        auto
            Get_HashListsDiffer(
                const TArray<FString>& InLhs,
                const TArray<FString>& InRhs)
            -> bool
        {
            if (InLhs.Num() != InRhs.Num())
            { return true; }

            for (auto Index = 0; Index < InLhs.Num(); ++Index)
            {
                if (InLhs[Index].Compare(InRhs[Index], ESearchCase::CaseSensitive) != 0)
                { return true; }
            }

            return false;
        }

        auto
            Get_PayloadTypeRows(
                FCk_PayloadTypeTable& InOutTable)
            -> TArray<FCk_SnapshotInspection_DiffPayloadTypeRow>
        {
            auto Rows = TArray<FCk_SnapshotInspection_DiffPayloadTypeRow>{};
            Rows.Reserve(InOutTable._Rows.Num());

            for (auto& Accumulator : InOutTable._Rows)
            {
                // The order payload rows appear in a table is not a property of their content, so the multiset
                // comparison only holds once both sides are ordered the same way.
                DoSort_Hashes(Accumulator._HashesBaseline);
                DoSort_Hashes(Accumulator._HashesCurrent);

                auto Row = FCk_SnapshotInspection_DiffPayloadTypeRow{};
                Row.Set_TypePath(Accumulator._TypePath)
                   .Set_CountBaseline(Accumulator._CountBaseline)
                   .Set_CountCurrent(Accumulator._CountCurrent)
                   .Set_BytesBaseline(Accumulator._BytesBaseline)
                   .Set_BytesCurrent(Accumulator._BytesCurrent)
                   .Set_ContentDiffers(Get_HashListsDiffer(Accumulator._HashesBaseline, Accumulator._HashesCurrent));

                Rows.Add(MoveTemp(Row));
            }

            Rows.Sort([](const FCk_SnapshotInspection_DiffPayloadTypeRow& InLhs,
                         const FCk_SnapshotInspection_DiffPayloadTypeRow& InRhs) -> bool
            {
                const auto LhsBytes = FMath::Abs(InLhs.Get_BytesCurrent() - InLhs.Get_BytesBaseline());
                const auto RhsBytes = FMath::Abs(InRhs.Get_BytesCurrent() - InRhs.Get_BytesBaseline());
                if (LhsBytes != RhsBytes)
                { return LhsBytes > RhsBytes; }

                const auto LhsCount = FMath::Abs(InLhs.Get_CountCurrent() - InLhs.Get_CountBaseline());
                const auto RhsCount = FMath::Abs(InRhs.Get_CountCurrent() - InRhs.Get_CountBaseline());
                if (LhsCount != RhsCount)
                { return LhsCount > RhsCount; }

                return InLhs.Get_TypePath().Compare(InRhs.Get_TypePath(), ESearchCase::CaseSensitive) < 0;
            });

            return Rows;
        }

        // ------------------------------------------------------------------------------------------------------------

        struct FCk_GroupAccumulator
        {
            FString _IdentityPath;
            FString _DisplayName;
            ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::EngineOwned;
            TArray<uint32> _SavedIdsBaseline;
            TArray<uint32> _SavedIdsCurrent;
            int64 _PayloadBytesBaseline = 0;
            int64 _PayloadBytesCurrent = 0;
            FCk_PayloadTypeTable _PayloadTypes;
        };

        struct FCk_GroupTable
        {
            TArray<FCk_GroupAccumulator> _Rows;
            TMap<FString, int32>         _RowIndexByIdentityPath;
        };

        // Every member of a group shares its path's last segment, and that segment is what the display name is derived
        // from — so the first row to open the group speaks for all of them.
        auto
            DoFindOrAdd_Group(
                FCk_GroupTable& InOutTable,
                const FString& InIdentityPath,
                const FCk_Snapshot_V3_EntityEntry& InEntry)
            -> FCk_GroupAccumulator&
        {
            if (const auto* Found = InOutTable._RowIndexByIdentityPath.Find(InIdentityPath))
            { return InOutTable._Rows[*Found]; }

            auto NewRow = FCk_GroupAccumulator{};
            NewRow._IdentityPath = InIdentityPath;
            NewRow._DisplayName = Get_DisplayName(InEntry);
            NewRow._Provenance = InEntry.Get_Provenance();

            const auto RowIndex = InOutTable._Rows.Add(MoveTemp(NewRow));
            InOutTable._RowIndexByIdentityPath.Add(InIdentityPath, RowIndex);

            return InOutTable._Rows[RowIndex];
        }

        auto
            DoAccumulate_Side(
                const FCk_SnapshotInspection_Document& InDocument,
                ECk_DiffSide InSide,
                FCk_GroupTable& InOutGroups,
                FCk_PayloadTypeTable& InOutScopePayloads)
            -> void
        {
            const auto Paths = Get_IdentityPaths(InDocument);
            const auto& Entities = InDocument.Get_Entities();

            for (auto Index = 0; Index < Entities.Num(); ++Index)
            {
                const auto& Entry = Entities[Index].Get_Entry();
                auto& Group = DoFindOrAdd_Group(InOutGroups, Paths[Index], Entry);

                if (InSide == ECk_DiffSide::Baseline)
                { Group._SavedIdsBaseline.Add(Entry.Get_SavedId()); }
                else
                { Group._SavedIdsCurrent.Add(Entry.Get_SavedId()); }
            }

            for (const auto& Payload : InDocument.Get_Payloads())
            {
                DoAdd_Payload(InOutScopePayloads, Payload, InSide);

                // A payload whose owner id names no entity row belongs to no group — it still counts at diff scope,
                // which is the only place it can be seen at all.
                const auto OwnerIndex = InDocument.TryGet_EntityIndexForSavedId(
                    Payload.Get_Entry().Get_OwnerSavedId());

                if (OwnerIndex == INDEX_NONE)
                { continue; }

                auto& Group = DoFindOrAdd_Group(InOutGroups, Paths[OwnerIndex], Entities[OwnerIndex].Get_Entry());
                DoAdd_Payload(Group._PayloadTypes, Payload, InSide);

                if (InSide == ECk_DiffSide::Baseline)
                { Group._PayloadBytesBaseline += Payload.Get_ByteCount(); }
                else
                { Group._PayloadBytesCurrent += Payload.Get_ByteCount(); }
            }
        }

        auto
            Get_GroupKind(
                const FCk_SnapshotInspection_DiffEntityGroup& InGroup)
            -> ECk_SnapshotInspection_DiffGroupKind
        {
            if (InGroup.Get_CountBaseline() == 0)
            { return ECk_SnapshotInspection_DiffGroupKind::Added; }

            if (InGroup.Get_CountCurrent() == 0)
            { return ECk_SnapshotInspection_DiffGroupKind::Removed; }

            if (InGroup.Get_CountBaseline() != InGroup.Get_CountCurrent())
            { return ECk_SnapshotInspection_DiffGroupKind::CountChanged; }

            for (const auto& Row : InGroup.Get_PayloadTypes())
            {
                if (Row.Get_CountBaseline() != Row.Get_CountCurrent()
                    || Row.Get_BytesBaseline() != Row.Get_BytesCurrent()
                    || Row.Get_ContentDiffers())
                { return ECk_SnapshotInspection_DiffGroupKind::PayloadsChanged; }
            }

            return ECk_SnapshotInspection_DiffGroupKind::Unchanged;
        }

        auto
            Get_EntityGroups(
                FCk_GroupTable& InOutTable)
            -> TArray<FCk_SnapshotInspection_DiffEntityGroup>
        {
            auto Groups = TArray<FCk_SnapshotInspection_DiffEntityGroup>{};
            Groups.Reserve(InOutTable._Rows.Num());

            for (auto& Accumulator : InOutTable._Rows)
            {
                Accumulator._SavedIdsBaseline.Sort();
                Accumulator._SavedIdsCurrent.Sort();

                auto Group = FCk_SnapshotInspection_DiffEntityGroup{};
                Group.Set_IdentityPath(Accumulator._IdentityPath)
                     .Set_DisplayName(Accumulator._DisplayName)
                     .Set_Provenance(Accumulator._Provenance)
                     .Set_CountBaseline(Accumulator._SavedIdsBaseline.Num())
                     .Set_CountCurrent(Accumulator._SavedIdsCurrent.Num())
                     .Set_PayloadBytesBaseline(Accumulator._PayloadBytesBaseline)
                     .Set_PayloadBytesCurrent(Accumulator._PayloadBytesCurrent)
                     .Set_SavedIdsBaseline(Accumulator._SavedIdsBaseline)
                     .Set_SavedIdsCurrent(Accumulator._SavedIdsCurrent)
                     .Set_PayloadTypes(Get_PayloadTypeRows(Accumulator._PayloadTypes));

                Group.Set_Kind(Get_GroupKind(Group));

                Groups.Add(MoveTemp(Group));
            }

            Groups.Sort([](const FCk_SnapshotInspection_DiffEntityGroup& InLhs,
                           const FCk_SnapshotInspection_DiffEntityGroup& InRhs) -> bool
            {
                const auto LhsCount = FMath::Abs(InLhs.Get_CountCurrent() - InLhs.Get_CountBaseline());
                const auto RhsCount = FMath::Abs(InRhs.Get_CountCurrent() - InRhs.Get_CountBaseline());
                if (LhsCount != RhsCount)
                { return LhsCount > RhsCount; }

                const auto LhsBytes = FMath::Abs(InLhs.Get_PayloadBytesCurrent() - InLhs.Get_PayloadBytesBaseline());
                const auto RhsBytes = FMath::Abs(InRhs.Get_PayloadBytesCurrent() - InRhs.Get_PayloadBytesBaseline());
                if (LhsBytes != RhsBytes)
                { return LhsBytes > RhsBytes; }

                return InLhs.Get_IdentityPath().Compare(InRhs.Get_IdentityPath(), ESearchCase::CaseSensitive) < 0;
            });

            return Groups;
        }

        // ------------------------------------------------------------------------------------------------------------

        auto
            Make_CensusRow(
                ECk_Snapshot_V3_Provenance InProvenance,
                int32 InCountBaseline,
                int32 InCountCurrent)
            -> FCk_SnapshotInspection_DiffCensusRow
        {
            auto Row = FCk_SnapshotInspection_DiffCensusRow{};
            Row.Set_Provenance(InProvenance)
               .Set_CountBaseline(InCountBaseline)
               .Set_CountCurrent(InCountCurrent);
            return Row;
        }

        auto
            Get_CensusRows(
                const FCk_SnapshotInspection_Document& InBaseline,
                const FCk_SnapshotInspection_Document& InCurrent)
            -> TArray<FCk_SnapshotInspection_DiffCensusRow>
        {
            const auto& BaselineCensus = InBaseline.Get_Census();
            const auto& CurrentCensus = InCurrent.Get_Census();

            auto Rows = TArray<FCk_SnapshotInspection_DiffCensusRow>{};
            Rows.Add(Make_CensusRow(ECk_Snapshot_V3_Provenance::EngineOwned,
                BaselineCensus.Get_EngineOwnedCount(), CurrentCensus.Get_EngineOwnedCount()));
            Rows.Add(Make_CensusRow(ECk_Snapshot_V3_Provenance::ConstructSpawned,
                BaselineCensus.Get_ConstructSpawnedCount(), CurrentCensus.Get_ConstructSpawnedCount()));
            Rows.Add(Make_CensusRow(ECk_Snapshot_V3_Provenance::RuntimeSpawned,
                BaselineCensus.Get_RuntimeSpawnedCount(), CurrentCensus.Get_RuntimeSpawnedCount()));
            Rows.Add(Make_CensusRow(ECk_Snapshot_V3_Provenance::DefinitionBuilt,
                BaselineCensus.Get_DefinitionBuiltCount(), CurrentCensus.Get_DefinitionBuiltCount()));

            return Rows;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Diff_Documents(
            const FCk_SnapshotInspection_Document& InBaseline,
            const FCk_SnapshotInspection_Document& InCurrent)
        -> FCk_SnapshotInspection_Diff
    {
        auto Diff = FCk_SnapshotInspection_Diff{};

        const auto BaselineReadable = InBaseline.Get_ReadStatus() == ECk_SnapshotInspection_ReadStatus::Success;
        const auto CurrentReadable = InCurrent.Get_ReadStatus() == ECk_SnapshotInspection_ReadStatus::Success;

        if (NOT BaselineReadable || NOT CurrentReadable)
        {
            auto Reasons = TArray<FString>{};

            if (NOT BaselineReadable)
            {
                Reasons.Add(ck::Format_UE(TEXT("the baseline read status is [{}]"),
                    Get_ReadStatusText(InBaseline.Get_ReadStatus())));
            }

            if (NOT CurrentReadable)
            {
                Reasons.Add(ck::Format_UE(TEXT("the current read status is [{}]"),
                    Get_ReadStatusText(InCurrent.Get_ReadStatus())));
            }

            Diff.Set_InvalidReason(ck::Format_UE(TEXT("A diff needs two successful reads: {}"),
                FString::Join(Reasons, TEXT(" and "))));

            return Diff;
        }

        Diff.Set_Valid(true)
            .Set_EntityCountBaseline(InBaseline.Get_Entities().Num())
            .Set_EntityCountCurrent(InCurrent.Get_Entities().Num())
            .Set_PayloadCountBaseline(InBaseline.Get_Payloads().Num())
            .Set_PayloadCountCurrent(InCurrent.Get_Payloads().Num())
            .Set_SnapshotBytesBaseline(InBaseline.Get_SnapshotByteCount())
            .Set_SnapshotBytesCurrent(InCurrent.Get_SnapshotByteCount())
            .Set_Census(ck_snapshot_inspection_diff::Get_CensusRows(InBaseline, InCurrent));

        auto Groups = ck_snapshot_inspection_diff::FCk_GroupTable{};
        auto ScopePayloads = ck_snapshot_inspection_diff::FCk_PayloadTypeTable{};

        ck_snapshot_inspection_diff::DoAccumulate_Side(InBaseline,
            ck_snapshot_inspection_diff::ECk_DiffSide::Baseline, Groups, ScopePayloads);
        ck_snapshot_inspection_diff::DoAccumulate_Side(InCurrent,
            ck_snapshot_inspection_diff::ECk_DiffSide::Current, Groups, ScopePayloads);

        Diff.Set_PayloadTypes(ck_snapshot_inspection_diff::Get_PayloadTypeRows(ScopePayloads))
            .Set_EntityGroups(ck_snapshot_inspection_diff::Get_EntityGroups(Groups));

        return Diff;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IdentityPathForEntity(
            const FCk_SnapshotInspection_Document& InDocument,
            int32 InEntityIndex)
        -> FString
    {
        if (NOT InDocument.Get_Entities().IsValidIndex(InEntityIndex))
        { return {}; }

        return ck_snapshot_inspection_diff::Get_IdentityPaths(InDocument)[InEntityIndex];
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_DiffGroupKindText(
            ECk_SnapshotInspection_DiffGroupKind InKind)
        -> FString
    {
        switch (InKind)
        {
            case ECk_SnapshotInspection_DiffGroupKind::Added:           return FString{TEXT("Added")};
            case ECk_SnapshotInspection_DiffGroupKind::Removed:         return FString{TEXT("Removed")};
            case ECk_SnapshotInspection_DiffGroupKind::CountChanged:    return FString{TEXT("CountChanged")};
            case ECk_SnapshotInspection_DiffGroupKind::PayloadsChanged: return FString{TEXT("PayloadsChanged")};
            case ECk_SnapshotInspection_DiffGroupKind::Unchanged:       return FString{TEXT("Unchanged")};
        }
        return FString{TEXT("Unknown")};
    }
}

// --------------------------------------------------------------------------------------------------------------------
