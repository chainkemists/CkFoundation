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
            case ECk_Plane_Axis::XZ: return FQuat(FVector::ForwardVector, PI * 0.5f); break;
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
            Triangles.Add(Next);
            Triangles.Add(Current + 1);

            Triangles.Add(Current + 1);
            Triangles.Add(Next);
            Triangles.Add(Next + 1);
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

        static auto TranslucentMaterial = LoadObject<UMaterial>(
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rot = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rot * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;
                const auto Thickness = InCommon.Get_LineThickness();
                const auto Radius = InParams.Get_Radius();
                const auto ArcSegments = 32;

                // Draw 3 cross-sectional circles (XZ, YZ, and XY planes)
                
                // XZ plane circle
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = 2.0f * PI * j / ArcSegments;
                    const auto Phi2 = 2.0f * PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi1),
                        0.0f,
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi2),
                        0.0f,
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }

                // YZ plane circle
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = 2.0f * PI * j / ArcSegments;
                    const auto Phi2 = 2.0f * PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi1),
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi2),
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }

                // XY plane circle
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = 2.0f * PI * j / ArcSegments;
                    const auto Phi2 = 2.0f * PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi1),
                        Radius * FMath::Sin(Phi1),
                        0.0f));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi2),
                        Radius * FMath::Sin(Phi2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rotation * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                UCk_Utils_DebugDraw_UE::DrawDebugBox(
                    World, Center, InParams.Get_Extent(), LineColor, CombinedRot.Rotator(),
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rot = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rot * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;
                const auto Thickness = InCommon.Get_LineThickness();
                const auto Radius = InParams.Get_Radius();
                const auto Height = InParams.Get_Height();
                const auto Segments = InParams.Get_Segments();
                const auto Apex = Center + CombinedRot.RotateVector(FVector(0, 0, Height));

                // Draw base circle with proper rotation
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }

                // Draw lines from base to apex
                for (auto i = 0; i < Segments; i += FMath::Max(1, Segments / 4))
                {
                    const auto Angle = 2.0f * PI * i / Segments;
                    const auto BasePoint = Center + CombinedRot.RotateVector(
                        FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0f));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BasePoint, Apex, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rot = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rot * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;
                const auto Thickness = InCommon.Get_LineThickness();
                const auto Radius = InParams.Get_Radius();
                const auto HalfHeight = InParams.Get_Height() * 0.5f;
                const auto Segments = InParams.Get_Segments();

                const auto TopCenter = Center + CombinedRot.RotateVector(FVector(0, 0, HalfHeight));
                const auto BottomCenter = Center + CombinedRot.RotateVector(FVector(0, 0, -HalfHeight));

                // Draw top and bottom circles with proper rotation
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    // Top circle
                    const auto TopP1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        HalfHeight));
                    const auto TopP2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        HalfHeight));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TopP1, TopP2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);

                    // Bottom circle
                    const auto BottomP1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        -HalfHeight));
                    const auto BottomP2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        -HalfHeight));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BottomP1, BottomP2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }

                // Draw vertical lines
                for (auto i = 0; i < Segments; i += FMath::Max(1, Segments / 4))
                {
                    const auto Angle = 2.0f * PI * i / Segments;
                    const auto Offset = CombinedRot.RotateVector(
                        FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0f));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(
                        World, TopCenter + Offset, BottomCenter + Offset, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rotation * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                UCk_Utils_DebugDraw_UE::DrawDebugCapsule(
                    World, Center, InParams.Get_HalfHeight(), InParams.Get_Radius(),
                    CombinedRot.Rotator(), LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rot = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rot * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;
                const auto Thickness = InCommon.Get_LineThickness();
                const auto BaseSize = InParams.Get_BaseSize();
                const auto Height = InParams.Get_Height();
                const auto HalfSize = BaseSize * 0.5f;

                const auto BL = Center + CombinedRot.RotateVector(FVector(-HalfSize, -HalfSize, 0.0f));
                const auto BR = Center + CombinedRot.RotateVector(FVector(HalfSize, -HalfSize, 0.0f));
                const auto TR = Center + CombinedRot.RotateVector(FVector(HalfSize, HalfSize, 0.0f));
                const auto TL = Center + CombinedRot.RotateVector(FVector(-HalfSize, HalfSize, 0.0f));
                const auto Apex = Center + CombinedRot.RotateVector(FVector(0, 0, Height));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BL, BR, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BR, TR, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TR, TL, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TL, BL, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BL, Apex, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BR, Apex, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TR, Apex, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TL, Apex, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rot = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rot * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;
                const auto Thickness = InCommon.Get_LineThickness();
                const auto Radius = InParams.Get_Radius();
                const auto ArcSegments = 16;
                const auto Segments = InParams.Get_Segments();

                // XZ plane half-circle (front-back)
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = PI * j / ArcSegments;
                    const auto Phi2 = PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi1),
                        0.0f,
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi2),
                        0.0f,
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }

                // YZ plane half-circle (left-right)
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = PI * j / ArcSegments;
                    const auto Phi2 = PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi1),
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi2),
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }

                // XY plane full circle (base)
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto P1 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + CombinedRot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                }
            }
        }
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rot = Transform.Get_Transform().GetRotation();
                const auto AxisRot = GetAxisRotation(InParams.Get_Axis());
                const auto CombinedRot = Rot * AxisRot;
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;
                const auto Thickness = InCommon.Get_LineThickness();
                const auto MajorRadius = InParams.Get_MajorRadius();
                const auto MinorRadius = InParams.Get_MinorRadius();
                const auto MajorSegments = InParams.Get_MajorSegments();
                const auto MinorSegments = InParams.Get_MinorSegments();

                const auto MajorCirclesToDraw = 8;
                for (auto CircleIdx = 0; CircleIdx < MajorCirclesToDraw; ++CircleIdx)
                {
                    const auto MajorAngle = 2.0f * PI * CircleIdx / MajorCirclesToDraw;
                    const auto MajorCos = FMath::Cos(MajorAngle);
                    const auto MajorSin = FMath::Sin(MajorAngle);

                    const auto TubeCenter = FVector(
                        MajorRadius * MajorCos,
                        MajorRadius * MajorSin,
                        0.0f);

                    for (auto j = 0; j < MinorSegments; ++j)
                    {
                        const auto MinorAngle1 = 2.0f * PI * j / MinorSegments;
                        const auto MinorAngle2 = 2.0f * PI * (j + 1) / MinorSegments;

                        const auto P1 = TubeCenter + FVector(
                            MinorRadius * FMath::Cos(MinorAngle1) * MajorCos,
                            MinorRadius * FMath::Cos(MinorAngle1) * MajorSin,
                            MinorRadius * FMath::Sin(MinorAngle1));

                        const auto P2 = TubeCenter + FVector(
                            MinorRadius * FMath::Cos(MinorAngle2) * MajorCos,
                            MinorRadius * FMath::Cos(MinorAngle2) * MajorSin,
                            MinorRadius * FMath::Sin(MinorAngle2));

                        const auto WorldP1 = Center + CombinedRot.RotateVector(P1);
                        const auto WorldP2 = Center + CombinedRot.RotateVector(P2);

                        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldP1, WorldP2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                    }
                }

                const auto MinorCirclesToDraw = 4;
                for (auto CircleIdx = 0; CircleIdx < MinorCirclesToDraw; ++CircleIdx)
                {
                    const auto MinorAngle = 2.0f * PI * CircleIdx / MinorCirclesToDraw;
                    const auto MinorCos = FMath::Cos(MinorAngle);
                    const auto MinorSin = FMath::Sin(MinorAngle);

                    for (auto i = 0; i < MajorSegments; ++i)
                    {
                        const auto MajorAngle1 = 2.0f * PI * i / MajorSegments;
                        const auto MajorAngle2 = 2.0f * PI * (i + 1) / MajorSegments;

                        const auto P1 = FVector(
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Cos(MajorAngle1),
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Sin(MajorAngle1),
                            MinorRadius * MinorSin);

                        const auto P2 = FVector(
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Cos(MajorAngle2),
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Sin(MajorAngle2),
                            MinorRadius * MinorSin);

                        const auto WorldP1 = Center + CombinedRot.RotateVector(P1);
                        const auto WorldP2 = Center + CombinedRot.RotateVector(P2);

                        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldP1, WorldP2, LineColor, InCommon.Get_Duration().Get_Seconds(), Thickness);
                    }
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
