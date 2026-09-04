#include "CkGroundNav_Fingerprint.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace fingerprint_private
    {
        // Bit-exact hashing of one scalar. Negative zero is folded onto positive zero first: the two have
        // different bit patterns but identical meaning, and a fingerprint that disagreed with equality
        // would force rebuilds of a world that did not change.
        auto DoHash_Scalar(
            double InValue,
            uint64 InSeed) -> uint64
        {
            if (InValue == 0.0)
            { InValue = 0.0; }

            auto Bits = uint64{};
            FMemory::Memcpy(&Bits, &InValue, sizeof(Bits));

            // 64-bit mixer (splitmix64 finalizer): avalanches every input bit, so a one-cell change in a
            // coordinate cannot alias with an unrelated one.
            auto Mixed = Bits ^ (InSeed * 0x9E3779B97F4A7C15ULL);
            Mixed ^= Mixed >> 30;
            Mixed *= 0xBF58476D1CE4E5B9ULL;
            Mixed ^= Mixed >> 27;
            Mixed *= 0x94D049BB133111EBULL;
            Mixed ^= Mixed >> 31;

            return Mixed;
        }

        auto DoHash_Vector(
            const FVector& InVector,
            uint64         InSeed) -> uint64
        {
            auto Hash = InSeed;

            // Sequential, NOT commutative: within one vector the axes are ordered, and (1,0,0) must not
            // fingerprint the same as (0,0,1).
            Hash = DoHash_Scalar(InVector.X, Hash) * 0x100000001B3ULL + 1;
            Hash = DoHash_Scalar(InVector.Y, Hash) * 0x100000001B3ULL + 2;
            Hash = DoHash_Scalar(InVector.Z, Hash) * 0x100000001B3ULL + 3;

            return Hash;
        }


        auto DoHash_String(
            const FString& InValue,
            uint64         InSeed) -> uint64
        {
            auto Hash = DoHash_Scalar(static_cast<double>(InValue.Len()), InSeed);

            for (const auto Character : InValue)
            { Hash = DoHash_Scalar(static_cast<double>(Character), Hash); }

            return Hash;
        }

        auto DoHash_Transform(
            const FTransform& InTransform,
            uint64            InSeed) -> uint64
        {
            const auto Rotation = InTransform.GetRotation();

            auto Hash = DoHash_Vector(InTransform.GetLocation(), InSeed);

            Hash = DoHash_Scalar(Rotation.X, Hash);
            Hash = DoHash_Scalar(Rotation.Y, Hash);
            Hash = DoHash_Scalar(Rotation.Z, Hash);
            Hash = DoHash_Scalar(Rotation.W, Hash);

            return DoHash_Vector(InTransform.GetScale3D(), Hash);
        }

        // The type and the dimensions its type gives meaning to. Only the authored branch is read: a
        // box's unread capsule members are whatever the struct was default-constructed with, and
        // folding those in would fingerprint a difference the bake cannot see.
        auto DoHash_Shape(
            const FCk_AnyShape& InShape,
            uint64              InSeed) -> uint64
        {
            auto Hash = DoHash_Scalar(static_cast<double>(static_cast<uint8>(InShape.Get_ShapeType())), InSeed);

            switch (InShape.Get_ShapeType())
            {
                case ECk_Shape_Type::Box:
                {
                    return DoHash_Vector(InShape.Get_Box().Get_HalfExtents(), Hash);
                }

                case ECk_Shape_Type::Capsule:
                {
                    Hash = DoHash_Scalar(InShape.Get_Capsule().Get_HalfHeight(), Hash);
                    return DoHash_Scalar(InShape.Get_Capsule().Get_Radius(), Hash);
                }

                case ECk_Shape_Type::Cylinder:
                {
                    Hash = DoHash_Scalar(InShape.Get_Cylinder().Get_HalfHeight(), Hash);
                    return DoHash_Scalar(InShape.Get_Cylinder().Get_Radius(), Hash);
                }

                case ECk_Shape_Type::Sphere:
                {
                    return DoHash_Scalar(InShape.Get_Sphere().Get_Radius(), Hash);
                }

                case ECk_Shape_Type::None:
                default:
                {
                    return Hash;
                }
            }
        }

        // The members of item 4, in declaration order. One place, because item 9 hashes a profile too
        // and a variant judged by a different set of members from the default would be a variant that
        // could change without the fingerprint noticing.
        auto DoHash_Profile(
            const FCk_GroundNav_AgentProfile& InProfile,
            uint64                            InSeed) -> uint64
        {
            auto Hash = DoHash_Scalar(InProfile.Get_MaxSlopeDegrees(), InSeed);

            Hash = DoHash_Scalar(InProfile.Get_MaxSlopeChangeDegrees(), Hash);
            Hash = DoHash_Scalar(InProfile.Get_StepHeightUu(), Hash);
            Hash = DoHash_Scalar(InProfile.Get_LedgeSensitivity(), Hash);
            Hash = DoHash_Scalar(InProfile.Get_RoughPerchToleranceUu(), Hash);

            return Hash;
        }

        /**
         * One triangle's hash, independent of which of its three corners is listed first but NOT of its
         * winding: rotating (A,B,C) to (B,C,A) is the same surface, while reversing it to (C,B,A) flips
         * the facing and is a different world.
         */
        auto DoHash_Triangle(
            const FVector& InA,
            const FVector& InB,
            const FVector& InC) -> uint64
        {
            const auto IsLess = [](const FVector& InLhs, const FVector& InRhs) -> bool
            {
                if (InLhs.X != InRhs.X) { return InLhs.X < InRhs.X; }
                if (InLhs.Y != InRhs.Y) { return InLhs.Y < InRhs.Y; }
                return InLhs.Z < InRhs.Z;
            };

            // Rotate so the lexicographically smallest corner leads, preserving cyclic order.
            const auto* First = &InA;
            const auto* Second = &InB;
            const auto* Third = &InC;

            if (IsLess(InB, *First)) { First = &InB; Second = &InC; Third = &InA; }
            if (IsLess(InC, *First)) { First = &InC; Second = &InA; Third = &InB; }

            auto Hash = uint64{0xCBF29CE484222325ULL};

            Hash = DoHash_Vector(*First, Hash);
            Hash = DoHash_Vector(*Second, Hash);
            Hash = DoHash_Vector(*Third, Hash);

            return Hash;
        }

        /**
         * Items 2 through 9 of the frozen enumeration, chained onto a seed.
         *
         * The seed IS item 1: Get_ContentFingerprint hands over the geometry hash and
         * Get_InputFingerprint hands over nothing, so there is one implementation of the enumeration
         * and the two answers cannot drift apart as items are added to it.
         */
        auto DoHash_Inputs(
            uint64                                                    InSeed,
            const FBox&                                               InRegion,
            const FCk_GroundNav_BakeConfig&                           InConfig,
            const FCk_GroundNav_AgentProfile&                         InProfile,
            TConstArrayView<FCk_GroundNav_MarkupRecord>               InMarkups,
            TConstArrayView<FCk_GroundNav_LinkRecord>                 InLinks,
            const FCk_GroundNav_MergeTunables&                        InMergeTunables,
            float                                                     InMaxClearanceUu,
            TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants) -> uint64
        {
            auto Hash = InSeed;

            // ---- 2. Region -----------------------------------------------------------------------------
            Hash = DoHash_Vector(InRegion.Min, Hash);
            Hash = DoHash_Vector(InRegion.Max, Hash);

            // ---- 3. Bake config ------------------------------------------------------------------------
            Hash = DoHash_Scalar(InConfig.Get_CellSizeUu(), Hash);
            Hash = DoHash_Scalar(InConfig.Get_CellHeightUu(), Hash);
            Hash = DoHash_Scalar(InConfig.Get_TileSizeUu(), Hash);
            Hash = DoHash_Scalar(static_cast<double>(InConfig.Get_MaxColumnsPerTile()), Hash);

            // ---- 4. Agent profile ----------------------------------------------------------------------
            Hash = DoHash_Profile(InProfile, Hash);

            // ---- 5. Markup, in canonical id order ------------------------------------------------------
            auto EnabledOrder = TArray<int32>{};
            EnabledOrder.Reserve(InMarkups.Num());

            for (auto Index = 0; Index < InMarkups.Num(); ++Index)
            {
                if (InMarkups[Index].Get_Enable() == ECk_EnableDisable::Disable)
                { continue; }

                EnabledOrder.Emplace(Index);
            }

            EnabledOrder.Sort([&](int32 InLhs, int32 InRhs) -> bool
            {
                return InMarkups[InLhs].Get_Id() < InMarkups[InRhs].Get_Id();
            });

            Hash = DoHash_Scalar(static_cast<double>(EnabledOrder.Num()), Hash);

            for (const auto Index : EnabledOrder)
            {
                const auto& Markup = InMarkups[Index];

                Hash = DoHash_Scalar(static_cast<double>(Markup.Get_Id()), Hash);
                Hash = DoHash_Shape(Markup.Get_Shape(), Hash);
                Hash = DoHash_Transform(Markup.Get_WorldTransform(), Hash);
                Hash = DoHash_String(Markup.Get_AreaTag().ToString(), Hash);
                Hash = DoHash_Scalar(static_cast<double>(static_cast<uint8>(Markup.Get_Kind())), Hash);
                Hash = DoHash_Scalar(static_cast<double>(Markup.Get_CostMultiplier()), Hash);
            }

            // ---- 6. Links, in the order the list carries them -------------------------------------------
            auto EnabledLinkCount = 0;

            for (const auto& Link : InLinks)
            {
                if (Link.Get_Enable() == ECk_EnableDisable::Disable)
                { continue; }

                ++EnabledLinkCount;
            }

            Hash = DoHash_Scalar(static_cast<double>(EnabledLinkCount), Hash);

            for (const auto& Link : InLinks)
            {
                if (Link.Get_Enable() == ECk_EnableDisable::Disable)
                { continue; }

                Hash = DoHash_Scalar(static_cast<double>(Link.Get_Id()), Hash);
                Hash = DoHash_Vector(Link.Get_Start(), Hash);
                Hash = DoHash_Vector(Link.Get_End(), Hash);
                Hash = DoHash_Scalar(static_cast<double>(static_cast<uint8>(Link.Get_Direction())), Hash);
                Hash = DoHash_Scalar(static_cast<double>(Link.Get_CostMultiplierForward()), Hash);
                Hash = DoHash_Scalar(static_cast<double>(Link.Get_CostMultiplierBackward()), Hash);
                Hash = DoHash_Scalar(static_cast<double>(Link.Get_ClearanceUu()), Hash);
                Hash = DoHash_String(Link.Get_AreaTag().ToString(), Hash);
                Hash = DoHash_String(Link.Get_UserTypeTag().ToString(), Hash);
                Hash = DoHash_Scalar(static_cast<double>(static_cast<uint8>(Link.Get_ProjectionMode())), Hash);
                Hash = DoHash_Scalar(static_cast<double>(Link.Get_ProjectionHorizontalExtentUu()), Hash);
                Hash = DoHash_Scalar(static_cast<double>(Link.Get_ProjectionVerticalExtentUu()), Hash);
            }

            // ---- 7. Merge tunables ---------------------------------------------------------------------
            // Every member, in DECLARATION order, the way the profile above is hashed: the struct is two
            // tolerances today and a member added to it without a line here is the latent staleness bug
            // the enumeration exists to refuse.
            Hash = DoHash_Scalar(InMergeTunables.Get_PlaneFitToleranceUu(), Hash);
            Hash = DoHash_Scalar(InMergeTunables.Get_NormalConeDegrees(), Hash);

            // ---- 8. Clearance cap ----------------------------------------------------------------------
            Hash = DoHash_Scalar(InMaxClearanceUu, Hash);

            // ---- 9. Profile variants, in authored order -------------------------------------------------
            // Sequential in the order the volume lists them, because that is the order it bakes them in;
            // the tag through its NAME for the reason every other tag here is hashed through its name.
            Hash = DoHash_Scalar(static_cast<double>(InVariants.Num()), Hash);

            for (const auto& Variant : InVariants)
            {
                Hash = DoHash_String(Variant.Key.ToString(), Hash);
                Hash = DoHash_Profile(Variant.Value, Hash);
            }

            return Hash;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_GeometryHash(
            const FCk_GroundNav_GeometryBatch& InGeometry)
        -> uint64
    {
        using namespace fingerprint_private;

        auto TriangleHash = uint64{0};
        const auto TriangleCount = InGeometry.Get_TriangleCount();

        for (auto TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
        {
            auto A = FVector::ZeroVector;
            auto B = FVector::ZeroVector;
            auto C = FVector::ZeroVector;
            InGeometry.Get_Triangle(TriangleIndex, A, B, C);

            // Commutative accumulation: submission order cannot matter, duplicates cannot cancel.
            TriangleHash += DoHash_Triangle(A, B, C);
        }

        // The count is folded in separately so a set of triangles that happened to sum to the same value
        // as a different-sized set still separates.
        return DoHash_Scalar(static_cast<double>(TriangleCount), 0x243F6A8885A308D3ULL) ^ TriangleHash;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ContentFingerprint(
            const FCk_GroundNav_GeometryBatch&                        InGeometry,
            const FBox&                                               InRegion,
            const FCk_GroundNav_BakeConfig&                           InConfig,
            const FCk_GroundNav_AgentProfile&                         InProfile,
            TConstArrayView<FCk_GroundNav_MarkupRecord>               InMarkups,
            TConstArrayView<FCk_GroundNav_LinkRecord>                 InLinks,
            const FCk_GroundNav_MergeTunables&                        InMergeTunables,
            float                                                     InMaxClearanceUu,
            TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants)
        -> FCk_GroundNav_ContentFingerprint
    {
        // Item 1 reduced here and the rest decided below: one implementation of the enumeration, and a
        // batch is simply the caller that still has its triangles in hand.
        return Get_ContentFingerprint(Get_GeometryHash(InGeometry), InRegion, InConfig, InProfile,
            InMarkups, InLinks, InMergeTunables, InMaxClearanceUu, InVariants);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_InputFingerprint(
            const FBox&                                               InRegion,
            const FCk_GroundNav_BakeConfig&                           InConfig,
            const FCk_GroundNav_AgentProfile&                         InProfile,
            TConstArrayView<FCk_GroundNav_MarkupRecord>               InMarkups,
            TConstArrayView<FCk_GroundNav_LinkRecord>                 InLinks,
            const FCk_GroundNav_MergeTunables&                        InMergeTunables,
            float                                                     InMaxClearanceUu,
            TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants)
        -> FCk_GroundNav_ContentFingerprint
    {
        using namespace fingerprint_private;

        // No item 1: an unseeded chain over items 2 through 9, which is exactly the full form with the
        // geometry left out.
        constexpr auto NoGeometry = uint64{0};

        return FCk_GroundNav_ContentFingerprint{DoHash_Inputs(NoGeometry, InRegion, InConfig, InProfile,
            InMarkups, InLinks, InMergeTunables, InMaxClearanceUu, InVariants)};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ContentFingerprint(
            uint64                                                    InGeometryHash,
            const FBox&                                               InRegion,
            const FCk_GroundNav_BakeConfig&                           InConfig,
            const FCk_GroundNav_AgentProfile&                         InProfile,
            TConstArrayView<FCk_GroundNav_MarkupRecord>               InMarkups,
            TConstArrayView<FCk_GroundNav_LinkRecord>                 InLinks,
            const FCk_GroundNav_MergeTunables&                        InMergeTunables,
            float                                                     InMaxClearanceUu,
            TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants)
        -> FCk_GroundNav_ContentFingerprint
    {
        using namespace fingerprint_private;

        // Item 1 is the SEED the authored items chain onto, which is what makes this the input
        // fingerprint with the geometry folded in rather than a second enumeration beside it.
        return FCk_GroundNav_ContentFingerprint{DoHash_Inputs(InGeometryHash, InRegion, InConfig,
            InProfile, InMarkups, InLinks, InMergeTunables, InMaxClearanceUu, InVariants)};
    }
}

// --------------------------------------------------------------------------------------------------------------------
