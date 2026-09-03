#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Templates/Function.h>

#include "CkNavSurface_AreaPolicy.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// What an area tag MEANS, stated without reference to any provider's own area representation.
//
// Recast answers an area tag with a UNavArea; a provider that has no such class still has to honour
// the same authored intent, so the meaning is registered separately from the class that carries it.
// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NavSurface_AreaPolicyKind : uint8
{
    // The area removes traversal outright, for every agent, regardless of query filter
    Walkability,
    // The area stays traversable and multiplies what crossing it costs
    Cost
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_AreaPolicyKind);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_AreaPolicy
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_AreaPolicy);

private:
    // Cost rather than Walkability: a default-constructed policy must not claim an area blocks
    // everything, and Cost at multiplier 1.0 is the policy that changes nothing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_AreaPolicyKind _Kind = ECk_NavSurface_AreaPolicyKind::Cost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _CostMultiplier = 1.0f;

public:
    CK_PROPERTY_GET(_Kind);
    CK_PROPERTY_GET(_CostMultiplier);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_AreaPolicy, _Kind, _CostMultiplier);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface
{
    /**
     * Publishes what an area tag means. The module that owns the area contributes it, exactly as it
     * contributes the provider-side class.
     *
     * Registering a tag a second time with a DIFFERENT policy is an ensure and the first policy
     * stands: two modules disagreeing about what one tag means is an authoring conflict, and
     * letting the later one win would make the meaning depend on link order.
     */
    CKNAVIGATION_API auto
    Register_AreaPolicy(
        const FGameplayTag&              InAreaTag,
        const FCk_NavSurface_AreaPolicy& InPolicy) -> void;

    /**
     * The policy an area tag was registered with, or unset when nothing registered one.
     */
    CKNAVIGATION_API auto
    TryGet_AreaPolicy(
        const FGameplayTag& InAreaTag) -> TOptional<FCk_NavSurface_AreaPolicy>;

    CKNAVIGATION_API auto
    Get_RegisteredAreaTags() -> TArray<FGameplayTag>;

    /**
     * Parks a registration at static-init time. The area tags live in the gameplay-tag manager,
     * which does not exist yet when a translation unit's statics run, so the table runs every
     * pending registration on first use instead.
     *
     * Registrars are constructed during static initialization and the table is read from the game
     * thread thereafter; neither side is synchronised, and neither needs to be.
     */
    struct CKNAVIGATION_API FRegistrar
    {
        explicit FRegistrar(TFunction<void()> InRegistration);
    };
}

// --------------------------------------------------------------------------------------------------------------------
