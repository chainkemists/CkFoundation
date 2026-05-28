#pragma once

#include "CkCamera/GameplayCamera/CkGameplayCamera_Fragment_Data.h"
#include "CkCamera/GameplayCamera/CkGameplayCamera_Profile.h"

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

class UCk_Utils_GameplayCamera_UE;
class UCk_CameraModifier_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // DIRECTOR FRAGMENTS
    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_GameplayCamera_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_GameplayCamera_Params);

    public:
        using ParamsType = FCk_Fragment_GameplayCamera_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_GameplayCamera_Params, _Params);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_GameplayCamera_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_GameplayCamera_Current);

    public:
        friend class FProcessor_GameplayCamera_ComposeProfile;
        friend class FProcessor_GameplayCamera_UpdatePOV;
        friend class UCk_Utils_GameplayCamera_UE;

    private:
        // The composed profile (accumulated modifier contributions) — written by ComposeProfile.
        FCk_GameplayCamera_Profile _ComposedProfile;

        // The final resolved POV — written by UpdatePOV, read by UCk_GameplayCameraComponent::GetCameraView.
        FMinimalViewInfo _ViewInfo;

    public:
        CK_PROPERTY(_ComposedProfile);
        CK_PROPERTY_GET(_ViewInfo);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_GameplayCamera_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_GameplayCamera_Requests);

    public:
        friend class FProcessor_GameplayCamera_HandleRequests;
        friend class UCk_Utils_GameplayCamera_UE;

    public:
        using AddModifierRequestType    = FCk_Request_GameplayCamera_AddModifier;
        using RemoveModifierRequestType = FCk_Request_GameplayCamera_RemoveModifier;
        using Requests                  = TArray<std::variant<AddModifierRequestType, RemoveModifierRequestType>>;

    private:
        Requests _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // PER-MODIFIER FRAGMENTS
    // ----------------------------------------------------------------------------------------------------------------

    // Stores the modifier's script class so RemoveModifier can match it after the deferred attach
    // has consumed FFragment_CameraModifier_PendingAttach.
    struct CKCAMERA_API FFragment_CameraModifier_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraModifier_Params);

    private:
        TSubclassOf<UCk_CameraModifier_EntityScript> _ModifierClass;

    public:
        CK_PROPERTY_GET(_ModifierClass);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_CameraModifier_Params, _ModifierClass);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Per-modifier blend weight in [0,1]. M0: pinned at 1.0; M1 drives blend-in/out over time.
    struct CKCAMERA_API FFragment_CameraModifier_Blend
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraModifier_Blend);

    public:
        friend class FProcessor_GameplayCamera_HandleRequests;
        friend class FProcessor_GameplayCamera_ComposeProfile;
        friend class FProcessor_GameplayCamera_UpdatePOV;

    private:
        float _Alpha = 1.0f;

    public:
        CK_PROPERTY(_Alpha);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Marks an active modifier (admitted to the compose loop). Stamped by EnterModifier, cleared by ExitModifier.
    CK_DEFINE_ECS_TAG(FTag_CameraModifier_Active);

    // Marks a modifier blending out / awaiting teardown.
    CK_DEFINE_ECS_TAG(FTag_CameraModifier_PendingExit);

    // ----------------------------------------------------------------------------------------------------------------
    // BACK-REFERENCE + RECORD
    //
    // NOTE: the modifier EntityScript is attached SYNCHRONOUSLY in FProcessor_GameplayCamera_HandleRequests
    // (mirroring UCk_Utils_SmState_UE::Create), NOT via a deferred PendingAttach. The SM PendingAttach exists
    // for tasks/conditions composed within DefineState (parent declares, child overrides in the same construction
    // pass) — a build-time hazard cameras don't have. A camera modifier is a runtime-pushed "mode" (≈ an SM
    // state, which also attaches synchronously). Same-frame Add(X)->Remove(X) of the identical modifier is not a
    // supported flow (same as SM states); revisit with request-batch reconciliation if it ever becomes one.
    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_CameraModifier_OwningCamera, FFragment_CameraModifier_OwningCamera, FCk_Handle_GameplayCamera);

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfCameraModifiers, FCk_Handle_CameraModifier);

    // Shared record-of-modifiers utility (used by both the processor and the Utils class).
    struct CKCAMERA_API FUtils_RecordOfCameraModifiers : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfCameraModifiers> {};
}

// --------------------------------------------------------------------------------------------------------------------
