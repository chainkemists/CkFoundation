#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Params.h"

#include "CkAcceleration_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Acceleration_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Acceleration_Setup {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Acceleration_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Acceleration_Params);

    public:
        using ParamsType = FCk_Fragment_Acceleration_ParamsData;

    public:
        FCk_Fragment_Acceleration_Params() = default;
        explicit FCk_Fragment_Acceleration_Params(ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Acceleration_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Acceleration_Current);

    public:
        friend class FCk_Processor_Acceleration_Setup;
        friend class UCk_Utils_Acceleration_UE;

    public:
        FCk_Fragment_Acceleration_Current() = default;
        explicit FCk_Fragment_Acceleration_Current(
            FVector InAcceleration);

    private:
        FVector _CurrentAcceleration = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentAcceleration);
    };
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKPHYSICS_API UCk_Fragment_Acceleration_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Acceleration_Rep);

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Acceleration();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Acceleration)
    FVector _Acceleration = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------
