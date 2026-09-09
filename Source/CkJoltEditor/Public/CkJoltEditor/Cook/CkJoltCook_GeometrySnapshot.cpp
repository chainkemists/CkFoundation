#include "CkJoltCook_GeometrySnapshot.h"

#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"

#include <GameFramework/Actor.h>
#include <Algo/Unique.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>

namespace ck_jolt_cook_geometry_snapshot
{
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    constexpr auto TriangleBatchSize = 256;
    static_assert(TriangleBatchSize >= JPH::Shape::cGetTrianglesMinTrianglesRequested);

    static auto Get_LeafShape(const JPH::Shape* InShape) -> const JPH::Shape*
    {
        auto* Leaf = InShape;
        while (Leaf != nullptr)
        {
            const auto SubType = Leaf->GetSubType();
            if (SubType != JPH::EShapeSubType::RotatedTranslated && SubType != JPH::EShapeSubType::Scaled &&
                SubType != JPH::EShapeSubType::OffsetCenterOfMass)
            { break; }
            Leaf = static_cast<const JPH::DecoratedShape*>(Leaf)->GetInnerShape();
        }
        return Leaf;
    }

    static auto Append_Body(const ck::jolt::bake::FCk_Jolt_ExtractedBody& InBody, const AActor& InActor,
        ck::jolt::cook::FCk_Jolt_CookGeometryBody& OutBody) -> bool
    {
        const auto* Shape = InBody._Shape.GetPtr();
        if (Shape == nullptr)
        { return false; }

        auto Context = JPH::Shape::GetTrianglesContext{};
        Shape->GetTrianglesStart(Context, JPH::AABox::sBiggest(), ck::jolt::Conv(InBody._Position),
            ck::jolt::Conv(InBody._Rotation), JPH::Vec3::sReplicate(1.0f));
        auto Scratch = TArray<JPH::Float3>{};
        Scratch.SetNumUninitialized(TriangleBatchSize * 3);
        auto Bounds = FBox{ForceInit};

        for (;;)
        {
            const auto NumTriangles = Shape->GetTrianglesNext(Context, TriangleBatchSize, Scratch.GetData());
            if (NumTriangles == 0)
            { break; }
            for (auto Index = 0; Index < NumTriangles; ++Index)
            {
                const auto A = ck::jolt::Conv(Scratch[Index * 3]);
                const auto B = ck::jolt::Conv(Scratch[Index * 3 + 1]);
                const auto C = ck::jolt::Conv(Scratch[Index * 3 + 2]);
                OutBody._Triangles._Vertices.Append({A, B, C});
                const auto First = OutBody._Triangles._Vertices.Num() - 3;
                OutBody._Triangles._Indices.Append({First, First + 1, First + 2});
                Bounds += A; Bounds += B; Bounds += C;
            }
        }

        if (OutBody._Triangles.Get_IsEmpty())
        { return false; }
        OutBody._Bounds = Bounds;
        const auto* Leaf = Get_LeafShape(Shape);
        OutBody._Kind = Leaf != nullptr && Leaf->GetSubType() == JPH::EShapeSubType::HeightField
            ? ECk_Jolt_StaticBodyKind::Surface : ECk_Jolt_StaticBodyKind::Solid;
        OutBody._Description = InActor.GetPathName();
        OutBody._DataLayerNames = InActor.GetDataLayerInstanceNames();
        OutBody._DataLayerNames.Remove(NAME_None);
        OutBody._DataLayerNames.Sort(FNameLexicalLess{});
        for (auto Index = OutBody._DataLayerNames.Num() - 1; Index > 0; --Index)
        {
            if (OutBody._DataLayerNames[Index] == OutBody._DataLayerNames[Index - 1])
            { OutBody._DataLayerNames.RemoveAt(Index); }
        }
        return true;
    }
}

namespace ck::jolt::cook
{
    auto FCk_Jolt_CookGeometrySnapshot::Try_AddActor(const AActor& InActor) -> bool
    {
        if (NOT _bComplete)
        { return false; }
        auto Cache = ck::jolt::bake::FCk_Jolt_ShapeCache{};
        auto Extracted = TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>{};
        const auto Filter = ck::jolt::bake::FCk_Jolt_BakeFilter::Make_FromProjectSettings();
        ck::jolt::bake::ExtractActor(InActor, Cache, Extracted, Filter, ck::jolt::bake::ECk_Jolt_ExtractionPolicy::LevelSweep);

        auto NewBodies = TArray<FCk_Jolt_CookGeometryBody>{};
        NewBodies.Reserve(Extracted.Num());
        for (const auto& Body : Extracted)
        {
            auto ValueBody = FCk_Jolt_CookGeometryBody{};
            if (NOT ck_jolt_cook_geometry_snapshot::Append_Body(Body, InActor, ValueBody))
            { _bComplete = false; return false; }
            NewBodies.Emplace(MoveTemp(ValueBody));
        }
        _Bodies.Append(MoveTemp(NewBodies));
        ++_NumActors;
        return true;
    }
}