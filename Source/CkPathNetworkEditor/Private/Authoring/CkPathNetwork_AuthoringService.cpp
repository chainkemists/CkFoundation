#include "CkPathNetworkEditor/Authoring/CkPathNetwork_AuthoringService.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"
#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"
#include "CkPathNetwork/Network/CkPathNetwork_Vectorize.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Editor.h>
#include <Editor/Transactor.h>
#include <Engine/Level.h>
#include <Engine/Selection.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <Misc/Crc.h>
#include <ScopedTransaction.h>
#include <UObject/Class.h>
#include <UObject/ObjectKey.h>
#include <UObject/Package.h>
#include <UObject/UObjectHash.h>
#include <UObject/UObjectGlobals.h>
#include <UObject/UObjectIterator.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::authoring::detail
{
    struct FPreviewInvariantSnapshot
    {
        int32 _ActorCount = 0;
        bool _WorldPackageWasDirty = false;
        TMap<TWeakObjectPtr<UPackage>, bool> _LevelPackageDirtyStates;
        TSet<FObjectKey> _SelectedActors;
        int32 _TransactionQueueLength = INDEX_NONE;
        int32 _TransactionUndoCount = INDEX_NONE;
        bool _TransactionWasActive = false;
    };

    auto
    Get_AreBoundsValid(
        const FBox& InBounds) -> bool
    {
        return InBounds.IsValid
            && NOT InBounds.Min.ContainsNaN()
            && NOT InBounds.Max.ContainsNaN()
            && InBounds.Min.X < InBounds.Max.X
            && InBounds.Min.Y < InBounds.Max.Y
            && InBounds.Min.Z < InBounds.Max.Z;
    }

    auto
    Get_AreVectorizeParamsValid(
        const FCk_PathNetwork_VectorizeParams& InParams) -> bool
    {
        return FMath::IsFinite(InParams.Get_MinHalfWidth())
            && FMath::IsFinite(InParams.Get_SimplifyTolerance())
            && FMath::IsFinite(InParams.Get_MinRibbonLength())
            && InParams.Get_MinHalfWidth() > 0.0f
            && InParams.Get_SimplifyTolerance() >= 0.0f
            && InParams.Get_MinRibbonLength() >= 0.0f;
    }

    auto
    Get_AreBuildParamsValid(
        const FCk_PathNetwork_BuildParams& InParams) -> bool
    {
        return FMath::IsFinite(InParams.Get_NodeSnapRadius())
            && FMath::IsFinite(InParams.Get_ChunkSize())
            && InParams.Get_NodeSnapRadius() > 0.0f
            && InParams.Get_ChunkSize() >= 100.0f;
    }

    auto
    Get_AreVectorizeParamsEqual(
        const FCk_PathNetwork_VectorizeParams& InLhs,
        const FCk_PathNetwork_VectorizeParams& InRhs) -> bool
    {
        return InLhs.Get_MinHalfWidth() == InRhs.Get_MinHalfWidth()
            && InLhs.Get_SimplifyTolerance() == InRhs.Get_SimplifyTolerance()
            && InLhs.Get_MinRibbonLength() == InRhs.Get_MinRibbonLength();
    }

    auto
    Get_AreBoundsEqual(
        const FBox& InLhs,
        const FBox& InRhs) -> bool
    {
        return InLhs.Min.Equals(InRhs.Min)
            && InLhs.Max.Equals(InRhs.Max);
    }

    auto
    Capture_PreviewInvariants(
        UWorld& InWorld) -> FPreviewInvariantSnapshot
    {
        auto Snapshot = FPreviewInvariantSnapshot{};

        for (TActorIterator<AActor> It{&InWorld}; It; ++It)
        { ++Snapshot._ActorCount; }

        Snapshot._WorldPackageWasDirty = InWorld.GetPackage()->IsDirty();
        for (auto* Level : InWorld.GetLevels())
        {
            if (ck::Is_NOT_Valid(Level, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }
            auto* Package = Level->GetPackage();
            Snapshot._LevelPackageDirtyStates.Add(Package, Package->IsDirty());
        }

        if (ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto* Selection = GEditor->GetSelectedActors())
            {
                for (FSelectionIterator It{*Selection}; It; ++It)
                {
                    if (const auto* Actor = Cast<AActor>(*It))
                    { Snapshot._SelectedActors.Add(FObjectKey{Actor}); }
                }
            }

            if (auto* Transactions = GEditor->Trans.Get();
                ck::IsValid(Transactions, ck::IsValid_Policy_NullptrOnly{}))
            {
                Snapshot._TransactionQueueLength = Transactions->GetQueueLength();
                Snapshot._TransactionUndoCount = Transactions->GetUndoCount();
                Snapshot._TransactionWasActive = Transactions->IsActive();
            }
        }

        return Snapshot;
    }

    auto
    Get_PreviewInvariantFailure(
        UWorld& InWorld,
        const FPreviewInvariantSnapshot& InBefore) -> FString
    {
        auto ActorCount = 0;
        for (TActorIterator<AActor> It{&InWorld}; It; ++It)
        { ++ActorCount; }
        if (ActorCount != InBefore._ActorCount)
        { return TEXT("Detector violated preview purity by changing the editor-world actor count"); }

        if (InWorld.GetPackage()->IsDirty() != InBefore._WorldPackageWasDirty)
        { return TEXT("Detector violated preview purity by changing the editor-world package dirty state"); }

        for (const auto& Kvp : InBefore._LevelPackageDirtyStates)
        {
            const auto* Package = Kvp.Key.Get();
            if (ck::Is_NOT_Valid(Package, ck::IsValid_Policy_NullptrOnly{}))
            { return TEXT("Detector violated preview purity by invalidating a level package"); }
            if (Package->IsDirty() != Kvp.Value)
            { return TEXT("Detector violated preview purity by changing a level package dirty state"); }
        }

        if (ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SelectedActors = TSet<FObjectKey>{};
            if (auto* Selection = GEditor->GetSelectedActors())
            {
                for (FSelectionIterator It{*Selection}; It; ++It)
                {
                    if (const auto* Actor = Cast<AActor>(*It))
                    { SelectedActors.Add(FObjectKey{Actor}); }
                }
            }
            if (SelectedActors.Num() != InBefore._SelectedActors.Num())
            { return TEXT("Detector violated preview purity by changing editor actor selection"); }
            for (const auto& SelectedActor : SelectedActors)
            {
                if (NOT InBefore._SelectedActors.Contains(SelectedActor))
                { return TEXT("Detector violated preview purity by changing editor actor selection"); }
            }

            if (auto* Transactions = GEditor->Trans.Get();
                ck::IsValid(Transactions, ck::IsValid_Policy_NullptrOnly{}))
            {
                if (Transactions->GetQueueLength() != InBefore._TransactionQueueLength
                    || Transactions->GetUndoCount() != InBefore._TransactionUndoCount
                    || Transactions->IsActive() != InBefore._TransactionWasActive)
                { return TEXT("Detector violated preview purity by changing editor transaction state"); }
            }
        }

        return {};
    }

    auto
    Duplicate_TransientDetector(
        const UCk_PathNetwork_Detector_UE& InTemplate,
        UWorld& InWorld) -> UCk_PathNetwork_Detector_UE*
    {
        auto Params = FObjectDuplicationParameters{
            const_cast<UCk_PathNetwork_Detector_UE*>(&InTemplate), &InWorld};
        Params.FlagMask = RF_NoFlags;
        Params.ApplyFlags = RF_Transient;
        Params.bAssignExternalPackages = false;

        return Cast<UCk_PathNetwork_Detector_UE>(StaticDuplicateObjectEx(Params));
    }

    auto
    Duplicate_AppliedDetector(
        const UCk_PathNetwork_Detector_UE& InTemplate,
        ACk_PathNetwork_UE& InActor) -> UCk_PathNetwork_Detector_UE*
    {
        auto Params = FObjectDuplicationParameters{
            const_cast<UCk_PathNetwork_Detector_UE*>(&InTemplate), &InActor};
        Params.FlagMask = RF_NoFlags;
        Params.ApplyFlags = RF_Transactional;
        Params.bAssignExternalPackages = false;

        return Cast<UCk_PathNetwork_Detector_UE>(StaticDuplicateObjectEx(Params));
    }

    auto
    Build_RelativeRibbons(
        ACk_PathNetwork_UE& InActor,
        const TArray<FCk_PathNetwork_Ribbon>& InAuthoredWorldRibbons,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons,
        FApplyResult& InOutResult) -> TArray<FCk_PathNetwork_Ribbon>
    {
        auto RelativeRibbons = TArray<FCk_PathNetwork_Ribbon>{};
        RelativeRibbons.Reserve(InAuthoredWorldRibbons.Num() + InGeneratedWorldRibbons.Num());

        for (const auto& Ribbon : InAuthoredWorldRibbons)
        {
            RelativeRibbons.Add(InActor.Convert_WorldRibbonToRelative(Ribbon));
            ++InOutResult._AuthoredRibbonCount;
        }
        for (const auto& Ribbon : InGeneratedWorldRibbons)
        { RelativeRibbons.Add(InActor.Convert_WorldRibbonToRelative(Ribbon)); }

        InOutResult._GeneratedRibbonCount = InGeneratedWorldRibbons.Num();
        InOutResult._TotalRibbonCount = RelativeRibbons.Num();
        return RelativeRibbons;
    }

    auto
    Collect_AuthoredWorldRibbons(
        const ACk_PathNetwork_UE& InActor) -> TArray<FCk_PathNetwork_Ribbon>
    {
        auto Authored = TArray<FCk_PathNetwork_Ribbon>{};
        for (const auto& Ribbon : InActor.Get_WorldRibbons())
        {
            if (Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored)
            { Authored.Add(Ribbon); }
        }
        return Authored;
    }

    auto
    Get_AreGeneratedRibbonsEquivalent(
        const TArray<FCk_PathNetwork_Ribbon>& InLhs,
        const TArray<FCk_PathNetwork_Ribbon>& InRhs) -> bool
    {
        if (InLhs.Num() != InRhs.Num())
        { return false; }

        for (auto RibbonIndex = 0; RibbonIndex < InLhs.Num(); ++RibbonIndex)
        {
            const auto& LhsRibbon = InLhs[RibbonIndex];
            const auto& RhsRibbon = InRhs[RibbonIndex];
            if (LhsRibbon.Get_Source() != RhsRibbon.Get_Source()
                || LhsRibbon.Get_Points().Num() != RhsRibbon.Get_Points().Num())
            { return false; }

            for (auto PointIndex = 0; PointIndex < LhsRibbon.Get_Points().Num(); ++PointIndex)
            {
                const auto& LhsPoint = LhsRibbon.Get_Points()[PointIndex];
                const auto& RhsPoint = RhsRibbon.Get_Points()[PointIndex];
                if (NOT LhsPoint.Get_Location().Equals(RhsPoint.Get_Location())
                    || NOT FMath::IsNearlyEqual(
                        LhsPoint.Get_HalfWidth(), RhsPoint.Get_HalfWidth()))
                { return false; }
            }
        }
        return true;
    }

    auto
    Revalidate_PreviewOutput(
        UWorld& InWorld,
        const UCk_PathNetwork_Detector_UE& InDetectorTemplate,
        const ACk_PathNetwork_UE* InSourceActor,
        const FBox& InDetectionBounds,
        const FCk_PathNetwork_VectorizeParams& InVectorizeParams,
        const FPreviewResult& InPreview,
        FString& OutFailureReason) -> bool
    {
        const auto FreshPreview = Preview(
            FPreviewRequest{
                ._World = &InWorld,
                ._DetectorTemplate = &InDetectorTemplate,
                ._SourceActor = InSourceActor,
                ._DetectionBounds = InDetectionBounds,
                ._VectorizeParams = InVectorizeParams});
        if (NOT FreshPreview._Succeeded)
        {
            OutFailureReason = FString::Printf(
                TEXT("Source data could not be revalidated: %s"),
                *FreshPreview._FailureReason);
            return false;
        }

        if (NOT Get_AreGeneratedRibbonsEquivalent(
                FreshPreview._GeneratedWorldRibbons,
                InPreview._GeneratedWorldRibbons))
        {
            OutFailureReason =
                TEXT("Source geometry changed after Preview. Run Preview again before Apply.");
            return false;
        }
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::authoring
{
    auto
    Is_UsableDetectorClass(
        const UClass* InClass) -> bool
    {
        return ck::IsValid(InClass, ck::IsValid_Policy_NullptrOnly{})
            && InClass->IsChildOf(UCk_PathNetwork_Detector_UE::StaticClass())
            && NOT InClass->HasAnyClassFlags(
                CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists
                | CLASS_Hidden | CLASS_HideDropDown);
    }

    auto
    Get_LoadedUsableDetectorClasses() -> TArray<UClass*>
    {
        auto Classes = TArray<UClass*>{};
        GetDerivedClasses(UCk_PathNetwork_Detector_UE::StaticClass(), Classes, true);
        Classes.RemoveAll([](const UClass* InClass)
        {
            return NOT Is_UsableDetectorClass(InClass);
        });
        Classes.Sort([](const UClass& InLhs, const UClass& InRhs)
        {
            return InLhs.GetPathName() < InRhs.GetPathName();
        });
        return Classes;
    }

    auto
    Compute_DetectorConfigurationFingerprint(
        const UCk_PathNetwork_Detector_UE* InDetector) -> uint32
    {
        if (NOT Is_UsableDetectorClass(ck::IsValid(InDetector) ? InDetector->GetClass() : nullptr))
        { return 0; }

        auto Text = InDetector->GetClass()->GetPathName();
        for (TFieldIterator<FProperty> It{
                InDetector->GetClass(), EFieldIterationFlags::IncludeSuper};
             It;
             ++It)
        {
            const auto* Property = *It;
            if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_SkipSerialization))
            { continue; }

            auto Value = FString{};
            Property->ExportText_InContainer(
                0, Value, InDetector, nullptr,
                const_cast<UCk_PathNetwork_Detector_UE*>(InDetector), PPF_None);
            Text += TEXT("|");
            Text += Property->GetName();
            Text += TEXT("=");
            Text += Value;
        }
        return FCrc::StrCrc32(*Text);
    }

    auto
    Preview(
        const FPreviewRequest& InRequest) -> FPreviewResult
    {
        auto Result = FPreviewResult{};

        const bool WorldIsUsable =
            ck::IsValid(InRequest._World)
            && InRequest._World->WorldType == EWorldType::Editor
            && NOT InRequest._World->IsGameWorld()
            && NOT InRequest._World->bIsTearingDown;
        CK_ENSURE_IF_NOT(WorldIsUsable,
            TEXT("Path-network preview requires a non-PIE editor world"))
        {}
        if (NOT WorldIsUsable)
        {
            Result._FailureReason = TEXT("Editor world is invalid, tearing down, or running PIE");
            return Result;
        }

        const bool DetectorIsValid = ck::IsValid(InRequest._DetectorTemplate)
            && Is_UsableDetectorClass(InRequest._DetectorTemplate->GetClass());
        CK_ENSURE_IF_NOT(DetectorIsValid,
            TEXT("Path-network preview requires a valid concrete detector"))
        {}
        if (NOT DetectorIsValid)
        {
            Result._FailureReason = TEXT("Detector is invalid, abstract, or deprecated");
            return Result;
        }

        const bool BoundsAreValid = detail::Get_AreBoundsValid(InRequest._DetectionBounds);
        CK_ENSURE_IF_NOT(BoundsAreValid,
            TEXT("Path-network preview requires finite ordered detection bounds"))
        {}
        if (NOT BoundsAreValid)
        {
            Result._FailureReason = TEXT("Detection bounds are invalid");
            return Result;
        }

        const bool VectorizeParamsAreValid =
            detail::Get_AreVectorizeParamsValid(InRequest._VectorizeParams);
        CK_ENSURE_IF_NOT(VectorizeParamsAreValid,
            TEXT("Path-network preview requires valid vectorize parameters"))
        {}
        if (NOT VectorizeParamsAreValid)
        {
            Result._FailureReason = TEXT("Vectorize parameters are invalid");
            return Result;
        }

        const bool SourceActorMatches =
            InRequest._SourceActor == nullptr
            || (ck::IsValid(InRequest._SourceActor)
                && InRequest._SourceActor->GetWorld() == InRequest._World
                && InRequest._DetectorTemplate == InRequest._SourceActor->Get_Detector());
        CK_ENSURE_IF_NOT(SourceActorMatches,
            TEXT("Path-network preview source actor must own the supplied detector in the supplied world"))
        {}
        if (NOT SourceActorMatches)
        {
            Result._FailureReason = TEXT("Source actor does not own the supplied detector in this world");
            return Result;
        }

        const auto Before = detail::Capture_PreviewInvariants(*InRequest._World);
        if (Before._TransactionWasActive)
        {
            Result._FailureReason = TEXT("Finish the active editor transaction before previewing");
            return Result;
        }

        const auto TemplateFingerprint =
            Compute_DetectorConfigurationFingerprint(InRequest._DetectorTemplate);
        auto* Detector = detail::Duplicate_TransientDetector(
            *InRequest._DetectorTemplate, *InRequest._World);
        const bool PreviewDetectorIsValid = ck::IsValid(Detector);
        CK_ENSURE_IF_NOT(PreviewDetectorIsValid,
            TEXT("Path-network preview could not duplicate detector [{}]"),
            InRequest._DetectorTemplate)
        {}
        if (NOT PreviewDetectorIsValid)
        {
            Result._FailureReason = TEXT("Could not create transient preview detector");
            return Result;
        }

        Result._World = InRequest._World;
        Result._DetectionBounds = InRequest._DetectionBounds;
        Result._VectorizeParams = InRequest._VectorizeParams;
        Result._DetectorClass = InRequest._DetectorTemplate->GetClass();
        Result._DetectorConfigurationFingerprint = TemplateFingerprint;

        const auto BoundsValidation =
            Detector->Validate_DetectionBounds(InRequest._DetectionBounds);
        if (NOT BoundsValidation.Get_Succeeded())
        { Result._FailureReason = BoundsValidation.Get_FailureReason(); }
        else
        {
            Result._Mask = Detector->Get_DetectionMask(InRequest._DetectionBounds);
            if (Result._Mask.Get_IsValidMask())
            {
                for (const auto Cell : Result._Mask.Get_Occupancy())
                { Result._OccupiedCellCount += Cell != 0 ? 1 : 0; }

                auto Vectorized =
                    ck::pathnetwork::Try_VectorizeDetectorMaskToRibbons(
                        *Detector,
                        InRequest._DetectionBounds,
                        Result._Mask,
                        InRequest._VectorizeParams);
                if (NOT Vectorized._Succeeded)
                { Result._FailureReason = Vectorized._FailureReason; }
                else
                {
                    Result._GeneratedWorldRibbons =
                        MoveTemp(Vectorized._Ribbons);
                }
            }

            if (Result._FailureReason.IsEmpty())
            {
                auto Processed = Detector->Process_GeneratedRibbons(
                    InRequest._DetectionBounds,
                    Result._GeneratedWorldRibbons);
                Result._UnsupportedSegmentCount =
                    Processed.Get_UnsupportedSegmentCount();
                if (NOT Processed.Get_Succeeded())
                { Result._FailureReason = Processed.Get_FailureReason(); }
                else
                {
                    Result._GeneratedWorldRibbons =
                        Processed.Get_GeneratedWorldRibbons();

                    if (NOT ck::pathnetwork::Get_AreAllRibbonSourcesGenerated(
                        Result._GeneratedWorldRibbons))
                    {
                        Result._FailureReason =
                            TEXT("Detector processing output contains a non-Generated ribbon");
                    }
                    else
                    {
                        const auto Validation = Detector->Validate_GeneratedRibbons(
                            InRequest._DetectionBounds,
                            Result._GeneratedWorldRibbons);
                        if (NOT Validation.Get_Succeeded())
                        { Result._FailureReason = Validation.Get_FailureReason(); }
                    }
                }
            }
        }

        const auto InvariantFailure =
            detail::Get_PreviewInvariantFailure(*InRequest._World, Before);
        if (NOT InvariantFailure.IsEmpty())
        {
            Result._FailureReason = InvariantFailure;
            return Result;
        }

        const bool TemplateIsUnchanged =
            TemplateFingerprint
            == Compute_DetectorConfigurationFingerprint(InRequest._DetectorTemplate);
        CK_ENSURE_IF_NOT(TemplateIsUnchanged,
            TEXT("Detector violated preview purity by changing its source template"))
        {}
        if (NOT TemplateIsUnchanged)
        {
            Result._FailureReason =
                TEXT("Detector violated preview purity by changing its source template");
            return Result;
        }
        if (NOT Result._FailureReason.IsEmpty())
        { return Result; }

        Result._Succeeded = true;
        return Result;
    }

    auto
    ApplyPreview_ToExistingActor(
        ACk_PathNetwork_UE* InActor,
        const FPreviewResult& InPreview) -> FApplyResult
    {
        auto Result = FApplyResult{};

        const bool ActorIsValid = ck::IsValid(InActor);
        CK_ENSURE_IF_NOT(ActorIsValid,
            TEXT("ApplyPreview_ToExistingActor requires a valid actor"))
        {}
        if (NOT ActorIsValid)
        {
            Result._FailureReason = TEXT("Path-network actor is invalid");
            return Result;
        }

        const auto* ActorDetector = InActor->Get_Detector().Get();
        const bool ActorDetectorIsValid = ck::IsValid(ActorDetector);
        const bool PreviewIsCurrent =
            InPreview._Succeeded
            && ActorDetectorIsValid
            && InPreview._World.Get() == InActor->GetWorld()
            && detail::Get_AreBoundsEqual(InPreview._DetectionBounds, InActor->Get_DetectionBounds())
            && detail::Get_AreVectorizeParamsEqual(
                InPreview._VectorizeParams, InActor->Get_VectorizeParams())
            && InPreview._DetectorClass.Get() == ActorDetector->GetClass()
            && InPreview._DetectorConfigurationFingerprint
                == Compute_DetectorConfigurationFingerprint(ActorDetector);
        CK_ENSURE_IF_NOT(PreviewIsCurrent,
            TEXT("ApplyPreview_ToExistingActor on [{}] rejected a failed or stale preview"), InActor)
        {}
        if (NOT PreviewIsCurrent)
        {
            Result._FailureReason = TEXT("Preview is failed, stale, or from different actor settings");
            return Result;
        }

        const bool PreviewOutputIsCurrent = detail::Revalidate_PreviewOutput(
            *InActor->GetWorld(),
            *ActorDetector,
            InActor,
            InActor->Get_DetectionBounds(),
            InActor->Get_VectorizeParams(),
            InPreview,
            Result._FailureReason);
        CK_ENSURE_IF_NOT(PreviewOutputIsCurrent,
            TEXT("ApplyPreview_ToExistingActor on [{}] detected changed source output"), InActor)
        {}
        if (NOT PreviewOutputIsCurrent)
        { return Result; }

        const auto AuthoredWorldRibbons = detail::Collect_AuthoredWorldRibbons(*InActor);
        const FScopedTransaction Transaction{
            NSLOCTEXT("CkPathNetworkEditor", "ApplyDetectorPreviewTransaction",
                "Path Network: Apply Detector Preview")};
        InActor->Modify();
        InActor->Set_Ribbons(detail::Build_RelativeRibbons(
            *InActor, AuthoredWorldRibbons, InPreview._GeneratedWorldRibbons, Result));
        InActor->PostEditChange();

        Result._Actor = InActor;
        Result._Succeeded = true;
        return Result;
    }

    auto
    ApplyPreview_ToLevel(
        const FApplyToLevelRequest& InRequest) -> FApplyResult
    {
        auto Result = FApplyResult{};

        const bool PreviewIsValid =
            InRequest._Preview != nullptr
            && InRequest._Preview->_Succeeded;
        CK_ENSURE_IF_NOT(PreviewIsValid,
            TEXT("ApplyPreview_ToLevel requires a successful preview"))
        {}
        if (NOT PreviewIsValid)
        {
            Result._FailureReason = TEXT("A successful preview is required");
            return Result;
        }

        const bool TargetLevelIsValid = ck::IsValid(InRequest._TargetLevel);
        CK_ENSURE_IF_NOT(TargetLevelIsValid,
            TEXT("ApplyPreview_ToLevel requires a valid target level"))
        {}
        if (NOT TargetLevelIsValid)
        {
            Result._FailureReason = TEXT("Target level is invalid");
            return Result;
        }

        auto* World = InRequest._TargetLevel->GetWorld();
        const bool WorldIsUsable =
            ck::IsValid(World)
            && World->WorldType == EWorldType::Editor
            && NOT World->IsGameWorld()
            && NOT World->bIsTearingDown
            && World->GetLevels().Contains(InRequest._TargetLevel);
        CK_ENSURE_IF_NOT(WorldIsUsable,
            TEXT("ApplyPreview_ToLevel requires a loaded level in a non-PIE editor world"))
        {}
        if (NOT WorldIsUsable)
        {
            Result._FailureReason = TEXT("Target level is not loaded in a usable editor world");
            return Result;
        }

        const bool DetectorIsValid =
            ck::IsValid(InRequest._DetectorTemplate)
            && Is_UsableDetectorClass(InRequest._DetectorTemplate->GetClass());
        CK_ENSURE_IF_NOT(DetectorIsValid,
            TEXT("ApplyPreview_ToLevel requires a valid concrete detector template"))
        {}
        if (NOT DetectorIsValid)
        {
            Result._FailureReason = TEXT("Detector template is invalid, abstract, or deprecated");
            return Result;
        }

        const bool ConfigurationIsValid =
            detail::Get_AreBoundsValid(InRequest._DetectionBounds)
            && detail::Get_AreVectorizeParamsValid(InRequest._VectorizeParams)
            && detail::Get_AreBuildParamsValid(InRequest._BuildParams)
            && (InRequest._UseRecommendedFollowerTuning != ECk_EnableDisable::Enable
                || UCk_Utils_PathNetworkFollower_UE::Get_IsTuningValid(
                    InRequest._RecommendedFollowerTuning));
        CK_ENSURE_IF_NOT(ConfigurationIsValid,
            TEXT("ApplyPreview_ToLevel requires valid bounds, vectorize parameters, build parameters, and route preferences"))
        {}
        if (NOT ConfigurationIsValid)
        {
            Result._FailureReason = TEXT("Authoring configuration is invalid");
            return Result;
        }

        const auto& Preview = *InRequest._Preview;
        const bool PreviewIsCurrent =
            Preview._World.Get() == World
            && Preview._DetectorClass.Get() == InRequest._DetectorTemplate->GetClass()
            && Preview._DetectorConfigurationFingerprint
                == Compute_DetectorConfigurationFingerprint(InRequest._DetectorTemplate)
            && detail::Get_AreBoundsEqual(Preview._DetectionBounds, InRequest._DetectionBounds)
            && detail::Get_AreVectorizeParamsEqual(
                Preview._VectorizeParams, InRequest._VectorizeParams);
        CK_ENSURE_IF_NOT(PreviewIsCurrent,
            TEXT("ApplyPreview_ToLevel rejected a stale preview"))
        {}
        if (NOT PreviewIsCurrent)
        {
            Result._FailureReason = TEXT("Preview is stale or belongs to different settings/world");
            return Result;
        }

        const bool PreviewOutputIsCurrent = detail::Revalidate_PreviewOutput(
            *World,
            *InRequest._DetectorTemplate,
            nullptr,
            InRequest._DetectionBounds,
            InRequest._VectorizeParams,
            Preview,
            Result._FailureReason);
        CK_ENSURE_IF_NOT(PreviewOutputIsCurrent,
            TEXT("ApplyPreview_ToLevel detected changed source output"))
        {}
        if (NOT PreviewOutputIsCurrent)
        { return Result; }

        auto* TargetActor = InRequest._ExplicitTargetActor;
        if (InRequest._ExplicitTargetActor != nullptr
            && NOT ck::IsValid(InRequest._ExplicitTargetActor))
        {
            Result._FailureReason = TEXT("Explicit target actor is invalid");
            return Result;
        }
        if (ck::IsValid(TargetActor))
        {
            if (TargetActor->GetWorld() != World || TargetActor->GetLevel() != InRequest._TargetLevel)
            {
                Result._FailureReason =
                    TEXT("Explicit target actor does not belong to the target level");
                return Result;
            }
        }
        else
        {
            auto Candidates = TArray<ACk_PathNetwork_UE*>{};
            for (const auto& Actor : InRequest._TargetLevel->Actors)
            {
                if (auto* Candidate = Cast<ACk_PathNetwork_UE>(Actor.Get());
                    ck::IsValid(Candidate))
                { Candidates.Add(Candidate); }
            }
            if (Candidates.Num() > 1)
            {
                Result._FailureReason =
                    TEXT("Target level contains multiple path-network actors; select one explicitly");
                return Result;
            }
            if (Candidates.Num() == 1)
            { TargetActor = Candidates[0]; }
        }

        auto AuthoredWorldRibbons = TArray<FCk_PathNetwork_Ribbon>{};
        auto RepresentativeWorldRoutes =
            TArray<FCk_PathNetwork_RepresentativeRoute>{};
        if (ck::IsValid(TargetActor))
        {
            AuthoredWorldRibbons =
                detail::Collect_AuthoredWorldRibbons(*TargetActor);
            RepresentativeWorldRoutes =
                TargetActor->Get_WorldRepresentativeRoutes();
        }

        FScopedTransaction Transaction{
            NSLOCTEXT("CkPathNetworkEditor", "ApplyPreviewToLevelTransaction",
                "Path Network: Apply Preview To Level")};

        if (NOT ck::IsValid(TargetActor))
        {
            auto SpawnParams = FActorSpawnParameters{};
            SpawnParams.OverrideLevel = InRequest._TargetLevel;
            SpawnParams.ObjectFlags |= RF_Transactional;
            SpawnParams.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            TargetActor = World->SpawnActor<ACk_PathNetwork_UE>(
                ACk_PathNetwork_UE::StaticClass(),
                FTransform{InRequest._DetectionBounds.GetCenter()},
                SpawnParams);
            if (NOT ck::IsValid(TargetActor))
            {
                Transaction.Cancel();
                Result._FailureReason = TEXT("Could not create path-network actor in target level");
                return Result;
            }
            Result._CreatedActor = true;
        }

        auto* OwnedDetector = detail::Duplicate_AppliedDetector(
            *InRequest._DetectorTemplate, *TargetActor);
        if (NOT ck::IsValid(OwnedDetector))
        {
            if (Result._CreatedActor)
            { World->DestroyActor(TargetActor, true, true); }
            Transaction.Cancel();
            Result._FailureReason = TEXT("Could not create actor-owned detector instance");
            return Result;
        }

        TargetActor->Modify();
        const bool ConfigurationWasApplied =
            TargetActor->Set_EditorAuthoringConfiguration(
            InRequest._BuildParams,
            InRequest._VectorizeParams,
            OwnedDetector,
            InRequest._DetectionBounds.GetExtent(),
            InRequest._AutoDetectOnBeginPlay,
            InRequest._UseRecommendedFollowerTuning,
            InRequest._RecommendedFollowerTuning);
        if (NOT ConfigurationWasApplied)
        {
            if (Result._CreatedActor)
            { World->DestroyActor(TargetActor, true, true); }
            Transaction.Cancel();
            Result._FailureReason = TEXT("Path-network actor rejected authoring configuration");
            return Result;
        }

        TargetActor->SetActorLocation(InRequest._DetectionBounds.GetCenter());
        TargetActor->Set_Ribbons(detail::Build_RelativeRibbons(
            *TargetActor, AuthoredWorldRibbons, Preview._GeneratedWorldRibbons, Result));
        auto RepresentativeRelativeRoutes =
            TArray<FCk_PathNetwork_RepresentativeRoute>{};
        RepresentativeRelativeRoutes.Reserve(
            RepresentativeWorldRoutes.Num());
        for (const auto& Route : RepresentativeWorldRoutes)
        {
            RepresentativeRelativeRoutes.Add(
                TargetActor->
                    Convert_WorldRepresentativeRouteToRelative(Route));
        }
        TargetActor->Set_RepresentativeRoutes(
            RepresentativeRelativeRoutes);
        TargetActor->PostEditChange();
        InRequest._TargetLevel->MarkPackageDirty();

        Result._Actor = TargetActor;
        Result._Succeeded = true;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
