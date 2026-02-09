#include "CkPmg_Processor_FlatShapes.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"

#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------
// Axis Rotation Helpers
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GetAxisRotation(ECk_Plane_Axis InAxis) -> FQuat
    {
        switch (InAxis)
        {
            case ECk_Plane_Axis::XY: return FQuat::Identity;
            case ECk_Plane_Axis::XZ: return FQuat(FVector::ForwardVector, PI * 0.5f);
            case ECk_Plane_Axis::YZ: return FQuat(FVector::RightVector, -PI * 0.5f);
            default: return FQuat::Identity;
        }
    }

    auto ApplyAxisRotation(
        TArray<FVector>& InOutVertices,
        TArray<FVector>& InOutNormals,
        ECk_Plane_Axis InAxis)
        -> void
    {
        if (InAxis == ECk_Plane_Axis::XY)
        { return; }

        const auto Rotation = GetAxisRotation(InAxis);

        for (auto& Vertex : InOutVertices)
        {
            Vertex = Rotation.RotateVector(Vertex);
        }

        for (auto& Normal : InOutNormals)
        {
            Normal = Rotation.RotateVector(Normal);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Shape Generation Functions
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GenerateDebugShape_Circle(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        bool InDrawDirectionLine,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(i + 2);
        }

        const auto BottomCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BottomCenterIndex);
            Triangles.Add(BottomCenterIndex + i + 2);
            Triangles.Add(BottomCenterIndex + i + 1);
        }

        if (InDrawDirectionLine)
        {
            const auto LineThickness = InRadius * 0.02f;
            const auto DirectionStartIndex = Vertices.Num();

            const auto P1 = FVector(0.0f, -LineThickness, 0.0f);
            const auto P2 = FVector(0.0f, LineThickness, 0.0f);
            const auto P3 = FVector(InRadius, LineThickness, 0.0f);
            const auto P4 = FVector(InRadius, -LineThickness, 0.0f);

            Vertices.Add(P1); Vertices.Add(P2); Vertices.Add(P3); Vertices.Add(P4);
            for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::UpVector); }
            UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(1, 0));

            Triangles.Add(DirectionStartIndex + 0); Triangles.Add(DirectionStartIndex + 1); Triangles.Add(DirectionStartIndex + 2);
            Triangles.Add(DirectionStartIndex + 0); Triangles.Add(DirectionStartIndex + 2); Triangles.Add(DirectionStartIndex + 3);

            Vertices.Add(P1); Vertices.Add(P2); Vertices.Add(P3); Vertices.Add(P4);
            for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::DownVector); }
            UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(1, 0));

            const auto BottomDirectionStartIndex = DirectionStartIndex + 4;
            Triangles.Add(BottomDirectionStartIndex + 0); Triangles.Add(BottomDirectionStartIndex + 2); Triangles.Add(BottomDirectionStartIndex + 1);
            Triangles.Add(BottomDirectionStartIndex + 0); Triangles.Add(BottomDirectionStartIndex + 3); Triangles.Add(BottomDirectionStartIndex + 2);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Triangle(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto Radius = InSize * 0.5f;
        const auto Height = InSize * 0.866f;
        const auto CenterY = -Height / 3.0f;

        const auto V1 = FVector(0.0f, Radius + CenterY, 0.0f);
        const auto V2 = FVector(-Radius * 0.866f, -Radius * 0.5f + CenterY, 0.0f);
        const auto V3 = FVector(Radius * 0.866f, -Radius * 0.5f + CenterY, 0.0f);

        // Top face
        Vertices.Add(V1);
        Vertices.Add(V2);
        Vertices.Add(V3);

        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::UpVector); }

        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(0.0f, 0.0f));
        UVs.Add(FVector2D(1.0f, 0.0f));

        Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);

        // Bottom face
        const auto BottomStart = Vertices.Num();
        Vertices.Add(V1);
        Vertices.Add(V2);
        Vertices.Add(V3);

        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::DownVector); }

        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(0.0f, 0.0f));
        UVs.Add(FVector2D(1.0f, 0.0f));

        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 2); Triangles.Add(BottomStart + 1);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Plane(
        UProceduralMeshComponent* InMeshComponent,
        float InWidth,
        float InHeight,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfWidth = InWidth * 0.5f;
        const auto HalfHeight = InHeight * 0.5f;

        Vertices.Add(FVector(-HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, HalfHeight, 0.0f));
        Vertices.Add(FVector(-HalfWidth, HalfHeight, 0.0f));

        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::UpVector); }

        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(1, 1));
        UVs.Add(FVector2D(0, 1));

        Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);
        Triangles.Add(0); Triangles.Add(2); Triangles.Add(3);

        const auto BottomStart = Vertices.Num();
        Vertices.Add(FVector(-HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, HalfHeight, 0.0f));
        Vertices.Add(FVector(-HalfWidth, HalfHeight, 0.0f));

        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::DownVector); }

        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(1, 1));
        UVs.Add(FVector2D(0, 1));

        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 2); Triangles.Add(BottomStart + 1);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 3); Triangles.Add(BottomStart + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Ring(
        UProceduralMeshComponent* InMeshComponent,
        float InInnerRadius,
        float InOuterRadius,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            Vertices.Add(FVector(InOuterRadius * Cos, InOuterRadius * Sin, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * Cos, 0.5f + 0.5f * Sin));

            Vertices.Add(FVector(InInnerRadius * Cos, InInnerRadius * Sin, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Cos, 0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Sin));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            const auto Current = i * 2;
            const auto Next = (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Current + 1);
            Triangles.Add(Next);

            Triangles.Add(Current + 1);
            Triangles.Add(Next + 1);
            Triangles.Add(Next);
        }

        const auto BottomStartIdx = Vertices.Num();
        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            Vertices.Add(FVector(InOuterRadius * Cos, InOuterRadius * Sin, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * Cos, 0.5f + 0.5f * Sin));

            Vertices.Add(FVector(InInnerRadius * Cos, InInnerRadius * Sin, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Cos, 0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Sin));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            const auto Current = BottomStartIdx + i * 2;
            const auto Next = BottomStartIdx + (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Next);
            Triangles.Add(Current + 1);

            Triangles.Add(Current + 1);
            Triangles.Add(Next);
            Triangles.Add(Next + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Cross(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        float InThickness,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfThickness = InThickness * 0.5f;

        // Horizontal bar (along X axis in XY plane)
        const auto HorizTL = FVector(-InSize, -HalfThickness, 0.0f);
        const auto HorizTR = FVector( InSize, -HalfThickness, 0.0f);
        const auto HorizBR = FVector( InSize,  HalfThickness, 0.0f);
        const auto HorizBL = FVector(-InSize,  HalfThickness, 0.0f);

        // Vertical bar (along Y axis in XY plane)
        const auto VertTL = FVector(-HalfThickness, -InSize, 0.0f);
        const auto VertTR = FVector( HalfThickness, -InSize, 0.0f);
        const auto VertBR = FVector( HalfThickness,  InSize, 0.0f);
        const auto VertBL = FVector(-HalfThickness,  InSize, 0.0f);

        // Horizontal bar front face
        auto BaseIdx = Vertices.Num();
        Vertices.Add(HorizTL); Vertices.Add(HorizTR); Vertices.Add(HorizBR); Vertices.Add(HorizBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        // Horizontal bar back face
        BaseIdx = Vertices.Num();
        Vertices.Add(HorizTL); Vertices.Add(HorizTR); Vertices.Add(HorizBR); Vertices.Add(HorizBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::BackwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 3); Triangles.Add(BaseIdx + 2);

        // Vertical bar front face
        BaseIdx = Vertices.Num();
        Vertices.Add(VertTL); Vertices.Add(VertTR); Vertices.Add(VertBR); Vertices.Add(VertBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        // Vertical bar back face
        BaseIdx = Vertices.Num();
        Vertices.Add(VertTL); Vertices.Add(VertTR); Vertices.Add(VertBR); Vertices.Add(VertBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::BackwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 3); Triangles.Add(BaseIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Star(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto InnerRadius = InRadius * InInnerRadiusRatio;

        // Top face
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Angle = 2.0f * PI * i / (InPoints * 2);
            const auto Radius = (i % 2 == 0) ? InRadius : InnerRadius;
            const auto X = Radius * FMath::Cos(Angle - PI * 0.5f);
            const auto Y = Radius * FMath::Sin(Angle - PI * 0.5f);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * X / InRadius,
                0.5f + 0.5f * Y / InRadius));
        }

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Next = (i + 1) % (InPoints * 2);
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(Next + 1);
        }

        // Bottom face
        const auto BottomCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Angle = 2.0f * PI * i / (InPoints * 2);
            const auto Radius = (i % 2 == 0) ? InRadius : InnerRadius;
            const auto X = Radius * FMath::Cos(Angle - PI * 0.5f);
            const auto Y = Radius * FMath::Sin(Angle - PI * 0.5f);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * X / InRadius,
                0.5f + 0.5f * Y / InRadius));
        }

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Next = (i + 1) % (InPoints * 2);
            Triangles.Add(BottomCenterIndex);
            Triangles.Add(BottomCenterIndex + Next + 1);
            Triangles.Add(BottomCenterIndex + i + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        GenerateDebugShape_Checkmark(
            UProceduralMeshComponent* InMeshComponent,
            float InSize,
            float InThickness,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto V = TArray<FVector>{};
        auto T = TArray<int32>{};
        auto N = TArray<FVector>{};
        auto UV = TArray<FVector2D>{};

        const float Half = InThickness * 0.5f;

        // Points in local XY (Z = 0)

        const auto Rotation = FRotator{180.0f, 0, 0};
        const FVector P0 = Rotation.RotateVector(FVector(-InSize * 0.2f, -InSize * 0.5f, 0.f));      // Start
        const FVector P1 = Rotation.RotateVector(FVector( InSize * 0.3f, -InSize * 0.1f, 0.f));      // Junction
        const FVector P2 = Rotation.RotateVector(FVector(-InSize * 0.3f,  InSize * 0.6f, 0.f));      // End

        // Clean 2D perpendicular in XY plane
        auto PerpXY = [&](const FVector& Dir)
        {
            FVector d = Dir;
            d.Z = 0.f;
            d.Normalize();
            return FVector(-d.Y, d.X, 0.f).GetSafeNormal();
        };

        const FVector D0 = (P1 - P0).GetSafeNormal();
        const FVector D1 = (P2 - P1).GetSafeNormal();

        const FVector Pp0 = PerpXY(D0);
        const FVector Pp1 = PerpXY(D1);

        // Junction should use average perpendicular (avoids gap)
        FVector PJ = (Pp0 + Pp1);
        if (PJ.IsNearlyZero())
            PJ = Pp0; // fallback if opposite
        PJ.Normalize();

        // Create quad at stroke start (P0->P1)
        const FVector A0 = P0 - Pp0 * Half;
        const FVector A1 = P0 + Pp0 * Half;
        const FVector B0 = P1 - PJ   * Half;
        const FVector B1 = P1 + PJ   * Half;

        // Create quad at stroke end (P1->P2)
        const FVector C0 = P1 - PJ   * Half;   // shared with B0
        const FVector C1 = P1 + PJ   * Half;   // shared with B1
        const FVector D0p= P2 - Pp1  * Half;
        const FVector D1p= P2 + Pp1  * Half;

        auto AddQuad = [&](const FVector& Q0, const FVector& Q1, const FVector& Q2, const FVector& Q3)
        {
            const int b = V.Num();
            V.Add(Q0); V.Add(Q1); V.Add(Q2); V.Add(Q3);
            UV.Add(FVector2D(0, 0)); UV.Add(FVector2D(1, 0)); UV.Add(FVector2D(1, 1)); UV.Add(FVector2D(0, 1));

            // All normals up in local XY
            N.Add(FVector::UpVector); N.Add(FVector::UpVector);
            N.Add(FVector::UpVector); N.Add(FVector::UpVector);

            // 2 tris
            T.Add(b + 0); T.Add(b + 1); T.Add(b + 2);
            T.Add(b + 0); T.Add(b + 2); T.Add(b + 3);
        };

        AddQuad(A0, A1, B1, B0);      // short stroke
        AddQuad(C0, C1, D1p, D0p);    // long stroke

        // Rotate into desired axis like your other shapes
        ApplyAxisRotation(V, N, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, V, T, N, UV,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Diamond(
        UProceduralMeshComponent* InMeshComponent,
        float InWidth,
        float InHeight,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfWidth = InWidth * 0.5f;
        const auto HalfHeight = InHeight * 0.5f;

        // Top face
        Vertices.Add(FVector::ZeroVector);
        Vertices.Add(FVector(0.0f, HalfWidth, 0.0f));
        Vertices.Add(FVector(HalfHeight, 0.0f, 0.0f));
        Vertices.Add(FVector(0.0f, -HalfWidth, 0.0f));
        Vertices.Add(FVector(-HalfHeight, 0.0f, 0.0f));

        for (auto i = 0; i < 5; ++i)
        {
            Normals.Add(FVector::UpVector);
        }

        UVs.Add(FVector2D(0.5f, 0.5f));
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.5f));
        UVs.Add(FVector2D(0.5f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.5f));

        Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);
        Triangles.Add(0); Triangles.Add(2); Triangles.Add(3);
        Triangles.Add(0); Triangles.Add(3); Triangles.Add(4);
        Triangles.Add(0); Triangles.Add(4); Triangles.Add(1);

        // Bottom face
        const auto BottomStart = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Vertices.Add(FVector(0.0f, HalfWidth, 0.0f));
        Vertices.Add(FVector(HalfHeight, 0.0f, 0.0f));
        Vertices.Add(FVector(0.0f, -HalfWidth, 0.0f));
        Vertices.Add(FVector(-HalfHeight, 0.0f, 0.0f));

        for (auto i = 0; i < 5; ++i)
        {
            Normals.Add(FVector::DownVector);
        }

        UVs.Add(FVector2D(0.5f, 0.5f));
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.5f));
        UVs.Add(FVector2D(0.5f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.5f));

        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 2); Triangles.Add(BottomStart + 1);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 3); Triangles.Add(BottomStart + 2);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 4); Triangles.Add(BottomStart + 3);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 1); Triangles.Add(BottomStart + 4);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Common Setup Helper
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto SetupMeshComponent(
        FCk_Handle_Pmg_DebugShape InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> UProceduralMeshComponent*
    {
        InHandle.Remove<ck::FTag_Pmg_DebugShape_NeedsSetup>();

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World, UProceduralMeshComponent::StaticClass(), NAME_None, RF_Transient);

        if (ck::Is_NOT_Valid(MeshComponent))
        { return nullptr; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(false);

        return MeshComponent;
    }

    auto
    FinalizeMeshComponent(
        UProceduralMeshComponent* InMeshComponent,
        FCk_Handle_Pmg_DebugShape InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> void
    {
        const auto ShouldBeVisible = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        InMeshComponent->SetVisibility(ShouldBeVisible, true);
        InMeshComponent->SetHiddenInGame(!ShouldBeVisible);

        InMeshComponent->SetCollisionEnabled(
            InCommon.Get_EnableCollision() ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

        auto TranslucentMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));

        if (ck::IsValid(TranslucentMaterial))
        {
            auto DynamicMaterial = UMaterialInstanceDynamic::Create(TranslucentMaterial, InMeshComponent);
            if (ck::IsValid(DynamicMaterial))
            {
                DynamicMaterial->SetVectorParameterValue(FName("Color"), InCommon.Get_Color());
                InMeshComponent->SetMaterial(0, DynamicMaterial);
            }
        }
        else
        {
            ck::pmg::Warning(TEXT("Failed to load M_SimpleUnlitTranslucent for Pmg DebugShape [{}]"), InHandle);
        }

        InMeshComponent->UpdateBounds();
        InMeshComponent->MarkRenderStateDirty();

        InCurrent = ck::FFragment_Pmg_DebugShape_Current{TStrongObjectPtr{InMeshComponent}, FCk_Time{InDeltaT}};

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            InMeshComponent->SetWorldTransform(Transform.Get_Transform());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Processor Implementations
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Pmg_Circle_Setup::
        ForEachEntity(
            TimeType InDeltaT, HandleType InHandle,
            const FFragment_Pmg_Circle_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Circle(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_DrawDirectionLine(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                    World, Center, InParams.Get_Radius(), InParams.Get_Axis(),
                    InParams.Get_Segments(), LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                if (InParams.Get_DrawDirectionLine())
                {
                    auto AxisRotation = FQuat::Identity;
                    switch (InParams.Get_Axis())
                    {
                        case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                        case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                        case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                    }

                    const auto Rotation = Transform.Get_Transform().GetRotation();
                    const auto FinalRotation = Rotation * AxisRotation;
                    const auto Direction = FinalRotation.RotateVector(FVector::ForwardVector);
                    const auto End = Center + Direction * InParams.Get_Radius();
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, End, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                }
            }
        }
    }

    auto FProcessor_Pmg_Triangle_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Triangle_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Triangle(MeshComponent, InParams.Get_Size(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto Radius = Size * 0.5f;
                const auto Height = Size * 0.866f;
                const auto CenterY = -Height / 3.0f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                const auto V1Local = FVector(0.0f, Radius + CenterY, 0.0f);
                const auto V2Local = FVector(-Radius * 0.866f, -Radius * 0.5f + CenterY, 0.0f);
                const auto V3Local = FVector(Radius * 0.866f, -Radius * 0.5f + CenterY, 0.0f);

                const auto V1 = Center + FinalRotation.RotateVector(V1Local);
                const auto V2 = Center + FinalRotation.RotateVector(V2Local);
                const auto V3 = Center + FinalRotation.RotateVector(V3Local);

                UCk_Utils_DebugDraw_UE::DrawDebugTriangle(World, V1, V2, V3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Plane_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Plane_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Plane(MeshComponent, InParams.Get_Width(), InParams.Get_Height(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto HalfWidth = InParams.Get_Width() * 0.5f;
                const auto HalfHeight = InParams.Get_Height() * 0.5f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                TArray<FVector> Corners;
                Corners.Add(Center + FinalRotation.RotateVector(FVector(-HalfWidth, -HalfHeight, 0.0f)));
                Corners.Add(Center + FinalRotation.RotateVector(FVector(HalfWidth, -HalfHeight, 0.0f)));
                Corners.Add(Center + FinalRotation.RotateVector(FVector(HalfWidth, HalfHeight, 0.0f)));
                Corners.Add(Center + FinalRotation.RotateVector(FVector(-HalfWidth, HalfHeight, 0.0f)));

                UCk_Utils_DebugDraw_UE::DrawDebugPolygon(World, Corners, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness(), true);
            }
        }
    }

    auto FProcessor_Pmg_Ring_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Ring_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Ring(MeshComponent, InParams.Get_InnerRadius(), InParams.Get_OuterRadius(), InParams.Get_Segments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                    World, Center, InParams.Get_OuterRadius(), InParams.Get_Axis(),
                    InParams.Get_Segments(), LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                    World, Center, InParams.Get_InnerRadius(), InParams.Get_Axis(),
                    InParams.Get_Segments(), LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Cross_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Cross_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Cross(MeshComponent, InParams.Get_Size(), InParams.Get_Thickness(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto Thickness = InParams.Get_Thickness();
                const auto HalfThick = Thickness * 0.5f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                // Horizontal bar (along X in XY plane)
                const auto HX1 = Center + FinalRotation.RotateVector(FVector(-Size, -HalfThick, 0.0f));
                const auto HX2 = Center + FinalRotation.RotateVector(FVector( Size, -HalfThick, 0.0f));
                const auto HX3 = Center + FinalRotation.RotateVector(FVector( Size,  HalfThick, 0.0f));
                const auto HX4 = Center + FinalRotation.RotateVector(FVector(-Size,  HalfThick, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HX1, HX2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HX2, HX3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HX3, HX4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HX4, HX1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                // Vertical bar (along Y in XY plane)
                const auto VY1 = Center + FinalRotation.RotateVector(FVector(-HalfThick, -Size, 0.0f));
                const auto VY2 = Center + FinalRotation.RotateVector(FVector( HalfThick, -Size, 0.0f));
                const auto VY3 = Center + FinalRotation.RotateVector(FVector( HalfThick,  Size, 0.0f));
                const auto VY4 = Center + FinalRotation.RotateVector(FVector(-HalfThick,  Size, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VY1, VY2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VY2, VY3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VY3, VY4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VY4, VY1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Star_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Star_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Star(MeshComponent, InParams.Get_OuterRadius(), InParams.Get_Points(), InParams.Get_InnerRadiusRatio(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Radius = InParams.Get_OuterRadius();
                const auto Points = InParams.Get_Points();
                const auto InnerRatio = InParams.Get_InnerRadiusRatio();
                const auto InnerRadius = Radius * InnerRatio;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                for (auto i = 0; i < Points * 2; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / (Points * 2);
                    const auto Angle2 = 2.0f * PI * (i + 1) / (Points * 2);
                    const auto R1 = (i % 2 == 0) ? Radius : InnerRadius;
                    const auto R2 = ((i + 1) % 2 == 0) ? Radius : InnerRadius;

                    const auto P1 = Center + FinalRotation.RotateVector(FVector(
                        R1 * FMath::Cos(Angle1 - PI * 0.5f),
                        R1 * FMath::Sin(Angle1 - PI * 0.5f),
                        0.0f));
                    const auto P2 = Center + FinalRotation.RotateVector(FVector(
                        R2 * FMath::Cos(Angle2 - PI * 0.5f),
                        R2 * FMath::Sin(Angle2 - PI * 0.5f),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                }
            }
        }
    }

    auto FProcessor_Pmg_Checkmark_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Checkmark_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Checkmark(MeshComponent, InParams.Get_Size(), InParams.Get_Thickness(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();

                // Keep the same axis mapping as your mesh code
                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                // Apply AxisRotation first, then actor/world rotation
                const auto FinalRotation = Rotation * AxisRotation;

                // Use the exact same local points the mesh used (same signs and values)
                const auto FlipRotation = FRotator{180.0f, 0, 0};
                const auto ShortStart = FlipRotation.RotateVector(FVector(-Size * 0.2f, -Size * 0.5f, 0.0f));
                const auto ShortEnd   = FlipRotation.RotateVector(FVector( Size * 0.3f, -Size * 0.1f, 0.0f));
                const auto LongEnd    = FlipRotation.RotateVector(FVector(-Size * 0.3f,  Size * 0.6f, 0.0f));

                // Rotate to world and offset by actor center
                const auto WorldShortStart = Center + FinalRotation.RotateVector(ShortStart);
                const auto WorldShortEnd   = Center + FinalRotation.RotateVector(ShortEnd);
                const auto WorldLongEnd    = Center + FinalRotation.RotateVector(LongEnd);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldShortStart, WorldShortEnd, LineColor,
                                                      InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldShortEnd, WorldLongEnd, LineColor,
                                                      InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Diamond_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Diamond_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Diamond(MeshComponent, InParams.Get_Width(), InParams.Get_Height(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Width = InParams.Get_Width();
                const auto Height = InParams.Get_Height();
                const auto HalfW = Width * 0.5f;
                const auto HalfH = Height * 0.5f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                const auto Top = Center + FinalRotation.RotateVector(FVector(0.0f, HalfW, 0.0f));
                const auto Right = Center + FinalRotation.RotateVector(FVector(HalfH, 0.0f, 0.0f));
                const auto Bottom = Center + FinalRotation.RotateVector(FVector(0.0f, -HalfW, 0.0f));
                const auto Left = Center + FinalRotation.RotateVector(FVector(-HalfH, 0.0f, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Top, Right, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Right, Bottom, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Bottom, Left, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Left, Top, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
