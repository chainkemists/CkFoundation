#pragma once

#include "CkNavigation/NavSurface/CkNavFilterDefinition_DataAsset.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Templates/Function.h>

// --------------------------------------------------------------------------------------------------------------------
// Which DEFINITION a query filter tag names, stated without reference to any provider's own filter.
//
// Recast compiles a definition into an FNavigationQueryFilter and a grounded field compiles the same
// one into plate tables, so a filter tag that meant two different things on the two providers would
// make "the same query" a claim nothing could check. The definition is therefore registered once,
// here, exactly as an area tag's POLICY is - and each provider reads this one table.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface
{
    /**
     * Publishes what a query filter tag means. The module that owns the filter contributes it.
     *
     * Registering a tag a second time with a DIFFERENT definition is an ensure and the first
     * definition stands, for the reason Register_AreaPolicy states: two modules disagreeing about
     * what one tag means is an authoring conflict, and letting the later one win would make the
     * meaning depend on link order.
     */
    CKNAVIGATION_API auto
    Register_FilterDefinition(
        const FGameplayTag&             InFilterTag,
        const FCk_NavFilter_Definition& InDefinition) -> void;

    /**
     * The definition a filter tag resolves to: the project settings' own mapping first, then the
     * native table, and unset for a tag neither names - which ensures, because a query asking for a
     * filter nothing defined is answered by the provider's default and that is never what it asked
     * for. An invalid tag is the caller asking for no filter at all and passes silently.
     */
    CKNAVIGATION_API auto
    TryGet_FilterDefinition(
        const FGameplayTag& InFilterTag) -> TOptional<FCk_NavFilter_Definition>;

    /**
     * Parks a registration at static-init time. The filter tags live in the gameplay-tag manager,
     * which does not exist yet when a translation unit's statics run, so the table runs every
     * pending registration on first use instead.
     *
     * A type of its own rather than nav_surface::FRegistrar, which parks AREA POLICY registrations
     * against that table's own pending list: one name cannot carry two lists in one namespace, and a
     * shared list would run an area registration at the moment a filter is first read.
     *
     * Registrars are constructed during static initialization and the table is read from the game
     * thread thereafter; neither side is synchronised, and neither needs to be.
     */
    struct CKNAVIGATION_API FFilterRegistrar
    {
        explicit FFilterRegistrar(TFunction<void()> InRegistration);
    };
}

// --------------------------------------------------------------------------------------------------------------------
