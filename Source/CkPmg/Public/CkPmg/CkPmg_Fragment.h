#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkCore/Chrono/CkChrono.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Pmg_Donut_UE;
class UCk_Utils_Pmg_DebugShape_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Pmg_Donut_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Donut_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Donut_Params);

    public:
        friend class FProcessor_Pmg_Donut_Setup;
        friend class UCk_Utils_Pmg_Donut_UE;

    private:
        FCk_Fragment_Pmg_Donut_ParamsData _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Donut_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Donut_Current);

    public:
        friend class FProcessor_Pmg_Donut_Setup;
        friend class FProcessor_Pmg_Donut_HandleRequests;
        friend class FProcessor_Pmg_Donut_UpdateTransform;
        friend class FProcessor_Pmg_Donut_EndPlay;
        friend class UCk_Utils_Pmg_Donut_UE;

    private:
        TStrongObjectPtr<UProceduralMeshComponent> _MeshComponent;

        float _InnerRadius = 50.0f;
        float _OuterRadius = 100.0f;
        int32 _Segments = 32;
        float _FillAngle = 360.0f;
        TObjectPtr<UMaterialInterface> _Material;
        bool _EnableCollision = false;
        ECk_Pmg_RenderMode _RenderMode = ECk_Pmg_RenderMode::DoubleSided;

    public:
        CK_PROPERTY_GET(_MeshComponent);
        CK_PROPERTY_GET(_InnerRadius);
        CK_PROPERTY_GET(_OuterRadius);
        CK_PROPERTY_GET(_Segments);
        CK_PROPERTY_GET(_FillAngle);
        CK_PROPERTY_GET(_Material);
        CK_PROPERTY_GET(_EnableCollision);
        CK_PROPERTY_GET(_RenderMode);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Pmg_Donut_UpdateParams = FCk_Request_Pmg_Donut_UpdateParams;

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Pmg_Donut_UpdateParams);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_NeedsSetup);

    // Marker for debug shapes that delegate rendering to child entities
    // instead of owning their own mesh component (icons, pivots, dashed
    // lines). Their own FFragment_Pmg_DebugShape_Current._MeshComponent is
    // intentionally null, so transform-update processors must skip them.
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_Composite);

    // One-shot bake gate for the wireframe overlay. Stamped automatically by
    // ck::pmg::Append_Debug*_World helpers whenever lines are appended;
    // consumed by FProcessor_Pmg_DebugShape_BakeLines, which triangulates
    // each cached FCk_Pmg_DebugLine as a flat rectangle and appends it as
    // a second mesh section on the entity's procmesh. The wireframe shares
    // the filled mesh's UMaterialInstanceDynamic, so Request_SetColor
    // updates both sections at once. After bake the tag is removed; the
    // processor never iterates the entity again. Replaces the prior
    // per-tick FProcessor_Pmg_DebugShape_DrawLines (which churned UE's
    // TransientLineBatchComponent every frame via DrawDebugLine).
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_LinesNeedBaking);

    // --------------------------------------------------------------------------------------------------------------------

    // Common properties shared by all debug shapes (C++ only - not exposed to BP/AS)
    struct CKPMG_API FFragment_Pmg_DebugShape_Common
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Common);

    public:
        friend class FProcessor_Pmg_Wedge_Setup;
        friend class FProcessor_Pmg_Arc_Setup;
        friend class FProcessor_Pmg_WedgeCone_Setup;
        friend class FProcessor_Pmg_Sphere_Setup;
        friend class FProcessor_Pmg_Box_Setup;
        friend class FProcessor_Pmg_Cone_Setup;
        friend class FProcessor_Pmg_Cylinder_Setup;
        friend class FProcessor_Pmg_Capsule_Setup;
        friend class FProcessor_Pmg_Pyramid_Setup;
        friend class FProcessor_Pmg_Hemisphere_Setup;
        friend class FProcessor_Pmg_Torus_Setup;
        friend class FProcessor_Pmg_Circle_Setup;
        friend class FProcessor_Pmg_Plane_Setup;
        friend class FProcessor_Pmg_Ring_Setup;
        friend class FProcessor_Pmg_Cross_Setup;
        friend class FProcessor_Pmg_Star_Setup;
        friend class FProcessor_Pmg_Checkmark_Setup;
        friend class FProcessor_Pmg_Diamond_Setup;
        friend class FProcessor_Pmg_Warning_Setup;
        friend class FProcessor_Pmg_Prohibition_Setup;
        friend class FProcessor_Pmg_NoEntry_Setup;
        friend class FProcessor_Pmg_MagnifyingGlass_Setup;
        friend class FProcessor_Pmg_QuestionMark_Setup;
        friend class FProcessor_Pmg_ExclamationMark_Setup;
        friend class FProcessor_Pmg_Flag_Setup;
        friend class FProcessor_Pmg_InfoCircle_Setup;
        friend class FProcessor_Pmg_Pin_Setup;
        friend class FProcessor_Pmg_Arrow_Setup;
        friend class FProcessor_Pmg_Pivot_Setup;
        friend class FProcessor_Pmg_DashedLine_Setup;
        friend class FProcessor_Pmg_Text_Setup;
        friend class FProcessor_Pmg_DebugShape_HandleRequests;

    private:
        FLinearColor _Color = FLinearColor::White;
        bool _DrawLines = true;
        float _LineThickness = 2.0f;
        FCk_Time _Duration = FCk_Time{0.0f};
        bool _EnableCollision = false;
        ECk_Pmg_RenderMode _RenderMode = ECk_Pmg_RenderMode::DoubleSided;

    public:
        CK_PROPERTY(_Color);
        CK_PROPERTY(_DrawLines);
        CK_PROPERTY(_LineThickness);
        CK_PROPERTY(_Duration);
        CK_PROPERTY(_EnableCollision);
        CK_PROPERTY(_RenderMode);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // A single cached debug line, expressed in entity-local space so that
    // FProcessor_Pmg_DebugShape_DrawLines can re-emit it each tick under the
    // entity's live transform — keeping wireframes attached to persistent /
    // moving shapes instead of flashing once at setup.
    struct CKPMG_API FCk_Pmg_DebugLine
    {
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;
        FLinearColor _Color = FLinearColor::White;
        float _Thickness = 2.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Per-shape cache of wireframe segments in entity-local space. Populated
    // by Setup processors via ck::pmg::Append_DebugLine_World (or _Box) and
    // consumed each tick by FProcessor_Pmg_DebugShape_DrawLines.
    struct CKPMG_API FFragment_Pmg_DebugShape_Lines
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Lines);

    public:
        friend class FProcessor_Pmg_DebugShape_BakeLines;
        friend class FProcessor_Pmg_Text_Setup;

    public:
        TArray<FCk_Pmg_DebugLine> _Lines;

    public:
        CK_PROPERTY_GET(_Lines);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_DebugShape_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Current);

    public:
        friend class FProcessor_Pmg_DebugShape_UpdateTransform;
        friend class FProcessor_Pmg_DebugShape_CheckDuration;
        friend class FProcessor_Pmg_DebugShape_EndPlay;
        friend class FProcessor_Pmg_DebugShape_HandleRequests;
        friend class FProcessor_Pmg_DebugShape_BakeLines;
        friend class FProcessor_Pmg_Wedge_Setup;
        friend class FProcessor_Pmg_Arc_Setup;
        friend class FProcessor_Pmg_WedgeCone_Setup;
        friend class FProcessor_Pmg_Sphere_Setup;
        friend class FProcessor_Pmg_Box_Setup;
        friend class FProcessor_Pmg_Cone_Setup;
        friend class FProcessor_Pmg_Cylinder_Setup;
        friend class FProcessor_Pmg_Capsule_Setup;
        friend class FProcessor_Pmg_Pyramid_Setup;
        friend class FProcessor_Pmg_Hemisphere_Setup;
        friend class FProcessor_Pmg_Torus_Setup;
        friend class FProcessor_Pmg_Circle_Setup;
        friend class FProcessor_Pmg_Plane_Setup;
        friend class FProcessor_Pmg_Ring_Setup;
        friend class FProcessor_Pmg_Cross_Setup;
        friend class FProcessor_Pmg_Star_Setup;
        friend class FProcessor_Pmg_Checkmark_Setup;
        friend class FProcessor_Pmg_Diamond_Setup;
        friend class FProcessor_Pmg_Warning_Setup;
        friend class FProcessor_Pmg_Prohibition_Setup;
        friend class FProcessor_Pmg_NoEntry_Setup;
        friend class FProcessor_Pmg_MagnifyingGlass_Setup;
        friend class FProcessor_Pmg_QuestionMark_Setup;
        friend class FProcessor_Pmg_ExclamationMark_Setup;
        friend class FProcessor_Pmg_Flag_Setup;
        friend class FProcessor_Pmg_InfoCircle_Setup;
        friend class FProcessor_Pmg_Pin_Setup;
        friend class FProcessor_Pmg_Arrow_Setup;
        friend class FProcessor_Pmg_Pivot_Setup;
        friend class FProcessor_Pmg_DashedLine_Setup;
        friend class FProcessor_Pmg_Text_Setup;

    private:
        TStrongObjectPtr<UProceduralMeshComponent> _MeshComponent;
        FCk_Time _SpawnTime;

    public:
        CK_PROPERTY_GET(_MeshComponent);
        CK_PROPERTY_GET(_SpawnTime);

        CK_DEFINE_CONSTRUCTORS(FFragment_Pmg_DebugShape_Current, _MeshComponent, _SpawnTime);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Per-shape pending mutation requests. Added on demand (AddOrGet) the first
    // time a Request_* utility is called against a shape; drained each tick by
    // FProcessor_Pmg_DebugShape_HandleRequests. Empty fragment after drain;
    // CopyAndRemove pattern means the fragment itself is removed from the entity
    // when there are no pending requests.
    struct CKPMG_API FFragment_Pmg_DebugShape_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Requests);

    public:
        friend class FProcessor_Pmg_DebugShape_HandleRequests;
        friend class ::UCk_Utils_Pmg_DebugShape_UE;

        using SetColorRequestType           = FCk_Request_Pmg_DebugShape_SetColor;
        using SetLineThicknessRequestType   = FCk_Request_Pmg_DebugShape_SetLineThickness;
        using SetDrawLinesRequestType       = FCk_Request_Pmg_DebugShape_SetDrawLines;
        using SetDurationRequestType        = FCk_Request_Pmg_DebugShape_SetDuration;
        using SetRenderModeRequestType      = FCk_Request_Pmg_DebugShape_SetRenderMode;
        using SetEnableCollisionRequestType = FCk_Request_Pmg_DebugShape_SetEnableCollision;

        using RequestType = std::variant<
            SetColorRequestType,
            SetLineThicknessRequestType,
            SetDrawLinesRequestType,
            SetDurationRequestType,
            SetRenderModeRequestType,
            SetEnableCollisionRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
