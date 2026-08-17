#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NativeGameplayTags.h>

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IsmProxy_UE;
class UCkUsf_OutlinePreset;
class UInstancedStaticMeshComponent;

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(FTag_Ism_Renderer_Defaults_Gnomon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(FTag_Ism_Renderer_Defaults_Entity);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsInstanceAdded);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_Disabled);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_Movable);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_IsmProxy_Params = FCk_Fragment_IsmProxy_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Current);

    public:
        friend class FProcessor_IsmProxy_Setup;
        friend class FProcessor_IsmProxy_EndPlay;
        friend class FProcessor_IsmProxy_AddInstance;
        friend class FProcessor_IsmProxy_HandleRequests;
        friend class FProcessor_IsmProxy_HandleLateCustomDataRequests;

    private:
        FPrimitiveInstanceId _IsmInstanceIndex;
        TArray<float> _CustomInstanceDataValues;

    public:
        CK_PROPERTY_GET(_IsmInstanceIndex);
        CK_PROPERTY_GET(_CustomInstanceDataValues);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applied-state for the entity outline (ck::FFragment_Usf_OutlineTarget from CkUsf): records the shadow
    // ISM (the custom-depth-only twin of the renderer's ISM, one per renderer+preset) and this proxy's
    // instance inside it, so the outline processors can undo/move it without re-deriving anything.
    struct CKISMRENDERER_API FFragment_IsmProxy_OutlineApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_OutlineApplied);

    private:
        TWeakObjectPtr<UCkUsf_OutlinePreset> _Preset;
        TWeakObjectPtr<UInstancedStaticMeshComponent> _ShadowIsm;
        FPrimitiveInstanceId _ShadowInstanceId;

    public:
        CK_PROPERTY_GET(_Preset);
        CK_PROPERTY_GET(_ShadowIsm);
        CK_PROPERTY_GET(_ShadowInstanceId);

        CK_DEFINE_CONSTRUCTORS(FFragment_IsmProxy_OutlineApplied, _Preset, _ShadowIsm, _ShadowInstanceId);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applied-state for the entity cel pattern (ck::FFragment_Usf_CelPatternTarget from CkUsf). Same
    // shadow-ISM mechanism the outline uses — Custom Depth/Stencil is per-COMPONENT, so a shared ISM cannot
    // express "pattern just this instance" — but keyed on the stencil VALUE rather than a preset, because the
    // cel contract is a direct value instead of a refcounted allocation. Nothing to release on teardown.
    struct CKISMRENDERER_API FFragment_IsmProxy_CelPatternApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_CelPatternApplied);

    private:
        ECk_Usf_CelPattern _Pattern = ECk_Usf_CelPattern::Bayer;
        TWeakObjectPtr<UInstancedStaticMeshComponent> _ShadowIsm;
        FPrimitiveInstanceId _ShadowInstanceId;
        int32 _StencilValue = 0;

    public:
        CK_PROPERTY_GET_BY_COPY(_Pattern);
        CK_PROPERTY_GET(_ShadowIsm);
        CK_PROPERTY_GET(_ShadowInstanceId);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_IsmProxy_CelPatternApplied, _Pattern, _ShadowIsm, _ShadowInstanceId, _StencilValue);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applied-state for the entity stylize effect mask (ck::FFragment_Usf_StylizeMaskTarget from CkUsf). The
    // cel-pattern twin above, minus the pattern: the mask has ONE project-wide stencil value, so membership
    // is the whole payload and the recorded value only exists to tell a no-op refresh from a real change
    // (project config can move between sessions).
    struct CKISMRENDERER_API FFragment_IsmProxy_StylizeMaskApplied
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_StylizeMaskApplied);

    private:
        TWeakObjectPtr<UInstancedStaticMeshComponent> _ShadowIsm;
        FPrimitiveInstanceId _ShadowInstanceId;
        int32 _StencilValue = 0;

    public:
        CK_PROPERTY_GET(_ShadowIsm);
        CK_PROPERTY_GET(_ShadowInstanceId);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_IsmProxy_StylizeMaskApplied, _ShadowIsm, _ShadowInstanceId, _StencilValue);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Requests);

    public:
        using SetCustomInstanceDataRequestType = FCk_Request_IsmProxy_SetCustomInstanceData;
        using SetCustomInstanceDataValueRequestType = FCk_Request_IsmProxy_SetCustomInstanceDataValue;
        using SetCustomPrimitiveDataRequestType = FCk_Request_IsmProxy_SetCustomPrimitiveData;
        using EnableDisableRequestType = FCk_Request_IsmProxy_EnableDisable;

        using RequestType = std::variant<
            SetCustomInstanceDataRequestType,
            SetCustomInstanceDataValueRequestType,
            SetCustomPrimitiveDataRequestType,
            EnableDisableRequestType>;
        using RequestList = TArray<RequestType>;

    public:
        friend class FProcessor_IsmProxy_HandleRequests;
        friend class UCk_Utils_IsmProxy_UE;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // Opt-in lane for per-instance value writes produced after the normal Gameplay_Rendering
    // request pass. DeferredApply drains it without changing the general request contract.
    struct CKISMRENDERER_API FFragment_IsmProxy_LateCustomDataRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_LateCustomDataRequests);

    public:
        friend class FProcessor_IsmProxy_HandleLateCustomDataRequests;
        friend class UCk_Utils_IsmProxy_UE;

    private:
        TArray<FCk_Request_IsmProxy_SetCustomInstanceDataValue> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
