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

    struct FAssignment
    {
        int32 OriginIndex = INDEX_NONE;
        int32 OriginRank = INDEX_NONE;
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
        const FCk_Queue_Origin& Origin;
        float SpacingUu = 0.0f;
        int32 MaxNodes = 0;
        FPlacementValidator Validator;
        const TArray<FPlacement>& AllPlacements;
        int32& SearchNodesVisited;
        bool BudgetExhausted = false;
        int32 OriginIndex = INDEX_NONE;
        TArray<FCell> Path;
        TArray<FPlacement> OriginPlacements;

        auto
            MakeTransform(
                const FCell& InCell) const
            -> FTransform
        {
            const auto OriginWorld = Origin.Get_LocalTransform() * OwnerWorldTransform;
            const auto Location = OriginWorld.GetLocation()
                + OriginWorld.GetUnitAxis(EAxis::X) * (static_cast<float>(InCell.X) * SpacingUu)
                + OriginWorld.GetUnitAxis(EAxis::Y) * (static_cast<float>(InCell.Y) * SpacingUu);
            return FTransform{OriginWorld.GetRotation(), Location, OriginWorld.GetScale3D()};
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
            const auto Previous = OriginPlacements.IsEmpty()
                ? TOptional<FTransform>{}
                : TOptional<FTransform>{OriginPlacements.Last().TargetWorldTransform};

            return Validator(OutCandidate, Previous)
                && IsPlacementClear(OutCandidate, SpacingUu, OriginPlacements)
                && IsPlacementClear(OutCandidate, SpacingUu, AllPlacements);
        }

        auto
            Search(
                int32 InNeeded,
                const FCell& InCurrent,
                const FCell& InHeading)
            -> bool
        {
            if (OriginPlacements.Num() == InNeeded)
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
                OriginPlacements.Add(FPlacement{
                    OriginIndex,
                    OriginPlacements.Num(),
                    Candidate});

                if (Search(InNeeded, Next, Direction))
                { return true; }

                OriginPlacements.Pop(EAllowShrinking::No);
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
            OriginPlacements.Add(FPlacement{
                OriginIndex,
                0,
                FrontTransform});

            return Search(InNeeded, FrontCell, FCell{-1, 0});
        }
    };

    auto
        IsOriginAvailable(
            const FCk_Queue_Origin& InOrigin,
            int32 InLoad)
        -> bool
    {
        const auto Cap = InOrigin.Get_HardLimitOverride();
        return Cap <= 0 || InLoad < Cap;
    }

    auto
        SelectLeastLoadedOrigin(
            const TArray<FCk_Queue_Origin>& InOrigins,
            const TArray<int32>& InLoads)
        -> int32
    {
        auto Best = int32{INDEX_NONE};
        for (auto Index = 0; Index < InOrigins.Num(); ++Index)
        {
            if (NOT IsOriginAvailable(InOrigins[Index], InLoads[Index]))
            { continue; }

            const auto IsLessLoaded = Best == INDEX_NONE
                || static_cast<int64>(InLoads[Index]) * InOrigins[Best].Get_Weight()
                    < static_cast<int64>(InLoads[Best]) * InOrigins[Index].Get_Weight();

            if (IsLessLoaded)
            { Best = Index; }
        }
        return Best;
    }

    auto
        OrientPlacementsTowardOrigin(
            const FTransform& InOriginWorld,
            TArray<FPlacement>& InOutPlacements)
        -> void
    {
        for (auto& Placement : InOutPlacements)
        {
            auto DirectionToOrigin = InOriginWorld.GetLocation()
                - Placement.TargetWorldTransform.GetLocation();
            DirectionToOrigin.Z = 0.0f;

            const auto HasDirectionToOrigin = DirectionToOrigin.Normalize();
            const auto FacingRotation = HasDirectionToOrigin
                ? FQuat{FRotator{0.0f, DirectionToOrigin.Rotation().Yaw, 0.0f}}
                : InOriginWorld.GetRotation();
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
            const TArray<FCk_Queue_Origin>& InOrigins,
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
            && InMaxSearchNodes > 0
            && NOT InOrigins.IsEmpty();
        if (NOT InputIsValid)
        { return Result; }

        if (InMemberCount == 0)
        {
            Result.Outcome = EBuildOutcome::Success;
            return Result;
        }

        auto Loads = TArray<int32>{};
        Loads.Init(0, InOrigins.Num());

        auto Assignments = TArray<details::FAssignment>{};
        Assignments.Reserve(InMemberCount);

        for (auto MemberIndex = 0; MemberIndex < InMemberCount; ++MemberIndex)
        {
            const auto OriginIndex = details::SelectLeastLoadedOrigin(InOrigins, Loads);
            if (OriginIndex == INDEX_NONE)
            { return Result; }

            Assignments.Add(details::FAssignment{OriginIndex, Loads[OriginIndex]});
            ++Loads[OriginIndex];
        }

        auto PerOriginPlacements = TArray<TArray<FPlacement>>{};
        PerOriginPlacements.SetNum(InOrigins.Num());
        auto AllPlacements = TArray<FPlacement>{};
        AllPlacements.Reserve(InMemberCount);

        for (auto OriginIndex = 0; OriginIndex < InOrigins.Num(); ++OriginIndex)
        {
            const auto Needed = Loads[OriginIndex];
            if (Needed == 0)
            { continue; }

            auto& OriginPlacements = PerOriginPlacements[OriginIndex];
            const auto OriginWorld = InOrigins[OriginIndex].Get_LocalTransform() * InOwnerWorldTransform;

            if (InLayoutAlgorithm == ECk_Queue_LayoutAlgorithm::Linear)
            {
                for (auto Rank = 0; Rank < Needed; ++Rank)
                {
                    if (Result.SearchNodesVisited >= InMaxSearchNodes)
                    {
                        Result.Outcome = EBuildOutcome::SearchBudgetExhausted;
                        return Result;
                    }
                    ++Result.SearchNodesVisited;

                    const auto Location = OriginWorld.GetLocation()
                        - OriginWorld.GetUnitAxis(EAxis::X) * (InSpacingUu * Rank);
                    auto Candidate = FTransform{
                        OriginWorld.GetRotation(),
                        Location,
                        OriginWorld.GetScale3D()};
                    const auto Previous = OriginPlacements.IsEmpty()
                        ? TOptional<FTransform>{}
                        : TOptional<FTransform>{OriginPlacements.Last().TargetWorldTransform};

                    if (NOT InValidator(Candidate, Previous)
                        || NOT details::IsPlacementClear(
                            Candidate,
                            InSpacingUu,
                            OriginPlacements)
                        || NOT details::IsPlacementClear(
                            Candidate,
                            InSpacingUu,
                            AllPlacements))
                    { return Result; }

                    OriginPlacements.Add(FPlacement{OriginIndex, Rank, Candidate});
                }
            }
            else if (InLayoutAlgorithm == ECk_Queue_LayoutAlgorithm::OrthogonalSnake)
            {
                auto Search = details::FSnakeSearch{
                    InOwnerWorldTransform,
                    InOrigins[OriginIndex],
                    InSpacingUu,
                    InMaxSearchNodes,
                    InValidator,
                    AllPlacements,
                    Result.SearchNodesVisited,
                    false,
                    OriginIndex};

                if (NOT Search.Build(Needed))
                {
                    Result.Outcome = Search.BudgetExhausted
                        ? EBuildOutcome::SearchBudgetExhausted
                        : EBuildOutcome::NoViablePlacement;
                    return Result;
                }
                OriginPlacements = MoveTemp(Search.OriginPlacements);
            }
            else
            { return Result; }

            details::OrientPlacementsTowardOrigin(OriginWorld, OriginPlacements);

            AllPlacements.Append(OriginPlacements);
        }

        Result.Placements.Reserve(InMemberCount);
        for (const auto& Assignment : Assignments)
        {
            const auto PlacementExists = PerOriginPlacements.IsValidIndex(Assignment.OriginIndex)
                && PerOriginPlacements[Assignment.OriginIndex].IsValidIndex(Assignment.OriginRank);
            if (NOT PlacementExists)
            {
                Result.Placements.Reset();
                return Result;
            }

            Result.Placements.Add(
                PerOriginPlacements[Assignment.OriginIndex][Assignment.OriginRank]);
        }

        Result.Outcome = EBuildOutcome::Success;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
