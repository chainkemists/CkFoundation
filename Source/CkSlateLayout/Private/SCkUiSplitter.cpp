#include "CkSlateLayout/SCkUiSplitter.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SNullWidget.h"

#include <cmath>
#include <limits>

namespace ck_ui_splitter
{
    auto ValidatePanes(const TArray<SCkUiSplitter::FPane>& InPanes, FString& OutFailure) -> bool
    {
        if (InPanes.Num() < 2)
        {
            OutFailure = TEXT("splitter requires at least two panes");
            return false;
        }

        TSet<FString> Ids;
        double TotalWeight = 0.0;
        for (const SCkUiSplitter::FPane& Pane : InPanes)
        {
            if (Pane.Id.IsEmpty())
            {
                OutFailure = TEXT("splitter pane id is required");
                return false;
            }
            if (Ids.Contains(Pane.Id))
            {
                OutFailure = FString::Printf(TEXT("splitter pane id '%s' is duplicated"), *Pane.Id);
                return false;
            }
            Ids.Add(Pane.Id);
            if (!Pane.Content.IsValid())
            {
                OutFailure = FString::Printf(TEXT("splitter pane '%s' has no content"), *Pane.Id);
                return false;
            }
            if (!FMath::IsFinite(Pane.Weight) || Pane.Weight <= 0.0f)
            {
                OutFailure = FString::Printf(TEXT("splitter pane '%s' has invalid weight"), *Pane.Id);
                return false;
            }
            if (!FMath::IsFinite(Pane.MinSize) || Pane.MinSize < 0.0f)
            {
                OutFailure = FString::Printf(TEXT("splitter pane '%s' has invalid minimum size"), *Pane.Id);
                return false;
            }

            TotalWeight += static_cast<double>(Pane.Weight);
            if (!std::isfinite(TotalWeight) || TotalWeight > static_cast<double>(std::numeric_limits<float>::max()))
            {
                OutFailure = TEXT("splitter pane weights overflow");
                return false;
            }
        }

        return true;
    }

    auto CollectCandidateWidgets(const TSharedRef<SWidget>& InRoot, const SWidget* InOwner, const SWidget* InNativeSplitter,
        const TSet<const SWidget*>* InExistingWidgets, const TSet<const SWidget*>* InReusableWidgets,
        TSet<const SWidget*>& InOutWidgets, FString& OutFailure) -> bool
    {
        constexpr int32 MaxCandidateWidgets = 4096;
        auto Pending = TArray<TSharedRef<SWidget>>{ InRoot };
        while (!Pending.IsEmpty())
        {
            const TSharedRef<SWidget> Current = Pending.Pop(EAllowShrinking::No);
            const SWidget* CurrentPointer = &Current.Get();
            if (CurrentPointer == &SNullWidget::NullWidget.Get()) { continue; }
            if (CurrentPointer == InOwner || CurrentPointer == InNativeSplitter)
            {
                OutFailure = TEXT("splitter pane content contains the splitter");
                return false;
            }
            if (InExistingWidgets != nullptr && InExistingWidgets->Contains(CurrentPointer)
                && (InReusableWidgets == nullptr || !InReusableWidgets->Contains(CurrentPointer)))
            {
                OutFailure = TEXT("splitter pane content aliases existing mounted content");
                return false;
            }
            if (InOutWidgets.Contains(CurrentPointer))
            {
                OutFailure = TEXT("splitter pane content reuses a widget");
                return false;
            }
            if (InOutWidgets.Num() >= MaxCandidateWidgets)
            {
                OutFailure = TEXT("splitter pane content hierarchy exceeds the widget limit");
                return false;
            }
            InOutWidgets.Add(CurrentPointer);
            FChildren* Children = Current->GetChildren();
            if (Children == nullptr) { continue; }
            for (int32 Index = 0; Index < Children->Num(); ++Index)
            {
                Pending.Add(Children->GetChildAt(Index));
            }
        }
        return true;
    }

    class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
    {
    public:
        FPreparedUpdate(TWeakPtr<SCkUiSplitter> InOwner, TArray<SCkUiSplitter::FPane>&& InPanes)
            : _Owner(MoveTemp(InOwner)), _Panes(MoveTemp(InPanes)) {}

        virtual void Commit() noexcept override
        {
            if (const TSharedPtr<SCkUiSplitter> Owner = _Owner.Pin())
            {
                Owner->Commit(MoveTemp(_Panes));
            }
        }

    private:
        TWeakPtr<SCkUiSplitter> _Owner;
        TArray<SCkUiSplitter::FPane> _Panes;
    };
}

void SCkUiSplitter::Construct(const FArguments& InArgs)
{
    SAssignNew(_Splitter, SSplitter)
        .Orientation(InArgs._Orientation)
        .MinimumSlotHeight(0.0f);
    ChildSlot[_Splitter.ToSharedRef()];
}

auto SCkUiSplitter::Prepare(TArray<FPane> InPanes, FString& OutFailure) -> TUniquePtr<ICkUiPreparedWidgetUpdate>
{
    OutFailure.Reset();
    if (!_Splitter.IsValid())
    {
        OutFailure = TEXT("splitter is not constructed");
        return {};
    }
    if (!ck_ui_splitter::ValidatePanes(InPanes, OutFailure))
    {
        return {};
    }

    if (_Panes.Num() != _Splitter->NumSlots())
    {
        OutFailure = TEXT("splitter pane state no longer matches native slots");
        return {};
    }

    TSet<const SWidget*> ExistingWidgets;
    FString ExistingFailure;
    for (const FCommittedPane& Existing : _Panes)
    {
        if (!Existing.Content.IsValid()
            || !ck_ui_splitter::CollectCandidateWidgets(Existing.Content.ToSharedRef(), this, _Splitter.Get(), nullptr, nullptr, ExistingWidgets, ExistingFailure))
        {
            OutFailure = TEXT("splitter committed content is invalid");
            return {};
        }
    }

    TSet<const SWidget*> CandidateWidgets;
    double TotalCoefficient = 0.0;
    for (const FPane& Pane : InPanes)
    {
        const int32 ExistingIndex = _Panes.IndexOfByPredicate([&Pane](const FCommittedPane& Existing) { return Existing.Id == Pane.Id; });
        const bool bPreservesNativeCoefficient = ExistingIndex != INDEX_NONE && _Panes[ExistingIndex].AuthoredWeight == Pane.Weight;
        const bool bReusesExistingContent = ExistingIndex != INDEX_NONE && _Panes[ExistingIndex].Content == Pane.Content;
        if (Pane.Content->GetParentWidget().IsValid() && !bReusesExistingContent)
        {
            OutFailure = FString::Printf(TEXT("splitter pane '%s' content is already mounted"), *Pane.Id);
            return {};
        }
        TSet<const SWidget*> ReusableWidgets;
        if (bReusesExistingContent
            && !ck_ui_splitter::CollectCandidateWidgets(_Panes[ExistingIndex].Content.ToSharedRef(), this, _Splitter.Get(), nullptr, nullptr, ReusableWidgets, OutFailure))
        {
            return {};
        }
        if (!ck_ui_splitter::CollectCandidateWidgets(Pane.Content.ToSharedRef(), this, _Splitter.Get(), &ExistingWidgets,
            bReusesExistingContent ? &ReusableWidgets : nullptr, CandidateWidgets, OutFailure))
        {
            return {};
        }
        const float Coefficient = bPreservesNativeCoefficient ? _Splitter->SlotAt(ExistingIndex).GetSizeValue() : Pane.Weight;
        // Native dragging may legitimately collapse a zero-minimum pane to zero. New authored weights cannot.
        if (!FMath::IsFinite(Coefficient) || Coefficient < 0.0f)
        {
            OutFailure = FString::Printf(TEXT("splitter pane '%s' has invalid retained coefficient"), *Pane.Id);
            return {};
        }
        TotalCoefficient += static_cast<double>(Coefficient);
        if (!std::isfinite(TotalCoefficient) || TotalCoefficient > static_cast<double>(std::numeric_limits<float>::max()))
        {
            OutFailure = TEXT("splitter pane coefficients overflow");
            return {};
        }
    }
    if (TotalCoefficient <= 0.0)
    {
        OutFailure = TEXT("splitter pane coefficients must have a positive total");
        return {};
    }
    return MakeUnique<ck_ui_splitter::FPreparedUpdate>(TWeakPtr<SCkUiSplitter>(SharedThis(this)), MoveTemp(InPanes));
}

auto SCkUiSplitter::GetSplitter() const -> TSharedPtr<SSplitter>
{
    return _Splitter;
}

auto SCkUiSplitter::Commit(TArray<FPane>&& InPanes) -> void
{
    if (!_Splitter.IsValid())
    {
        return;
    }

    bool bSameOrder = _Panes.Num() == InPanes.Num();
    if (bSameOrder)
    {
        for (int32 Index = 0; Index < InPanes.Num(); ++Index)
        {
            if (_Panes[Index].Id != InPanes[Index].Id)
            {
                bSameOrder = false;
                break;
            }
        }
    }

    if (bSameOrder)
    {
        for (int32 Index = 0; Index < InPanes.Num(); ++Index)
        {
            const FPane& Pane = InPanes[Index];
            FCommittedPane& Existing = _Panes[Index];
            SSplitter::FSlot& Slot = _Splitter->SlotAt(Index);
            if (Existing.Content != Pane.Content)
            {
                Slot.AttachWidget(Pane.Content.ToSharedRef());
            }
            Slot.SetMinSize(Pane.MinSize);
            if (Existing.AuthoredWeight != Pane.Weight)
            {
                Slot.SetSizeValue(Pane.Weight);
            }
            Existing.Content = Pane.Content;
            Existing.AuthoredWeight = Pane.Weight;
            Existing.MinSize = Pane.MinSize;
        }
        return;
    }

    if (_Splitter->HasMouseCapture() && FSlateApplication::IsInitialized())
    {
        // SSplitter stores its drag flag until OnMouseButtonUp; releasing capture alone leaves that flag set.
        const TSet<FKey> NoPressedButtons;
        const FPointerEvent EndDrag(0, FVector2D::ZeroVector, FVector2D::ZeroVector, NoPressedButtons,
            EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
        _Splitter->OnMouseButtonUp(FGeometry(), EndDrag);
        // SSplitter takes the cursor capture while dragging its handle. Do not disturb another captor.
        FSlateApplication::Get().GetCursorUser()->ReleaseCursorCapture();
    }

    TArray<float> PreviousCoefficients;
    PreviousCoefficients.Reserve(_Panes.Num());
    for (int32 Index = 0; Index < _Panes.Num(); ++Index)
    {
        const float Coefficient = _Splitter->IsValidSlotIndex(Index) ? _Splitter->SlotAt(Index).GetSizeValue() : 0.0f;
        PreviousCoefficients.Add(Coefficient);
    }
    TArray<FCommittedPane> Previous = MoveTemp(_Panes);
    _Splitter->ClearChildren();
    _Panes.Reset(InPanes.Num());
    for (int32 Index = 0; Index < InPanes.Num(); ++Index)
    {
        FPane& Pane = InPanes[Index];
        const int32 PreviousIndex = Previous.IndexOfByPredicate([&Pane](const FCommittedPane& Existing) { return Existing.Id == Pane.Id; });
        const bool bAuthoredWeightUnchanged = PreviousIndex != INDEX_NONE && Previous[PreviousIndex].AuthoredWeight == Pane.Weight;
        const float PreservedCoefficient = bAuthoredWeightUnchanged ? PreviousCoefficients[PreviousIndex] : Pane.Weight;
        _Splitter->AddSlot()
            .Value(PreservedCoefficient)
            .MinSize(Pane.MinSize)
            [Pane.Content.ToSharedRef()];
        _Panes.Add({ MoveTemp(Pane.Id), MoveTemp(Pane.Content), Pane.Weight, Pane.MinSize });
    }
}
