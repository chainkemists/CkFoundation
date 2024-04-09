#pragma once
#include "CkCore/Format/CkFormat.h"

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

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EAttachmentRule _LocationRule = EAttachmentRule::KeepWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EAttachmentRule _RotationRule = EAttachmentRule::KeepWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    EAttachmentRule _ScaleRule = EAttachmentRule::KeepWorld;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _WeldSimulatedBodies = false;

public:
    CK_PROPERTY(_LocationRule);
    CK_PROPERTY(_RotationRule);
    CK_PROPERTY(_ScaleRule);
    CK_PROPERTY(_WeldSimulatedBodies);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Actor_AttachToActor_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Actor_AttachToActor_Params);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<AActor> _ActorToAttachTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _SocketName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Actor_AttachmentRules _AttachmentRules;

public:
    CK_PROPERTY_GET(_ActorToAttachTo);
    CK_PROPERTY(_SocketName);
    CK_PROPERTY(_AttachmentRules);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Actor_AttachToActor_Params, _ActorToAttachTo);
};

// --------------------------------------------------------------------------------------------------------------------
