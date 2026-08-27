#include "CkQueue/Queue/CkQueue_Layout_Algorithm.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::queue::layout::details
{
    struct FCell
    {
        int32 X = 0;
        int32 Y = 0;

        auto operator==(const FCell& InOther) const -> bool
        { return X == InOther.X && Y == InOther.Y; }
    };

    auto
        IsPlacementClear(
            const FTransform& InCandidate,
            float InSpacingUu,
            const TArray<FPlacement>& InPlaced)
        -> bool
    {
        for (const auto& Placement : InPlaced)
        {
            if (FVector::Dist2D(
                    InCandidate.GetLocation(),
                    Placement.TargetWorldTransform.GetLocation()) < InSpacingUu - KINDA_SMALL_NUMBER)
            { return false; }
        }
        return true;
    }

    struct FSnakeSearch
    {
        const FTransform& OwnerWorldTransform;
        float SpacingUu = 0.0f;
        int32 MaxNodes = 0;
        FPlacementValidator Validator;
        int32& SearchNodesVisited;
        bool BudgetExhausted = false;
        TArray<FCell> Path;
        TArray<FPlacement> Placements;

        auto
            MakeTransform(
                const FCell& InCell) const
            -> FTransform
        {
            const auto Location = OwnerWorldTransform.GetLocation()
                + OwnerWorldTransform.GetUnitAxis(EAxis::X) * (static_cast<float>(InCell.X) * SpacingUu)
                + OwnerWorldTransform.GetUnitAxis(EAxis::Y) * (static_cast<float>(InCell.Y) * SpacingUu);
            return FTransform{
                OwnerWorldTransform.GetRotation(),
                Location,
                OwnerWorldTransform.GetScale3D()};
        }

        auto
            Contains(
                const FCell& InCell) const
            -> bool
        {
            return Path.Contains(InCell);
        }

        auto
            TryCandidate(
                const FCell& InCell,
                FTransform& OutCandidate)
            -> bool
        {
            if (SearchNodesVisited >= MaxNodes)
            {
                BudgetExhausted = true;
                return false;
            }
            ++SearchNodesVisited;

            OutCandidate = MakeTransform(InCell);
            const auto Previous = Placements.IsEmpty()
                ? TOptional<FTransform>{}
                : TOptional<FTransform>{Placements.Last().TargetWorldTransform};

            return Validator(OutCandidate, Previous)
                && IsPlacementClear(OutCandidate, SpacingUu, Placements);
        }

        auto
            Search(
                int32 InNeeded,
                const FCell& InCurrent,
                const FCell& InHeading)
            -> bool
        {
            if (Placements.Num() == InNeeded)
            { return true; }

            const auto Directions = TArray<FCell>{
                InHeading,
                FCell{-InHeading.Y, InHeading.X},
                FCell{InHeading.Y, -InHeading.X}};

            for (const auto& Direction : Directions)
            {
                const auto Next = FCell{InCurrent.X + Direction.X, InCurrent.Y + Direction.Y};
                auto Candidate = FTransform::Identity;
                if (Contains(Next) || NOT TryCandidate(Next, Candidate))
                {
                    if (BudgetExhausted)
                    { return false; }
                    continue;
                }

                Path.Add(Next);
                Placements.Add(FPlacement{Placements.Num(), Candidate});

                if (Search(InNeeded, Next, Direction))
                { return true; }

                Placements.Pop(EAllowShrinking::No);
                Path.Pop(EAllowShrinking::No);

                if (BudgetExhausted)
                { return false; }
            }
            return false;
        }

        auto
            Build(
                int32 InNeeded)
            -> bool
        {
            const auto FrontCell = FCell{};
            auto FrontTransform = FTransform::Identity;
            if (NOT TryCandidate(FrontCell, FrontTransform))
            { return false; }

            Path.Add(FrontCell);
            Placements.Add(FPlacement{0, FrontTransform});

            return Search(InNeeded, FrontCell, FCell{-1, 0});
        }
    };

    auto
        OrientPlacementsTowardOwner(
            const FTransform& InOwnerWorldTransform,
            TArray<FPlacement>& InOutPlacements)
        -> void
    {
        for (auto& Placement : InOutPlacements)
        {
            auto DirectionToOwner = InOwnerWorldTransform.GetLocation()
                - Placement.TargetWorldTransform.GetLocation();
            DirectionToOwner.Z = 0.0f;

            const auto HasDirectionToOwner = DirectionToOwner.Normalize();
            const auto FacingRotation = HasDirectionToOwner
                ? FQuat{FRotator{0.0f, DirectionToOwner.Rotation().Yaw, 0.0f}}
                : InOwnerWorldTransform.GetRotation();
            Placement.TargetWorldTransform.SetRotation(FacingRotation);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::queue::layout
{
    auto
        Build(
            const FTransform& InOwnerWorldTransform,
            int32 InMemberCount,
            float InSpacingUu,
            int32 InMaxSearchNodes,
            ECk_Queue_LayoutAlgorithm InLayoutAlgorithm,
            FPlacementValidator InValidator)
        -> FBuildResult
    {
        auto Result = FBuildResult{};
        const auto InputIsValid = InMemberCount >= 0
            && InSpacingUu > 0.0f
            && InMaxSearchNodes > 0;
        if (NOT InputIsValid)
        { return Result; }

        if (InMemberCount == 0)
        {
            Result.Outcome = EBuildOutcome::Success;
            return Result;
        }

        auto Placements = TArray<FPlacement>{};
        Placements.Reserve(InMemberCount);

        if (InLayoutAlgorithm == ECk_Queue_LayoutAlgorithm::Linear)
        {
            for (auto Rank = 0; Rank < InMemberCount; ++Rank)
            {
                if (Result.SearchNodesVisited >= InMaxSearchNodes)
                {
                    Result.Outcome = EBuildOutcome::SearchBudgetExhausted;
                    return Result;
                }
                ++Result.SearchNodesVisited;

                const auto Location = InOwnerWorldTransform.GetLocation()
                    - InOwnerWorldTransform.GetUnitAxis(EAxis::X) * (InSpacingUu * Rank);
                auto Candidate = FTransform{
                    InOwnerWorldTransform.GetRotation(),
                    Location,
                    InOwnerWorldTransform.GetScale3D()};
                const auto Previous = Placements.IsEmpty()
                    ? TOptional<FTransform>{}
                    : TOptional<FTransform>{Placements.Last().TargetWorldTransform};

                if (NOT InValidator(Candidate, Previous)
                    || NOT details::IsPlacementClear(Candidate, InSpacingUu, Placements))
                { return Result; }

                Placements.Add(FPlacement{Rank, Candidate});
            }
        }
        else if (InLayoutAlgorithm == ECk_Queue_LayoutAlgorithm::OrthogonalSnake)
        {
            auto Search = details::FSnakeSearch{
                InOwnerWorldTransform,
                InSpacingUu,
                InMaxSearchNodes,
                InValidator,
                Result.SearchNodesVisited};

            if (NOT Search.Build(InMemberCount))
            {
                Result.Outcome = Search.BudgetExhausted
                    ? EBuildOutcome::SearchBudgetExhausted
                    : EBuildOutcome::NoViablePlacement;
                return Result;
            }
            Placements = MoveTemp(Search.Placements);
        }
        else
        { return Result; }

        details::OrientPlacementsTowardOwner(InOwnerWorldTransform, Placements);

        Result.Placements = MoveTemp(Placements);
        Result.Outcome = EBuildOutcome::Success;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
