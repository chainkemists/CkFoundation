#include "CkPmg_Utils_SymbolShapes.h"

#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    auto
        GenerateDebugShape_MagnifyingGlass(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto LensRadius = InSize * 0.4f;
        const auto HandleLength = InSize * 0.5f;
        const auto HandleWidth = InSize * 0.1f;
        const auto Segments = 32;

        // Lens (circle)
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= Segments; ++i)
        {
            const auto Angle = 2.0f * PI * i / Segments;
            Vertices.Add(FVector(LensRadius * FMath::Cos(Angle), LensRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < Segments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        // Handle (angled bar)
        const auto HandleAngle = -PI * 0.25f;
        const auto HandleStart = FVector(
            LensRadius * FMath::Cos(HandleAngle),
            LensRadius * FMath::Sin(HandleAngle),
            0.0f);
        const auto HandleEnd = HandleStart + FVector(
            HandleLength * FMath::Cos(HandleAngle),
            HandleLength * FMath::Sin(HandleAngle),
            0.0f);

        const auto HandlePerp = FVector(-FMath::Sin(HandleAngle), FMath::Cos(HandleAngle), 0.0f) * HandleWidth * 0.5f;

        auto HandleIdx = Vertices.Num();
        Vertices.Add(HandleStart - HandlePerp);
        Vertices.Add(HandleStart + HandlePerp);
        Vertices.Add(HandleEnd + HandlePerp);
        Vertices.Add(HandleEnd - HandlePerp);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(HandleIdx + 0); Triangles.Add(HandleIdx + 1); Triangles.Add(HandleIdx + 2);
        Triangles.Add(HandleIdx + 0); Triangles.Add(HandleIdx + 2); Triangles.Add(HandleIdx + 3);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_QuestionMark(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto CurveRadius = InSize * 0.25f;
        const auto Thickness = InSize * 0.12f;
        const auto Segments = 16;

        // Top curve (semicircle)
        const auto CurveCenter = FVector(0.0f, 0.0f, InSize * 0.15f);
        for (auto i = 0; i <= Segments; ++i)
        {
            const auto T = static_cast<float>(i) / Segments;
            const auto Angle = PI * T;
            const auto Inner = CurveCenter + FVector(CurveRadius * FMath::Cos(Angle), 0.0f, CurveRadius * FMath::Sin(Angle));
            const auto Outer = CurveCenter + FVector((CurveRadius + Thickness) * FMath::Cos(Angle), 0.0f, (CurveRadius + Thickness) * FMath::Sin(Angle));

            if (i < Segments)
            {
                const auto BaseIdx = Vertices.Num();
                Vertices.Add(Inner);
                Vertices.Add(Outer);
                Normals.Add(FVector::ForwardVector);
                Normals.Add(FVector::ForwardVector);
                UVs.Add(FVector2D(T, 0));
                UVs.Add(FVector2D(T, 1));

                if (i > 0)
                {
                    Triangles.Add(BaseIdx - 2); Triangles.Add(BaseIdx - 1); Triangles.Add(BaseIdx + 1);
                    Triangles.Add(BaseIdx - 2); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 0);
                }
            }
        }

        // Vertical stem
        const auto StemTop = CurveCenter + FVector(0.0f, 0.0f, -CurveRadius);
        const auto StemBottom = StemTop + FVector(0.0f, 0.0f, -InSize * 0.2f);
        const auto HalfThick = Thickness * 0.5f;

        auto StemIdx = Vertices.Num();
        Vertices.Add(StemTop + FVector(-HalfThick, 0.0f, 0.0f));
        Vertices.Add(StemTop + FVector(HalfThick, 0.0f, 0.0f));
        Vertices.Add(StemBottom + FVector(HalfThick, 0.0f, 0.0f));
        Vertices.Add(StemBottom + FVector(-HalfThick, 0.0f, 0.0f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(StemIdx + 0); Triangles.Add(StemIdx + 1); Triangles.Add(StemIdx + 2);
        Triangles.Add(StemIdx + 0); Triangles.Add(StemIdx + 2); Triangles.Add(StemIdx + 3);

        // Dot
        const auto DotSize = InSize * 0.08f;
        const auto DotCenter = StemBottom + FVector(0.0f, 0.0f, -DotSize * 1.5f);

        auto DotIdx = Vertices.Num();
        Vertices.Add(DotCenter);
        Vertices.Add(DotCenter + FVector(DotSize, 0.0f, -DotSize * 0.5f));
        Vertices.Add(DotCenter + FVector(0.0f, 0.0f, -DotSize));
        Vertices.Add(DotCenter + FVector(-DotSize, 0.0f, -DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_ExclamationMark(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto BarWidth = InSize * 0.12f;
        const auto BarHeight = InSize * 0.6f;
        const auto DotSize = InSize * 0.1f;

        // Vertical bar
        const auto BarTop = InSize * 0.35f;
        const auto BarBottom = BarTop - BarHeight;

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarTop));
        Vertices.Add(FVector(-BarWidth, 0.0f, BarTop));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        // Dot (small diamond)
        const auto DotCenter = BarBottom - DotSize * 1.5f;
        auto DotIdx = Vertices.Num();
        Vertices.Add(FVector(0.0f, 0.0f, DotCenter));
        Vertices.Add(FVector(DotSize, 0.0f, DotCenter - DotSize * 0.5f));
        Vertices.Add(FVector(0.0f, 0.0f, DotCenter - DotSize));
        Vertices.Add(FVector(-DotSize, 0.0f, DotCenter - DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Flag(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto PoleRadius = InSize * 0.04f;
        const auto PoleHeight = InSize;
        const auto FlagWidth = InSize * 0.6f;
        const auto FlagHeight = InSize * 0.4f;

        // Pole (thin cylinder)
        auto PoleIdx = Vertices.Num();
        Vertices.Add(FVector(-PoleRadius, 0.0f, 0.0f));
        Vertices.Add(FVector(PoleRadius, 0.0f, 0.0f));
        Vertices.Add(FVector(PoleRadius, 0.0f, PoleHeight));
        Vertices.Add(FVector(-PoleRadius, 0.0f, PoleHeight));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(PoleIdx + 0); Triangles.Add(PoleIdx + 1); Triangles.Add(PoleIdx + 2);
        Triangles.Add(PoleIdx + 0); Triangles.Add(PoleIdx + 2); Triangles.Add(PoleIdx + 3);

        // Flag (triangle)
        const auto FlagBase = PoleHeight * 0.7f;
        auto FlagIdx = Vertices.Num();
        Vertices.Add(FVector(PoleRadius, 0.0f, FlagBase + FlagHeight));
        Vertices.Add(FVector(FlagWidth, 0.0f, FlagBase + FlagHeight * 0.5f));
        Vertices.Add(FVector(PoleRadius, 0.0f, FlagBase));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0, 0));
        Triangles.Add(FlagIdx + 0); Triangles.Add(FlagIdx + 1); Triangles.Add(FlagIdx + 2);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Pin(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HeadRadius = InSize * 0.3f;
        const auto ShaftRadius = InSize * 0.05f;
        const auto ShaftLength = InSize * 0.6f;
        const auto Segments = 16;

        // Head (circle)
        Vertices.Add(FVector(0.0f, 0.0f, InSize * 0.5f));
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= Segments; ++i)
        {
            const auto Angle = 2.0f * PI * i / Segments;
            const auto X = HeadRadius * FMath::Cos(Angle);
            const auto Y = HeadRadius * FMath::Sin(Angle);
            Vertices.Add(FVector(X, Y, InSize * 0.5f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < Segments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        // Shaft (thin rectangle)
        const auto ShaftTop = InSize * 0.5f - HeadRadius;
        const auto ShaftBottom = ShaftTop - ShaftLength;

        auto ShaftIdx = Vertices.Num();
        Vertices.Add(FVector(-ShaftRadius, 0.0f, ShaftTop));
        Vertices.Add(FVector(ShaftRadius, 0.0f, ShaftTop));
        Vertices.Add(FVector(ShaftRadius, 0.0f, ShaftBottom));
        Vertices.Add(FVector(-ShaftRadius, 0.0f, ShaftBottom));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(ShaftIdx + 0); Triangles.Add(ShaftIdx + 1); Triangles.Add(ShaftIdx + 2);
        Triangles.Add(ShaftIdx + 0); Triangles.Add(ShaftIdx + 2); Triangles.Add(ShaftIdx + 3);

        // Point (small triangle)
        auto PointIdx = Vertices.Num();
        Vertices.Add(FVector(-ShaftRadius, 0.0f, ShaftBottom));
        Vertices.Add(FVector(ShaftRadius, 0.0f, ShaftBottom));
        Vertices.Add(FVector(0.0f, 0.0f, ShaftBottom - ShaftRadius * 2.0f));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0.5f, 0));
        Triangles.Add(PointIdx + 0); Triangles.Add(PointIdx + 1); Triangles.Add(PointIdx + 2);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------
