#pragma once

#include "CoreMinimal.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkUsf_StylizeMask_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How a stylize effect is confined to part of the view. The order is a CONTRACT with
// CkUsf_Stylize_MaskWeight's dispatcher in /CkUsf/StylizeCommon.ush — every effect's subsystem writes
// this enum's integer value straight into its look's MaskMode parameter.
UENUM(BlueprintType)
enum class ECk_Usf_StylizeMaskMode : uint8
{
    Off,                   // the effect covers the whole view — the historical behaviour
    IncludeStencilRange,   // ONLY pixels whose Custom Stencil lands in the range are stylized
    ExcludeStencilRange    // pixels whose Custom Stencil lands in the range are left untouched
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_StylizeMaskMode);

// --------------------------------------------------------------------------------------------------------------------

// The per-effect masking block, carried identically by all four stylize params structs. One value shared
// by four effects rather than four near-identical field groups: the Custom-Stencil byte is ONE resource
// and the rule that keeps the claims on it disjoint has to be stated once (NN#9).
//
// The range is inclusive on both ends. It must stay clear of UCkUsf_OutlineSubsystem's allocated range AND
// of the cel-pattern span while that contract is enabled — each effect's Request_SetSettings rejects a
// colliding range with its own diagnostic rather than letting one feature's meshes silently restyle under
// another's.
//
// A mask is a PIXEL test on a nominal id, not on a varying quantity, so it is deliberately NOT
// fwidth-anti-aliased: fwidth of a stencil id spikes at every id boundary and measures nothing about how
// far the pixel is from the threshold. Edge quality therefore comes from where the look sits in the chain
// — pre-TAA looks get a temporally resolved edge for free, ScreenDither (post-tonemap) does not and
// softens its own mask over the 4 axis neighbours instead. See each look's .ush header.
USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_StylizeMask_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Usf_StylizeMask_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_StylizeMaskMode _Mode = ECk_Usf_StylizeMaskMode::Off;

    // Lowest Custom-Stencil value the mask matches. Defaults to the shipped
    // UCk_Usf_Stylize_ProjectSettings_UE mask value (190), so the entity-level API works with no tuning.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 255, ClampMax = 255,
                      EditCondition = "_Mode != ECk_Usf_StylizeMaskMode::Off"))
    int32 _StencilMin = 190;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 255, ClampMax = 255,
                      EditCondition = "_Mode != ECk_Usf_StylizeMaskMode::Off"))
    int32 _StencilMax = 190;

public:
    CK_PROPERTY(_Mode);
    CK_PROPERTY(_StencilMin);
    CK_PROPERTY(_StencilMax);

public:
    // Whether this block claims any Custom-Stencil value at all. Off claims nothing, so an Off mask can
    // never collide with the outline or cel-pattern ranges and is never validated against them.
    auto Get_ClaimsStencil() const -> bool;

public:
    auto operator==(const ThisType& InOther) const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
