#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/EngineTypes.h>

#include "CkJoltCollisionLayer_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/// Which broadphase tree a body lives in. Jolt's broadphase keeps one bounding-volume tree per
/// layer; Static-vs-Static pairs are filtered out entirely (statics never pair with each other).
UENUM(BlueprintType)
enum class ECk_Jolt_BodyDomain : uint8
{
    Static,
    Dynamic
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_BodyDomain);

// --------------------------------------------------------------------------------------------------------------------

/// UE's touch-resolution outcome for a body pair or a query-vs-body test:
/// min(A.Response[B.Channel], B.Response[A.Channel]). Jolt's binary pair filter treats
/// anything != Ignore as "interact"; Block-vs-Overlap is resolved at query/contact sites.
UENUM(BlueprintType)
enum class ECk_Jolt_PairInteraction : uint8
{
    Ignore,
    Overlap,
    Block
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_PairInteraction);

// --------------------------------------------------------------------------------------------------------------------

/// The complete set of inputs UE/Chaos uses to decide whether two bodies interact, captured
/// per-body: object channel, the full 32-channel response array (2 bits per channel), collision
/// enabled mode, and the Static/Dynamic domain. One Jolt ObjectLayer is allocated per unique
/// signature (Phase 2) — per-channel or per-profile layers both lose information (profiles differ
/// in response arrays; components can carry custom per-channel edits).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_CollisionSignature
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_CollisionSignature);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TEnumAsByte<ECollisionChannel> _ObjectChannel = ECC_WorldStatic;

    // 2 bits per channel x 32 channels. Bit pattern per channel: 0 = Ignore, 1 = Overlap, 2 = Block.
    // (uint64 is not Blueprint-exposable — BP/AS reads go through Get_ResponseToChannel.)
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint64 _ResponseMask = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TEnumAsByte<ECollisionEnabled::Type> _CollisionEnabled = ECollisionEnabled::QueryAndPhysics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Jolt_BodyDomain _Domain = ECk_Jolt_BodyDomain::Static;

public:
    CK_PROPERTY(_ObjectChannel);
    CK_PROPERTY(_ResponseMask);
    CK_PROPERTY(_CollisionEnabled);
    CK_PROPERTY(_Domain);

public:
    auto Get_ResponseToChannel(ECollisionChannel InChannel) const -> ECk_Jolt_PairInteraction;
    auto Set_ResponseToChannel(ECollisionChannel InChannel, ECk_Jolt_PairInteraction InResponse) -> void;

    /// Builds the signature from a component's effective collision setup (object type, the
    /// component's response container including per-component custom edits, collision enabled).
    static auto Make_FromComponent(
        const class UPrimitiveComponent& InComponent,
        ECk_Jolt_BodyDomain InDomain) -> FCk_Jolt_CollisionSignature;

    auto operator==(const FCk_Jolt_CollisionSignature& InOther) const -> bool
    {
        return _ObjectChannel == InOther._ObjectChannel
            && _ResponseMask == InOther._ResponseMask
            && _CollisionEnabled == InOther._CollisionEnabled
            && _Domain == InOther._Domain;
    }

    auto operator!=(const FCk_Jolt_CollisionSignature& InOther) const -> bool
    {
        return NOT (*this == InOther);
    }

    friend auto GetTypeHash(const FCk_Jolt_CollisionSignature& InSignature) -> uint32
    {
        return HashCombine(
            HashCombine(GetTypeHash(InSignature._ResponseMask), GetTypeHash(InSignature._ObjectChannel.GetValue())),
            HashCombine(GetTypeHash(static_cast<uint8>(InSignature._CollisionEnabled.GetValue())),
                GetTypeHash(static_cast<uint8>(InSignature._Domain))));
    }
};

// --------------------------------------------------------------------------------------------------------------------
