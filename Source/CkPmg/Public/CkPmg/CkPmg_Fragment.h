#pragma once

#include "CkPmg_Fragment_Data_Donut.h"
#include "CkPmg_Fragment_Data_DebugShapes.h"

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
        FCk_Pmg_Donut_Spec _Params;

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
        // Lifetime owned by the CkCore ObjectPooling subsystem (DestroyOnRelease)
        TWeakObjectPtr<UProceduralMeshComponent> _MeshComponent;

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

    // Rendering is delegated to child entities, so this entity's own _MeshComponent stays null
    // and transform-update processors must skip it.
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_Composite);

    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_LinesNeedBaking);

    // Shapes with a negative duration never expire and must stay out of the per-frame duration view.
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_PersistentDuration);

    CK_DEFINE_ECS_TAG(FTag_Pmg_EditorSelectionHandle);

    // --------------------------------------------------------------------------------------------------------------------

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

    // Entity-local space, so the baked wireframe rides the entity's live transform.
    struct CKPMG_API FCk_Pmg_DebugLine
    {
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;
        FLinearColor _Color = FLinearColor::White;
        float _Thickness = 2.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

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
        // Subsystem-owned (see FFragment_Pmg_Donut_Current::_MeshComponent)
        TWeakObjectPtr<UProceduralMeshComponent> _MeshComponent;
        FCk_Time _SpawnTime;

    public:
        CK_PROPERTY_GET(_MeshComponent);
        CK_PROPERTY_GET(_SpawnTime);

        CK_DEFINE_CONSTRUCTORS(FFragment_Pmg_DebugShape_Current, _MeshComponent, _SpawnTime);
    };

    // --------------------------------------------------------------------------------------------------------------------

    namespace pmg_debug_shape
    {
        // Debug-shape color is the semantic outline color. Filled mesh sections deliberately
        // use the same RGB at a low, fixed alpha so they remain readable without obscuring
        // the scene; baked wireframes retain their own fully opaque MID.
        inline constexpr float FillOpacity = 0.1f;

        inline auto
            Get_FillColor(
                FLinearColor InColor) -> FLinearColor
        {
            InColor.A = FillOpacity;
            return InColor;
        }

        inline auto
            AddCommon(
                FCk_Handle& InHandle,
                const FFragment_Pmg_DebugShape_Common& InCommon) -> void
        {
            InHandle.Add<FFragment_Pmg_DebugShape_Common>(InCommon);

            if (InCommon.Get_Duration().Get_Seconds() < 0.0f)
            {
                InHandle.AddOrGet<FTag_Pmg_DebugShape_PersistentDuration>();
            }
        }

        inline auto
            UpdateDurationMembership(
                FCk_Handle& InHandle,
                const FCk_Time& InDuration) -> void
        {
            if (InDuration.Get_Seconds() < 0.0f)
            {
                InHandle.AddOrGet<FTag_Pmg_DebugShape_PersistentDuration>();
            }
            else
            {
                InHandle.Try_Remove<FTag_Pmg_DebugShape_PersistentDuration>();
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

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
