#include "CkPathNetwork_PathSimplify.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_path_simplify
{
    auto
    Does_ShortcutPreserveVerticalProfile(
        TConstArrayView<FVector> InPath,
        int32 InAnchorIndex,
        int32 InCandidateIndex,
        float InMaximumVerticalDeviationCm) -> bool
    {
        if (InCandidateIndex <= InAnchorIndex + 1)
        { return true; }

        const auto& Anchor = InPath[InAnchorIndex];
        const auto& Candidate = InPath[InCandidateIndex];
        const auto PlanarChord = FVector2D{Candidate - Anchor};
        const auto PlanarChordLengthSquared = PlanarChord.SquaredLength();
        if (PlanarChordLengthSquared <= UE_KINDA_SMALL_NUMBER)
        { return false; }

        for (auto Index = InAnchorIndex + 1; Index < InCandidateIndex; ++Index)
        {
            const auto& Point = InPath[Index];
            const auto AlongChord = FMath::Clamp(
                FVector2D::DotProduct(
                    FVector2D{Point - Anchor},
                    PlanarChord) / PlanarChordLengthSquared,
                0.0,
                1.0);
            const auto ChordZ = FMath::Lerp(Anchor.Z, Candidate.Z, AlongChord);
            if (FMath::Abs(Point.Z - ChordZ) > InMaximumVerticalDeviationCm)
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    Simplify_PathByTraversal(
        TConstArrayView<FVector> InPath,
        TFunctionRef<bool(const FVector&, const FVector&)> InIsShortcutTraversable,
        float InMaximumVerticalDeviationCm) -> TArray<FVector>
    {
        auto OriginalPath = TArray<FVector>{};
        if (NOT InPath.IsEmpty())
        { OriginalPath.Append(InPath.GetData(), InPath.Num()); }

        if (InPath.Num() <= 2 ||
            NOT FMath::IsFinite(InMaximumVerticalDeviationCm) ||
            InMaximumVerticalDeviationCm < 0.0f)
        { return OriginalPath; }

        auto Result = TArray<FVector>{};
        Result.Reserve(InPath.Num());
        Result.Add(InPath[0]);

        auto AnchorIndex = 0;
        while (AnchorIndex < InPath.Num() - 1)
        {
            auto BestCandidateIndex = AnchorIndex + 1;
            for (auto CandidateIndex = InPath.Num() - 1;
                 CandidateIndex > AnchorIndex;
                 --CandidateIndex)
            {
                if (NOT InIsShortcutTraversable(
                        InPath[AnchorIndex],
                        InPath[CandidateIndex]) ||
                    NOT ck_pathnetwork_path_simplify::Does_ShortcutPreserveVerticalProfile(
                        InPath,
                        AnchorIndex,
                        CandidateIndex,
                        InMaximumVerticalDeviationCm))
                { continue; }

                BestCandidateIndex = CandidateIndex;
                break;
            }

            Result.Add(InPath[BestCandidateIndex]);
            AnchorIndex = BestCandidateIndex;
        }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
