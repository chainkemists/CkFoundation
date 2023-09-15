#pragma once
#include "CkFormat/CkFormat.h"

#include "CkActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Actor_NetworkingType : uint8
{
    Local,
    Replicated
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Actor_NetworkingType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Actor_AttachmentRules
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Actor_AttachmentRules);

public:
    FCk_Actor_AttachmentRules() = default;
    FCk_Actor_AttachmentRules(
        EAttachmentRule InLocationRule,
        EAttachmentRule InRotationRule,
        EAttachmentRule InScaleRule,
        bool            InWeldSimulatedBodies);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EAttachmentRule _LocationRule        = EAttachmentRule::KeepWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EAttachmentRule _RotationRule        = EAttachmentRule::KeepWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EAttachmentRule _ScaleRule           = EAttachmentRule::KeepWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _WeldSimulatedBodies = false;

public:
    CK_PROPERTY_GET(_LocationRule);
    CK_PROPERTY_GET(_RotationRule);
    CK_PROPERTY_GET(_ScaleRule);
    CK_PROPERTY_GET(_WeldSimulatedBodies);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Actor_AttachToActor_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Actor_AttachToActor_Params);

public:
    FCk_Actor_AttachToActor_Params() = default;
    FCk_Actor_AttachToActor_Params(
        AActor*                   InActorToAttachTo,
        FName                     InSocketName,
        FCk_Actor_AttachmentRules InAttachmentRules);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<AActor> _ActorToAttachTo   = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _SocketName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Actor_AttachmentRules _AttachmentRules;

public:
    CK_PROPERTY_GET(_ActorToAttachTo);
    CK_PROPERTY_GET(_SocketName);
    CK_PROPERTY_GET(_AttachmentRules);
};

// --------------------------------------------------------------------------------------------------------------------
