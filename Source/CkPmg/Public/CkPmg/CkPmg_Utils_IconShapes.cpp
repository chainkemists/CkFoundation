#include "CkPmg_Utils_IconShapes.h"

#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    auto
        GenerateDebugShape_Warning(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfSize = InSize * 0.5f;
        const auto Height = InSize * 0.866f;

        // Triangle outline
        const auto Top = FVector(0.0f, 0.0f, Height * 0.667f);
        const auto BottomLeft = FVector(-HalfSize, 0.0f, -Height * 0.333f);
        const auto BottomRight = FVector(HalfSize, 0.0f, -Height * 0.333f);

        // Top face
        Vertices.Add(Top);
        Vertices.Add(BottomRight);
        Vertices.Add(BottomLeft);
        Normals.Add(FVector::ForwardVector);
        Normals.Add(FVector::ForwardVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.0f));
        Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);

        // Bottom face
        const auto BottomStart = Vertices.Num();
        Vertices.Add(Top);
        Vertices.Add(BottomRight);
        Vertices.Add(BottomLeft);
        Normals.Add(FVector::BackwardVector);
        Normals.Add(FVector::BackwardVector);
        Normals.Add(FVector::BackwardVector);
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.0f));
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 2); Triangles.Add(BottomStart + 1);

        // Exclamation mark (vertical bar + dot)
        const auto BarWidth = InSize * 0.1f;
        const auto BarHeight = InSize * 0.4f;
        const auto DotSize = InSize * 0.08f;
        const auto BarBottom = -Height * 0.15f;

        // Bar
        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarBottom + BarHeight));
        Vertices.Add(FVector(-BarWidth, 0.0f, BarBottom + BarHeight));
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
        GenerateDebugShape_Prohibition(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        // Circle background
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        // X mark (two diagonal bars)
        const auto BarWidth = InRadius * 0.15f;
        const auto BarLength = InRadius * 1.3f;

        // First diagonal
        auto Bar1Idx = Vertices.Num();
        const auto Angle1 = PI * 0.25f;
        const auto Perp1 = FVector(-FMath::Sin(Angle1), FMath::Cos(Angle1), 0.0f) * BarWidth;
        const auto Dir1 = FVector(FMath::Cos(Angle1), FMath::Sin(Angle1), 0.0f) * BarLength * 0.5f;

        Vertices.Add(-Dir1 - Perp1);
        Vertices.Add(-Dir1 + Perp1);
        Vertices.Add(Dir1 + Perp1);
        Vertices.Add(Dir1 - Perp1);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(Bar1Idx + 0); Triangles.Add(Bar1Idx + 1); Triangles.Add(Bar1Idx + 2);
        Triangles.Add(Bar1Idx + 0); Triangles.Add(Bar1Idx + 2); Triangles.Add(Bar1Idx + 3);

        // Second diagonal
        auto Bar2Idx = Vertices.Num();
        const auto Angle2 = -PI * 0.25f;
        const auto Perp2 = FVector(-FMath::Sin(Angle2), FMath::Cos(Angle2), 0.0f) * BarWidth;
        const auto Dir2 = FVector(FMath::Cos(Angle2), FMath::Sin(Angle2), 0.0f) * BarLength * 0.5f;

        Vertices.Add(-Dir2 - Perp2);
        Vertices.Add(-Dir2 + Perp2);
        Vertices.Add(Dir2 + Perp2);
        Vertices.Add(Dir2 - Perp2);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(Bar2Idx + 0); Triangles.Add(Bar2Idx + 1); Triangles.Add(Bar2Idx + 2);
        Triangles.Add(Bar2Idx + 0); Triangles.Add(Bar2Idx + 2); Triangles.Add(Bar2Idx + 3);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_NoEntry(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        // Circle background
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        // Horizontal bar
        const auto BarWidth = InRadius * 1.3f;
        const auto BarHeight = InRadius * 0.25f;

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth * 0.5f, -BarHeight, 0.0f));
        Vertices.Add(FVector(BarWidth * 0.5f, -BarHeight, 0.0f));
        Vertices.Add(FVector(BarWidth * 0.5f, BarHeight, 0.0f));
        Vertices.Add(FVector(-BarWidth * 0.5f, BarHeight, 0.0f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_InfoCircle(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        // Circle background
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        // 'i' letter
        const auto DotSize = InRadius * 0.12f;
        const auto DotY = InRadius * 0.3f;
        const auto BarWidth = InRadius * 0.12f;
        const auto BarHeight = InRadius * 0.5f;
        const auto BarTop = -InRadius * 0.05f;

        // Dot
        auto DotIdx = Vertices.Num();
        Vertices.Add(FVector(0.0f, 0.0f, DotY));
        Vertices.Add(FVector(DotSize, 0.0f, DotY - DotSize * 0.5f));
        Vertices.Add(FVector(0.0f, 0.0f, DotY - DotSize));
        Vertices.Add(FVector(-DotSize, 0.0f, DotY - DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        // Bar
        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, BarTop - BarHeight));
        Vertices.Add(FVector(BarWidth, 0.0f, BarTop - BarHeight));
        Vertices.Add(FVector(BarWidth, 0.0f, BarTop));
        Vertices.Add(FVector(-BarWidth, 0.0f, BarTop));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------
