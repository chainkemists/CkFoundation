#pragma once

#include "CkDebugScene/CkDebugScene_Shapes.h"
#include "CkDebugScene/CkDebugScene_Target.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Ergonomic face over the debug scene, in the spirit of CkPmg's shape utilities.
//
// Accumulate shapes with Add_*, then Commit once. Mesh shapes become instances of the shared unit
// primitives and land as a single Upsert_Item; lines land on a line channel, which is a separate
// mechanism in the target and cannot carry meshes.
//
//   auto Shapes = FCk_DebugScene_ShapeBuilder{}.Set_Color(FLinearColor::Green);
//   Shapes.Add_Ring(Destination, 55.0f, 0.75f);
//   Shapes.Add_Line(Destination, Destination + FVector::UpVector * 120.0, 3.0f);
//   Shapes.Commit(Target, MyItemKey, TEXT("MyDebugger.Ping"));
//
// Commit REPLACES everything previously published under that item key and channel, so a caller
// re-publishing each frame gets diffing for free and a caller publishing an empty builder clears.
// --------------------------------------------------------------------------------------------------------------------

class CKDEBUGSCENE_API FCk_DebugScene_ShapeBuilder
{
  public:
    /** Colour applied to subsequent Add_* calls that do not name their own. */
    auto
    Set_Color(FLinearColor InColor) -> FCk_DebugScene_ShapeBuilder&;
    /** Below 1.0 also routes subsequent shapes to the translucent render class. */
    auto
    Set_Opacity(float InOpacity) -> FCk_DebugScene_ShapeBuilder&;
    auto
    Set_DepthPriority(ECk_DebugScene_DepthPriority InDepthPriority) -> FCk_DebugScene_ShapeBuilder&;
    /** Pick identity stamped on subsequent shapes. 0 (the default) means not individually pickable. */
    auto
    Set_PickIdentity(uint64 InPickIdentity) -> FCk_DebugScene_ShapeBuilder&;

    // --- 3D. Sizes are full extents / radii in world units, not the unit-mesh scale. ---------------

    auto
    Add_Box(const FVector& InCenter, const FVector& InExtent, const FRotator& InRotation = FRotator::ZeroRotator,
            TOptional<FLinearColor> InColor = {}) -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Sphere(const FVector& InCenter, float InRadius, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Cylinder(const FVector& InCenter, float InRadius, float InHeight,
                 const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Cone(const FVector& InBaseCenter, float InRadius, float InHeight,
             const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    /** InSegmentHeight is the STRAIGHT section only; total height is InSegmentHeight + 2*InRadius. */
    auto
    Add_Capsule(const FVector& InCenter, float InRadius, float InSegmentHeight,
                const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Torus(const FVector& InCenter, float InOuterRadius, float InTubeRatio = 0.25f,
              const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    /** Cylinder stretched between two points -- the filled counterpart of a thick line. */
    auto
    Add_Bar(const FVector& InFrom, const FVector& InTo, float InRadius, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;

    // --- 2D. Default to the XY plane; pass a rotation to reach another. ----------------------------

    auto
    Add_Quad(const FVector& InCenter, const FVector2D& InSize, const FRotator& InRotation = FRotator::ZeroRotator,
             TOptional<FLinearColor> InColor = {}) -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Disc(const FVector& InCenter, float InRadius, const FRotator& InRotation = FRotator::ZeroRotator,
             TOptional<FLinearColor> InColor = {}) -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Ring(const FVector& InCenter, float InOuterRadius, float InInnerRatio = 0.75f,
             const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Triangle(const FVector& InCenter, float InRadius, const FRotator& InRotation = FRotator::ZeroRotator,
                 TOptional<FLinearColor> InColor = {}) -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_Cross(const FVector& InCenter, float InSize, float InThicknessRatio = 0.25f,
              const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    /** Flat arrow lying in its plane, from one point toward another. */
    auto
    Add_Arrow(const FVector& InFrom, const FVector& InTo, float InWidth, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;

    // --- Lines (separate mechanism; committed to the line channel) ---------------------------------

    auto
    Add_Line(const FVector& InFrom, const FVector& InTo, float InThickness = 1.0f,
             TOptional<FLinearColor> InColor = {}) -> FCk_DebugScene_ShapeBuilder&;
    /** Closed polyline outline in the XY plane -- the unfilled counterpart of Add_Disc. */
    auto
    Add_Circle(const FVector& InCenter, float InRadius, int32 InSegments = 32, float InThickness = 1.0f,
               const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;
    auto
    Add_WireBox(const FVector& InCenter, const FVector& InExtent, float InThickness = 1.0f,
                const FRotator& InRotation = FRotator::ZeroRotator, TOptional<FLinearColor> InColor = {})
        -> FCk_DebugScene_ShapeBuilder&;

    // --- Publication ------------------------------------------------------------------------------

    /** Publishes meshes under InItemKey and lines under InLineChannel. Either may be empty, which
     *  clears whatever was previously published there. Returns false if the target rejected it. */
    auto
    Commit(FCk_DebugScene_Target& InTarget, uint64 InItemKey, FName InLineChannel) -> bool;
    auto
    Reset() -> void;

    auto
    Get_Instances() const -> const TArray<FCk_DebugScene_Instance>&;
    auto
    Get_Lines() const -> const TArray<FCk_DebugScene_Line>&;
    auto
    IsEmpty() const -> bool;

  private:
    auto
    Make_Appearance(const TOptional<FLinearColor>& InColor) const -> FCk_DebugScene_Appearance;
    auto
    Add_Mesh(TSharedPtr<FCk_DebugScene_Mesh> InMesh, const FTransform& InTransform,
             const TOptional<FLinearColor>& InColor) -> FCk_DebugScene_ShapeBuilder&;

    TArray<FCk_DebugScene_Instance> _Instances;
    TArray<FCk_DebugScene_Line> _Lines;
    FLinearColor _Color = FLinearColor::White;
    float _Opacity = 1.0f;
    ECk_DebugScene_DepthPriority _DepthPriority = ECk_DebugScene_DepthPriority::World;
    uint64 _PickIdentity = 0;
};
