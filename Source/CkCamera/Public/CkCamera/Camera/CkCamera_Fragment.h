#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"
#include "CkCamera/Camera/Profile/CkCameraProfile.h"
#include "CkCamera/Camera/CkCamera_POV.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include <Camera/CameraTypes.h>
#include <StructUtils/InstancedStruct.h>

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Camera_UE;
class UCk_CameraModifier_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // DIRECTOR FRAGMENTS
    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Params);

    public:
        using ParamsType = FCk_Fragment_Camera_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Camera_Params, _Params);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Current);

    public:
        friend class FProcessor_Camera_ComposeProfile;
        friend class FProcessor_Camera_UpdatePOV;
        friend class UCk_Utils_Camera_UE;

    private:
        // The dedicated profile entity modifiers contribute into via handle (DoContributeToProfile). Created in
        // UCk_Utils_Camera_UE::Add, owned by (and destroyed with) the director.
        FCk_Handle_CameraProfile _ProfileEntity;

        // Resolved read-cache of the profile entity's composed result — refreshed each frame by ComposeProfile.
        // UpdatePOV and observers (debugger inspector) read this instead of resolving the entity every access.
        FCk_CameraProfile _ComposedProfile;

        // Persistent POV pipeline state (boom rotation, smoothed pivot, collision distance, noise) — advanced by UpdatePOV.
        ck::camera::FPov_State _PovState;

        // Abstract orbit intention fed by whoever owns input (PC / ability / rail). Zero = no manual orbit.
        // The camera module never reads input devices — it only consumes this value.
        FVector _OrientationIntention = FVector::ZeroVector;

        // Look-at target resolved from the dominant active modifier (for auto-reorient). Set by ComposeProfile,
        // consumed by UpdatePOV. Unset = the dominant modifier has no look-at (a zero vector is a valid target).
        TOptional<FVector> _DominantLookAt;

        // Class of the dominant active modifier (highest blend alpha). Set by ComposeProfile; observable via Utils.
        TSubclassOf<UCk_CameraModifier_EntityScript> _DominantModifierClass;

        // The final resolved POV — written by UpdatePOV, read by UCk_CameraComponent::GetCameraView.
        FMinimalViewInfo _ViewInfo;

    public:
        CK_PROPERTY(_ProfileEntity);
        CK_PROPERTY(_ComposedProfile);
        CK_PROPERTY(_OrientationIntention);
        CK_PROPERTY_GET(_DominantModifierClass);
        CK_PROPERTY_GET(_DominantLookAt);
        CK_PROPERTY_GET(_PovState);
        CK_PROPERTY_GET(_ViewInfo);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_Camera_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Camera_Requests);

    public:
        friend class FProcessor_Camera_HandleRequests;
        friend class UCk_Utils_Camera_UE;

    public:
        using AddModifierRequestType    = FCk_Request_Camera_AddModifier;
        using RemoveModifierRequestType = FCk_Request_Camera_RemoveModifier;
        using Requests                  = TArray<std::variant<AddModifierRequestType, RemoveModifierRequestType>>;

    private:
        Requests _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // PER-MODIFIER FRAGMENTS
    // ----------------------------------------------------------------------------------------------------------------

    // Per-modifier identity/config: the script class (so RemoveModifier/OneOnly can match), the ordering group,
    // and an optional look-at target for auto-reorient.
    struct CKCAMERA_API FFragment_CameraModifier_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraModifier_Params);

    public:
        friend class FProcessor_Camera_HandleRequests;

    private:
        TSubclassOf<UCk_CameraModifier_EntityScript> _ModifierClass;
        FGameplayTag                                 _OrderingGroup;
        FCk_Handle_Transform                         _LookAtTarget;

    public:
        CK_PROPERTY_GET(_ModifierClass);
        CK_PROPERTY(_OrderingGroup);
        CK_PROPERTY(_LookAtTarget);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_CameraModifier_Params, _ModifierClass);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Per-modifier blend weight in [0,1]. _Alpha interpolates toward _TargetAlpha at _BlendRate (units/sec).
    // Blend-in: target 1. Blend-out (removal / OneOnly eviction): target 0 → pruned when it reaches 0.
    struct CKCAMERA_API FFragment_CameraModifier_Blend
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraModifier_Blend);

    public:
        friend class FProcessor_Camera_HandleRequests;
        friend class FProcessor_Camera_ComposeProfile;
        friend class FProcessor_Camera_UpdatePOV;

    private:
        float _Alpha       = 0.0f;
        float _TargetAlpha = 1.0f;
        float _BlendRate   = 1000.0f; // ~instant unless overridden

    public:
        CK_PROPERTY(_Alpha);
        CK_PROPERTY(_TargetAlpha);
        CK_PROPERTY(_BlendRate);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Marks an active modifier (admitted to the compose loop). Stamped by EnterModifier, cleared by ExitModifier.
    CK_DEFINE_ECS_TAG(FTag_CameraModifier_Active);

    // Marks a modifier blending out / awaiting teardown.
    CK_DEFINE_ECS_TAG(FTag_CameraModifier_PendingExit);

    // ----------------------------------------------------------------------------------------------------------------
    // BACK-REFERENCE + RECORD
    //
    // NOTE: the modifier EntityScript is attached SYNCHRONOUSLY in FProcessor_Camera_HandleRequests
    // (mirroring UCk_Utils_SmState_UE::Create), NOT via a deferred PendingAttach. The SM PendingAttach exists
    // for tasks/conditions composed within DefineState (parent declares, child overrides in the same construction
    // pass) — a build-time hazard cameras don't have. A camera modifier is a runtime-pushed "mode" (≈ an SM
    // state, which also attaches synchronously). Same-frame Add(X)->Remove(X) of the identical modifier is not a
    // supported flow (same as SM states); revisit with request-batch reconciliation if it ever becomes one.
    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_CameraModifier_OwningCamera, FFragment_CameraModifier_OwningCamera, FCk_Handle_Camera);

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfCameraModifiers, FCk_Handle_CameraModifier);

    // Shared record-of-modifiers utility (used by both the processor and the Utils class).
    struct CKCAMERA_API FUtils_RecordOfCameraModifiers : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfCameraModifiers> {};
}

// --------------------------------------------------------------------------------------------------------------------
