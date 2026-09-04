#include "CkPathNetwork_Details.h"

#include "CkPathNetworkEditor/CkPathNetworkEditor_Log.h"
#include "CkPathNetworkEditor/CkPathNetwork_EditorUtils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"

#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>
#include <ScopedTransaction.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>

#define LOCTEXT_NAMESPACE "FCk_PathNetwork_Details"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FCk_PathNetwork_Details::
        MakeInstance()
        -> TSharedRef<IDetailCustomization>
    {
        return MakeShareable(new FCk_PathNetwork_Details);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_PathNetwork_Details::
        CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder)
        -> void
    {
        _CustomizedActors.Reset();

        auto CustomizedObjects = TArray<TWeakObjectPtr<UObject>>{};
        DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

        for (const auto& Object : CustomizedObjects)
        {
            if (auto* Actor = Cast<ACk_PathNetwork_UE>(Object.Get()))
            { _CustomizedActors.Add(Actor); }
        }

        auto& ToolsCategory = DetailBuilder.EditCategory(
            TEXT("Ck|PathNetwork|Tools"),
            LOCTEXT("ToolsCategory", "Path Network Tools"),
            ECategoryPriority::Important);

        const auto MakeButton = [this](const FText& InLabel, const FText& InTooltip, FReply(FCk_PathNetwork_Details::*InHandler)())
        {
            return SNew(SButton)
                .Text(InLabel)
                .ToolTipText(InTooltip)
                .OnClicked(FOnClicked::CreateSP(this, InHandler));
        };

        ToolsCategory.AddCustomRow(LOCTEXT("ToolsRowFilter", "Path Network Tools"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
            [
                MakeButton(
                    LOCTEXT("AddRibbon", "Add Ribbon"),
                    LOCTEXT("AddRibbonTooltip", "Add a new authored ribbon in front of the actor. Drag its point widgets in the viewport; add points via the details array."),
                    &FCk_PathNetwork_Details::DoAddRibbon)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
            [
                MakeButton(
                    LOCTEXT("RunDetector", "Run Detector"),
                    LOCTEXT("RunDetectorTooltip", "Run the assigned detector over the detection bounds and replace all Generated ribbons with the result. Authored ribbons are untouched."),
                    &FCk_PathNetwork_Details::DoRunDetector)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
            [
                MakeButton(
                    LOCTEXT("PromoteGenerated", "Promote Generated"),
                    LOCTEXT("PromoteGeneratedTooltip", "Convert every Generated ribbon to Authored so re-running the detector cannot replace it. Do this before hand-editing detector output."),
                    &FCk_PathNetwork_Details::DoPromoteGenerated)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
            [
                MakeButton(
                    LOCTEXT("ClearGenerated", "Clear Generated"),
                    LOCTEXT("ClearGeneratedTooltip", "Remove every Generated ribbon (authored ribbons stay)."),
                    &FCk_PathNetwork_Details::DoClearGenerated)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
            [
                MakeButton(
                    LOCTEXT("ValidateNavmesh", "Validate Generated vs Navmesh"),
                    LOCTEXT("ValidateNavmeshTooltip", "Check every generated ribbon point against the navmesh and report broad projections that move more than 50 cm in the plane or vertically. Authored ribbons are excluded."),
                    &FCk_PathNetwork_Details::DoValidateAgainstNavmesh)
            ]
        ];
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_PathNetwork_Details::
        DoRunDetector()
        -> FReply
    {
        for (const auto& WeakActor : _CustomizedActors)
        {
            auto* Actor = WeakActor.Get();
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            const auto Result = UCk_Utils_PathNetworkEditor_UE::Bake_DetectorToActor(Actor);
            if (NOT Result.Get_Succeeded())
            {
                ck::pathnetwork_editor::Warning(TEXT("Run Detector on [{}] failed: [{}]"),
                    Actor, Result.Get_FailureReason());
                continue;
            }

            ck::pathnetwork_editor::Display(TEXT("Run Detector on [{}]: [{}] generated ribbons (authored kept: [{}])"),
                Actor, Result.Get_GeneratedRibbonCount(), Result.Get_AuthoredRibbonCount());
        }

        return FReply::Handled();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_PathNetwork_Details::
        DoPromoteGenerated()
        -> FReply
    {
        const FScopedTransaction Transaction{LOCTEXT("PromoteGeneratedTransaction", "Path Network: Promote Generated Ribbons")};

        for (const auto& WeakActor : _CustomizedActors)
        {
            auto* Actor = WeakActor.Get();
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            Actor->Modify();

            auto Ribbons = Actor->Get_Ribbons();
            auto PromotedCount = 0;

            for (auto& Ribbon : Ribbons)
            {
                if (Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Generated)
                {
                    Ribbon.Set_Source(ECk_PathNetwork_RibbonSource::Authored);
                    ++PromotedCount;
                }
            }

            Actor->Set_Ribbons(Ribbons);
            Actor->PostEditChange();

            ck::pathnetwork_editor::Display(TEXT("Promote Generated on [{}]: [{}] ribbons promoted"), Actor, PromotedCount);
        }

        return FReply::Handled();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_PathNetwork_Details::
        DoClearGenerated()
        -> FReply
    {
        const FScopedTransaction Transaction{LOCTEXT("ClearGeneratedTransaction", "Path Network: Clear Generated Ribbons")};

        for (const auto& WeakActor : _CustomizedActors)
        {
            auto* Actor = WeakActor.Get();
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            Actor->Modify();

            auto Ribbons = Actor->Get_Ribbons();
            const auto RemovedCount = Ribbons.RemoveAll([](const FCk_PathNetwork_Ribbon& InRibbon)
            { return InRibbon.Get_Source() == ECk_PathNetwork_RibbonSource::Generated; });

            Actor->Set_Ribbons(Ribbons);
            Actor->PostEditChange();

            ck::pathnetwork_editor::Display(TEXT("Clear Generated on [{}]: [{}] ribbons removed"), Actor, RemovedCount);
        }

        return FReply::Handled();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_PathNetwork_Details::
        DoAddRibbon()
        -> FReply
    {
        const FScopedTransaction Transaction{LOCTEXT("AddRibbonTransaction", "Path Network: Add Ribbon")};

        for (const auto& WeakActor : _CustomizedActors)
        {
            auto* Actor = WeakActor.Get();
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            Actor->Modify();

            auto Points = TArray<FCk_PathNetwork_RibbonPoint>{};
            Points.Add(FCk_PathNetwork_RibbonPoint{FVector{200.0, 0.0, 0.0}, 100.0f});
            Points.Add(FCk_PathNetwork_RibbonPoint{FVector{700.0, 0.0, 0.0}, 100.0f});

            auto NewRibbon = FCk_PathNetwork_Ribbon{Points};
            NewRibbon.Set_Source(ECk_PathNetwork_RibbonSource::Authored);
            NewRibbon.Set_RibbonId(FGuid::NewGuid());

            auto Ribbons = Actor->Get_Ribbons();
            Ribbons.Add(NewRibbon);
            Actor->Set_Ribbons(Ribbons);
            Actor->PostEditChange();
        }

        return FReply::Handled();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_PathNetwork_Details::
        DoValidateAgainstNavmesh()
        -> FReply
    {
        for (const auto& WeakActor : _CustomizedActors)
        {
            auto* Actor = WeakActor.Get();
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            const auto Result = UCk_Utils_PathNetworkEditor_UE::Validate_RibbonPointProjectability(
                Actor, FVector{200.0, 200.0, 500.0}, 50.0f, 50.0f);
            if (NOT Result.Get_Succeeded())
            {
                ck::pathnetwork_editor::Warning(TEXT("Validate on [{}] failed: [{}]"),
                    Actor, Result.Get_FailureReason());
                continue;
            }

            for (const auto& UnprojectablePoint : Result.Get_UnprojectablePoints())
            {
                ck::pathnetwork_editor::Warning(TEXT("Validate on [{}]: generated ribbon point at {} fails navmesh conformance"),
                    Actor, UnprojectablePoint);
            }
            for (const auto& Failure : Result.Get_NonconformantPoints())
            {
                ck::pathnetwork_editor::Warning(TEXT("Validate on [{}]: generated point {} projected [{}] (status={}, planar delta={}cm, vertical delta={}cm) exceeds conformance tolerance"),
                    Actor, Failure.Get_SourcePoint(), Failure.Get_ProjectedPoint(), Failure.Get_Status(),
                    Failure.Get_PlanarDelta(), Failure.Get_VerticalDelta());
            }

            const auto UnprojectablePointCount = Result.Get_UnprojectablePoints().Num();
            if (UnprojectablePointCount == 0)
            {
                ck::pathnetwork_editor::Display(TEXT("Validate on [{}]: all [{}] generated ribbon points conform to navmesh"),
                    Actor, Result.Get_TotalPointCount());
            }
            else
            {
                ck::pathnetwork_editor::Warning(TEXT("Validate on [{}]: [{}] of [{}] generated ribbon points fail navmesh conformance"),
                    Actor, UnprojectablePointCount, Result.Get_TotalPointCount());
            }
        }

        return FReply::Handled();
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
