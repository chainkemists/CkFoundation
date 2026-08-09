#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <InputCoreTypes.h>

#include "CkInputButtonMap_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Which space a button's identity was minted in, and therefore what its key association follows.
 *
 * Mapped is one button per Enhanced Input player-mappable MAPPING NAME: its key is whatever the player's
 * resolved mappings currently say, and it moves when they rebind. Physical is one button per raw key that no
 * mapping claims: its key is the key, fixed forever, which is what makes it usable before a binding profile
 * exists at all.
 */
UENUM(BlueprintType)
enum class ECk_Input_ButtonTier : uint8
{
    Mapped,
    Physical
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Input_ButtonTier);

// --------------------------------------------------------------------------------------------------------------------

/**
 * A stable identity for a pressable thing, decoupled from the key that currently produces it.
 *
 * Identity is (tier, name) and is IMMUTABLE: once a button has been derived or registered it never changes for
 * the map's lifetime, and a re-derive moves ASSOCIATIONS only. That is the whole point — a definition naming a
 * button keeps naming the same button after the player rebinds it, with no edit anywhere.
 *
 * The name is the mapping name for a Mapped button and the key's own FName for a Physical one, so identity is
 * derivation-deterministic: two maps built from the same profile agree without having to agree on an order.
 * There is deliberately no dense index here — packing buttons into a bitmask is a bake-time job for a consumer
 * that knows exactly which buttons it references.
 */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Input_ButtonId
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Input_ButtonId);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Input_ButtonTier _Tier = ECk_Input_ButtonTier::Mapped;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Name = NAME_None;

public:
    CK_PROPERTY_GET(_Tier);
    CK_PROPERTY_GET(_Name);

public:
    auto operator==(
        const FCk_Input_ButtonId& InOther) const -> bool
    {
        return _Tier == InOther._Tier && _Name == InOther._Name;
    }

    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FCk_Input_ButtonId);

    friend auto GetTypeHash(
        const FCk_Input_ButtonId& InButtonId) -> uint32
    {
        return HashCombineFast(GetTypeHash(InButtonId._Tier), GetTypeHash(InButtonId._Name));
    }

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Input_ButtonId, _Tier, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One button and the key currently producing it. An INVALID key is a real state, not an error: a mapping the
 * player left unbound — or one whose context stopped being registered — keeps its identity and reports no key,
 * because dropping the identity would break every definition that names it.
 */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Input_ButtonAssociation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Input_ButtonAssociation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _ButtonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _Key = FKey{EKeys::Invalid};

public:
    CK_PROPERTY_GET(_ButtonId);
    CK_PROPERTY(_Key);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Input_ButtonAssociation, _ButtonId);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINPUT_API FCk_Handle_InputButtonMap : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InputButtonMap); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InputButtonMap);

// --------------------------------------------------------------------------------------------------------------------

// The tier-2 buttons this map is born with. Tier 1 is absent here on purpose: mapped buttons derive from the
// player's live profile, and a declaration of them would be a second source of truth that goes stale the first
// time the player rebinds anything.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Fragment_InputButtonMap_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InputButtonMap_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FKey> _PhysicalButtons;

public:
    CK_PROPERTY_GET(_PhysicalButtons);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InputButtonMap_ParamsData, _PhysicalButtons);
};

// --------------------------------------------------------------------------------------------------------------------

// Re-reads every tier-1 association from the player's resolved mappings. Carries no payload because there is
// nothing to choose: the profile is the argument, and a partial re-derive would let two mapped buttons disagree
// about which profile they came from.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputButtonMap_Rederive : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputButtonMap_Rederive);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputButtonMap_Rederive);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputButtonMap_RegisterPhysicalButton : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputButtonMap_RegisterPhysicalButton);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputButtonMap_RegisterPhysicalButton);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _Key;

public:
    CK_PROPERTY_GET(_Key);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputButtonMap_RegisterPhysicalButton, _Key);
};

// --------------------------------------------------------------------------------------------------------------------
