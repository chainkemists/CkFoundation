#include "CkPmg_Processor_BasicShapes.h"

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
    auto GenerateDebugShape_Sphere(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Sin(Phi);
            const auto RingHeight = InRadius * FMath::Cos(Phi);

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                const auto Vertex = FVector(X, Y, RingHeight);
                Vertices.Add(Vertex);
                Normals.Add(Vertex.GetSafeNormal());
                UVs.Add(FVector2D(
                    static_cast<float>(Segment) / InSegments,
                    static_cast<float>(Ring) / InRings));
            }
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Box(
        UProceduralMeshComponent* InMeshComponent,
        const FVector& InExtent,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const TArray Corners = {
            FVector(-InExtent.X, -InExtent.Y, -InExtent.Z), FVector( InExtent.X, -InExtent.Y, -InExtent.Z),
            FVector( InExtent.X,  InExtent.Y, -InExtent.Z), FVector(-InExtent.X,  InExtent.Y, -InExtent.Z),
            FVector(-InExtent.X, -InExtent.Y,  InExtent.Z), FVector( InExtent.X, -InExtent.Y,  InExtent.Z),
            FVector( InExtent.X,  InExtent.Y,  InExtent.Z), FVector(-InExtent.X,  InExtent.Y,  InExtent.Z)
        };

        struct FFace { TArray<int32> Indices; FVector Normal; };
        const TArray Faces = {
            FFace{{0, 1, 2, 3}, FVector( 0,  0, -1)},
            FFace{{4, 7, 6, 5}, FVector( 0,  0,  1)},
            FFace{{0, 4, 5, 1}, FVector( 0, -1,  0)},
            FFace{{2, 6, 7, 3}, FVector( 0,  1,  0)},
            FFace{{0, 3, 7, 4}, FVector(-1,  0,  0)},
            FFace{{1, 5, 6, 2}, FVector( 1,  0,  0)}
        };

        for (const auto& Face : Faces)
        {
            const auto BaseIndex = Vertices.Num();

            for (const auto Index : Face.Indices)
            {
                Vertices.Add(Corners[Index]);
                Normals.Add(Face.Normal);
            }

            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(1, 1));
            UVs.Add(FVector2D(0, 1));

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 1);
            Triangles.Add(BaseIndex + 2);

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 2);
            Triangles.Add(BaseIndex + 3);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

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

    auto GenerateDebugShape_Cone(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        float InHeight,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto Apex = FVector(0, 0, InHeight);
        Vertices.Add(Apex);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 1.0f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));

            const auto EdgeDir = FVector(X, Y, 0.0f) - Apex;
            const auto Tangent = FVector(-Y, X, 0.0f).GetSafeNormal();
            Normals.Add(FVector::CrossProduct(EdgeDir, Tangent).GetSafeNormal());

            UVs.Add(FVector2D(static_cast<float>(i) / InSegments, 0.0f));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 2);
            Triangles.Add(i + 1);
        }

        const auto BaseCenter = Vertices.Num();
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
            Triangles.Add(BaseCenter);
            Triangles.Add(BaseCenter + i + 1);
            Triangles.Add(BaseCenter + i + 2);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Cylinder(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        float InHeight,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfHeight = InHeight * 0.5f;

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, -HalfHeight));
            Vertices.Add(FVector(X, Y,  HalfHeight));

            const auto Normal = FVector(X, Y, 0.0f).GetSafeNormal();
            Normals.Add(Normal);
            Normals.Add(Normal);

            const auto U = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D(U, 0.0f));
            UVs.Add(FVector2D(U, 1.0f));
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

        const auto BottomCenter = Vertices.Num();
        Vertices.Add(FVector(0, 0, -HalfHeight));
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), -HalfHeight));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BottomCenter);
            Triangles.Add(BottomCenter + i + 1);
            Triangles.Add(BottomCenter + i + 2);
        }

        const auto TopCenter = Vertices.Num();
        Vertices.Add(FVector(0, 0, HalfHeight));
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), HalfHeight));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(TopCenter);
            Triangles.Add(TopCenter + i + 2);
            Triangles.Add(TopCenter + i + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Capsule(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        float InHalfHeight,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto CylinderHeight = InHalfHeight * 2.0f - InRadius * 2.0f;
        const auto HalfCylinderHeight = CylinderHeight * 0.5f;

        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * 0.5f * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Cos(Phi);
            const auto RingHeight = InRadius * FMath::Sin(Phi) + HalfCylinderHeight;

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                Vertices.Add(FVector(X, Y, RingHeight));
                Normals.Add(FVector(X, Y, InRadius * FMath::Sin(Phi)).GetSafeNormal());
                UVs.Add(FVector2D(static_cast<float>(Segment) / InSegments, static_cast<float>(Ring) / (InRings * 2 + 1)));
            }
        }

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, HalfCylinderHeight));
            Vertices.Add(FVector(X, Y, -HalfCylinderHeight));

            const auto Normal = FVector(X, Y, 0.0f).GetSafeNormal();
            Normals.Add(Normal);
            Normals.Add(Normal);

            const auto U = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D(U, static_cast<float>(InRings) / (InRings * 2 + 1)));
            UVs.Add(FVector2D(U, static_cast<float>(InRings + 1) / (InRings * 2 + 1)));
        }

        const auto BottomHemiStart = Vertices.Num();
        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * 0.5f * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Cos(Phi);
            const auto RingHeight = -InRadius * FMath::Sin(Phi) - HalfCylinderHeight;

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                Vertices.Add(FVector(X, Y, RingHeight));
                Normals.Add(FVector(X, Y, -InRadius * FMath::Sin(Phi)).GetSafeNormal());
                UVs.Add(FVector2D(static_cast<float>(Segment) / InSegments, static_cast<float>(InRings + 1 + Ring) / (InRings * 2 + 1)));
            }
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Next);
                Triangles.Add(Current + 1);

                Triangles.Add(Current + 1);
                Triangles.Add(Next);
                Triangles.Add(Next + 1);
            }
        }

        const auto CylinderStart = (InRings + 1) * (InSegments + 1);
        for (auto i = 0; i < InSegments; ++i)
        {
            const auto Current = CylinderStart + i * 2;
            const auto Next = CylinderStart + (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Current + 1);
            Triangles.Add(Next);

            Triangles.Add(Current + 1);
            Triangles.Add(Next + 1);
            Triangles.Add(Next);
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = BottomHemiStart + Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

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

    auto GenerateDebugShape_Pyramid(
        UProceduralMeshComponent* InMeshComponent,
        float InBaseSize,
        float InHeight,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfSize = InBaseSize * 0.5f;
        const auto Apex = FVector(0, 0, InHeight);

        const TArray BaseCorners = {
            FVector(-HalfSize, -HalfSize, 0.0f),
            FVector(HalfSize, -HalfSize, 0.0f),
            FVector(HalfSize, HalfSize, 0.0f),
            FVector(-HalfSize, HalfSize, 0.0f)
        };

        for (auto i = 0; i < 4; ++i)
        {
            const auto Next = (i + 1) % 4;
            const auto BaseIdx = Vertices.Num();

            Vertices.Add(BaseCorners[i]);
            Vertices.Add(BaseCorners[Next]);
            Vertices.Add(Apex);

            const auto EdgeDir = BaseCorners[Next] - BaseCorners[i];
            const auto ToApex = Apex - BaseCorners[i];
            const auto Normal = FVector::CrossProduct(EdgeDir, ToApex).GetSafeNormal();

            Normals.Add(Normal);
            Normals.Add(Normal);
            Normals.Add(Normal);

            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(0.5f, 1));

            Triangles.Add(BaseIdx + 0);
            Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx + 1);
        }

        const auto BaseStart = Vertices.Num();
        for (const auto& Corner : BaseCorners)
        {
            Vertices.Add(Corner);
            Normals.Add(FVector::DownVector);
        }

        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(1, 1));
        UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseStart + 0);
        Triangles.Add(BaseStart + 1);
        Triangles.Add(BaseStart + 2);
        Triangles.Add(BaseStart + 0);
        Triangles.Add(BaseStart + 2);
        Triangles.Add(BaseStart + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Hemisphere(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * 0.5f * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Cos(Phi);
            const auto RingHeight = InRadius * FMath::Sin(Phi);

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                const auto Vertex = FVector(X, Y, RingHeight);
                Vertices.Add(Vertex);
                Normals.Add(Vertex.GetSafeNormal());
                UVs.Add(FVector2D(static_cast<float>(Segment) / InSegments, static_cast<float>(Ring) / InRings));
            }
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Next);
                Triangles.Add(Current + 1);

                Triangles.Add(Current + 1);
                Triangles.Add(Next);
                Triangles.Add(Next + 1);
            }
        }

        const auto BaseCenter = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BaseCenter);
            Triangles.Add(BaseCenter + i + 1);
            Triangles.Add(BaseCenter + i + 2);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Torus(
        UProceduralMeshComponent* InMeshComponent,
        float InMajorRadius,
        float InMinorRadius,
        int32 InMajorSegments,
        int32 InMinorSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto i = 0; i <= InMajorSegments; ++i)
        {
            const auto MajorAngle = 2.0f * PI * i / InMajorSegments;
            const auto MajorCos = FMath::Cos(MajorAngle);
            const auto MajorSin = FMath::Sin(MajorAngle);

            const auto TubeCenter = FVector(InMajorRadius * MajorCos, InMajorRadius * MajorSin, 0.0f);

            for (auto j = 0; j <= InMinorSegments; ++j)
            {
                const auto MinorAngle = 2.0f * PI * j / InMinorSegments;
                const auto MinorCos = FMath::Cos(MinorAngle);
                const auto MinorSin = FMath::Sin(MinorAngle);

                const auto Vertex = TubeCenter + FVector(
                    InMinorRadius * MinorCos * MajorCos,
                    InMinorRadius * MinorCos * MajorSin,
                    InMinorRadius * MinorSin);

                Vertices.Add(Vertex);

                const auto Normal = FVector(MinorCos * MajorCos, MinorCos * MajorSin, MinorSin);
                Normals.Add(Normal.GetSafeNormal());

                UVs.Add(FVector2D(static_cast<float>(i) / InMajorSegments, static_cast<float>(j) / InMinorSegments));
            }
        }

        for (auto i = 0; i < InMajorSegments; ++i)
        {
            for (auto j = 0; j < InMinorSegments; ++j)
            {
                const auto Current = i * (InMinorSegments + 1) + j;
                const auto Next = (i + 1) * (InMinorSegments + 1) + j;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

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

    auto FinalizeMeshComponent(
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

        auto DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
        if (ck::IsValid(DefaultMaterial))
        {
            InMeshComponent->SetMaterial(0, DefaultMaterial);
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
    auto FProcessor_Pmg_Sphere_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Sphere_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Sphere(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_Rings(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Box_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Box_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Box(MeshComponent, InParams.Get_Extent(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Cone_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Cone_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Cone(MeshComponent, InParams.Get_Radius(), InParams.Get_Height(), InParams.Get_Segments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Cylinder_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Cylinder_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Cylinder(MeshComponent, InParams.Get_Radius(), InParams.Get_Height(), InParams.Get_Segments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Capsule_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Capsule_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Capsule(MeshComponent, InParams.Get_Radius(), InParams.Get_HalfHeight(), InParams.Get_Segments(), InParams.Get_Rings(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Pyramid_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Pyramid_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Pyramid(MeshComponent, InParams.Get_BaseSize(), InParams.Get_Height(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Hemisphere_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Hemisphere_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Hemisphere(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_Rings(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }

    auto FProcessor_Pmg_Torus_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Torus_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Torus(MeshComponent, InParams.Get_MajorRadius(), InParams.Get_MinorRadius(), InParams.Get_MajorSegments(), InParams.Get_MinorSegments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    }
}

// --------------------------------------------------------------------------------------------------------------------
