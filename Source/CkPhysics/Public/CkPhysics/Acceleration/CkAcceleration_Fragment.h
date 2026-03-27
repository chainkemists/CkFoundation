#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkAcceleration_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Acceleration_UE;
class UCk_Utils_BulkAccelerationModifier_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Acceleration_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_AccelerationChannel);
    CK_DEFINE_ECS_TAG(FTag_AccelerationModifier);
    CK_DEFINE_ECS_TAG(FTag_AccelerationModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkAccelerationModifier_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_BulkAccelerationModifier_GlobalScope);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Acceleration_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Acceleration_Params);

    public:
        using ParamsType = FCk_Fragment_Acceleration_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Acceleration_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_Acceleration_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Acceleration_Current);

    public:
        friend class UCk_Utils_Acceleration_UE;
        friend class FProcessor_Acceleration_Setup;
        friend class FProcessor_AccelerationModifier_Setup;
        friend class FProcessor_AccelerationModifier_EndPlay;

    private:
        FVector _CurrentAcceleration = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentAcceleration);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Acceleration_Current, _CurrentAcceleration);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkAccelerationModifier_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkAccelerationModifier_Params);

    public:
        using ParamsType = FCk_Fragment_BulkAccelerationModifier_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_BulkAccelerationModifier_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_BulkAccelerationModifier_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_BulkAccelerationModifier_Requests);

    public:
        friend class FProcessor_BulkAccelerationModifier_HandleRequests;
        friend class UCk_Utils_BulkAccelerationModifier_UE;

    public:
        using RequestStartAffectingEntityType = FCk_Request_BulkAccelerationModifier_AddTarget;
        using RequestStopAffectingEntityType = FCk_Request_BulkAccelerationModifier_RemoveTarget;

        using RequestType = std::variant<RequestStartAffectingEntityType, RequestStopAffectingEntityType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER(FFragment_Acceleration_Target, FCk_Handle_Transform);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAccelerationModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfBulkAccelerationModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAccelerationChannels, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_BulkAccelerationModifier_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKPHYSICS_API FCk_RepData_Acceleration
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Acceleration);

    UPROPERTY()
    FVector Value = FVector::ZeroVector;
};

namespace ck
{
    using FFragment_ContainerRef_Acceleration = TFragment_ContainerEntryRef<FCk_RepData_Acceleration>;
}

// --------------------------------------------------------------------------------------------------------------------
