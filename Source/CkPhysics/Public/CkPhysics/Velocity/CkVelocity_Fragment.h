#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkLabel/CkLabel_Fragment.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkVelocity_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Velocity_UE;
class UCk_Utils_BulkVelocityModifier_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_Velocity_Setup {};
    struct FTag_VelocityChannel {};
    struct FTag_VelocityModifier {};
    struct FTag_VelocityModifier_Setup {};
    struct FTag_BulkVelocityModifier_Setup {};
    struct FTag_BulkVelocityModifier_GlobalScope {};

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

    struct CKPHYSICS_API FFragment_Velocity_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Velocity_Current);

    public:
        friend class UCk_Utils_Velocity_UE;
        friend class FProcessor_Velocity_Setup;
        friend class FProcessor_VelocityModifier_Setup;
        friend class FProcessor_VelocityModifier_Teardown;

    private:
        FVector _CurrentVelocity = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentVelocity);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Velocity_Current, _CurrentVelocity);
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

    struct FFragment_Velocity_Target : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfVelocityModifiers : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfBulkVelocityModifiers : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfVelocityChannels : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Velocity_Replicate; }

UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Velocity_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Velocity_Rep);

public:
    friend class ck::FProcessor_Velocity_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Velocity();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Velocity)
    FVector _Velocity = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------
