#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkVelocity_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UMovementComponent;
class UCk_Utils_Velocity_UE;
class UCk_Utils_BulkVelocityModifier_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Velocity_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VelocityChannel);
    CK_DEFINE_ECS_TAG(FTag_VelocityModifier);
    CK_DEFINE_ECS_TAG(FTag_VelocityModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkVelocityModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkVelocityModifier_GlobalScope);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Velocity_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_Params);

    public:
        using ParamsType = FCk_Fragment_Velocity_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Velocity_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_MovementComponent
    {
    public:
        CK_GENERATED_BODY(FFragment_MovementComponent);

    private:
        TWeakObjectPtr<UMovementComponent> _MovementComponent;

    public:
        CK_PROPERTY_GET(_MovementComponent);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_MovementComponent, _MovementComponent);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Velocity_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_Current);

    public:
        friend class UCk_Utils_Velocity_UE;
        friend class FProcessor_Velocity_Setup;
        friend class FProcessor_Velocity_Clamp;
        friend class FProcessor_VelocityModifier_Setup;
        friend class FProcessor_VelocityModifier_EndPlay;

    private:
        FVector _CurrentVelocity = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentVelocity);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Velocity_Current, _CurrentVelocity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Velocity_MinMax
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_MinMax);

    public:
        friend class UCk_Utils_Velocity_UE;

    private:
        TOptional<float> _MinSpeed;
        TOptional<float> _MaxSpeed;

    public:
        CK_PROPERTY_GET(_MinSpeed);
        CK_PROPERTY_GET(_MaxSpeed);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkVelocityModifier_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkVelocityModifier_Params);

    public:
        using ParamsType = FCk_Fragment_BulkVelocityModifier_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_BulkVelocityModifier_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkVelocityModifier_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkVelocityModifier_Requests);

    public:
        friend class FProcessor_BulkVelocityModifier_HandleRequests;
        friend class UCk_Utils_BulkVelocityModifier_UE;

    public:
        using RequestStartAffectingEntityType = FCk_Request_BulkVelocityModifier_AddTarget;
        using RequestStopAffectingEntityType = FCk_Request_BulkVelocityModifier_RemoveTarget;

        using RequestType = std::variant<RequestStartAffectingEntityType, RequestStopAffectingEntityType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER(FFragment_Velocity_Target, FCk_Handle_Transform);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVelocityModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfBulkVelocityModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVelocityChannels, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_BulkVelocityModifier_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKPHYSICS_API FCk_RepData_Velocity
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Velocity);

    UPROPERTY()
    FVector Value = FVector::ZeroVector;
};

namespace ck
{
    using FFragment_ContainerRef_Velocity = TFragment_ContainerEntryRef<FCk_RepData_Velocity>;
}

// --------------------------------------------------------------------------------------------------------------------
