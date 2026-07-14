#include "CkPathNetwork_Details.h"

#include "CkPathNetworkEditor/CkPathNetworkEditor_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"
#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_Vectorize.h"

#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>
#include <NavigationSystem.h>
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
                    LOCTEXT("ValidateNavmesh", "Validate vs Navmesh"),
                    LOCTEXT("ValidateNavmeshTooltip", "Project every ribbon point onto the navmesh and report points with no navmesh underneath (agents cannot walk there)."),
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
        const FScopedTransaction Transaction{LOCTEXT("RunDetectorTransaction", "Path Network: Run Detector")};

        for (const auto& WeakActor : _CustomizedActors)
        {
            auto* Actor = WeakActor.Get();
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            const auto& Detector = Actor->Get_Detector();

            if (ck::Is_NOT_Valid(Detector))
            {
                ck::pathnetwork_editor::Warning(TEXT("Run Detector on [{}]: no detector assigned"), Actor);
                continue;
            }

            const auto Mask = Detector->Get_DetectionMask(Actor->Get_DetectionBounds());

            if (NOT Mask.Get_IsValidMask())
            {
                ck::pathnetwork_editor::Warning(TEXT("Run Detector on [{}]: detector returned an empty/invalid mask"), Actor);
                continue;
            }

            const auto GeneratedWorldRibbons = ck::pathnetwork::Vectorize_MaskToRibbons(Mask, Actor->Get_VectorizeParams());

            Actor->Modify();

            auto NewRibbons = TArray<FCk_PathNetwork_Ribbon>{};
            for (const auto& Ribbon : Actor->Get_Ribbons())
            {
                if (Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored)
                { NewRibbons.Add(Ribbon); }
            }

            for (const auto& WorldRibbon : GeneratedWorldRibbons)
            { NewRibbons.Add(Actor->Convert_WorldRibbonToRelative(WorldRibbon)); }

            Actor->Set_Ribbons(NewRibbons);
            Actor->PostEditChange();

            ck::pathnetwork_editor::Display(TEXT("Run Detector on [{}]: [{}] generated ribbons (authored kept: [{}])"),
                Actor, GeneratedWorldRibbons.Num(), NewRibbons.Num() - GeneratedWorldRibbons.Num());
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

            auto* World = Actor->GetWorld();
            auto* NavSys = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;

            if (NavSys == nullptr)
            {
                ck::pathnetwork_editor::Warning(TEXT("Validate on [{}]: no navigation system in this world"), Actor);
                continue;
            }

            const auto ProjectionExtent = FVector{200.0, 200.0, 500.0};
            auto OffMeshPoints = 0;
            auto TotalPoints = 0;

            for (const auto& WorldRibbon : Actor->Get_WorldRibbons())
            {
                for (const auto& Point : WorldRibbon.Get_Points())
                {
                    ++TotalPoints;

                    auto Projected = FNavLocation{};
                    if (NOT NavSys->ProjectPointToNavigation(Point.Get_Location(), Projected, ProjectionExtent))
                    {
                        ++OffMeshPoints;
                        ck::pathnetwork_editor::Warning(TEXT("Validate on [{}]: ribbon point at {} has no navmesh within {}"),
                            Actor, Point.Get_Location(), ProjectionExtent);
                    }
                }
            }

            if (OffMeshPoints == 0)
            {
                ck::pathnetwork_editor::Display(TEXT("Validate on [{}]: all [{}] ribbon points sit on navmesh"),
                    Actor, TotalPoints);
            }
            else
            {
                ck::pathnetwork_editor::Warning(TEXT("Validate on [{}]: [{}] of [{}] ribbon points are OFF the navmesh"),
                    Actor, OffMeshPoints, TotalPoints);
            }
        }

        return FReply::Handled();
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
