#include "CkNavigation/NavSurface/CkNavFilterDefinition_Registry.h"

#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_filter_definition
{
    auto Get_Definitions() -> TMap<FGameplayTag, FCk_NavFilter_Definition>&
    {
        static auto Definitions = TMap<FGameplayTag, FCk_NavFilter_Definition>{};
        return Definitions;
    }

    auto Get_PendingRegistrations() -> TArray<TFunction<void()>>&
    {
        static auto Pending = TArray<TFunction<void()>>{};
        return Pending;
    }

    auto DoFlushPendingRegistrations() -> void
    {
        auto& Pending = Get_PendingRegistrations();
        if (Pending.IsEmpty())
        { return; }

        auto Running = MoveTemp(Pending);
        Pending.Reset();

        for (const auto& Registration : Running)
        { Registration(); }
    }

    auto Get_SeededDefinitions() -> TMap<FGameplayTag, FCk_NavFilter_Definition>&
    {
        DoFlushPendingRegistrations();
        return Get_Definitions();
    }

    auto Get_IsSameCostTable(
        const TMap<FGameplayTag, float>& InA,
        const TMap<FGameplayTag, float>& InB) -> bool
    {
        if (InA.Num() != InB.Num())
        { return false; }

        for (const auto& Entry : InA)
        {
            const auto* Other = InB.Find(Entry.Key);

            if (Other == nullptr || Entry.Value != *Other)
            { return false; }
        }

        return true;
    }

    auto Get_IsSameDefinition(
        const FCk_NavFilter_Definition& InA,
        const FCk_NavFilter_Definition& InB) -> bool
    {
        return InA.Get_RequiredAreaTags() == InB.Get_RequiredAreaTags() &&
               InA.Get_ExcludedAreaTags() == InB.Get_ExcludedAreaTags() &&
               Get_IsSameCostTable(InA.Get_AreaCostMultipliers(), InB.Get_AreaCostMultipliers());
    }

    auto Get_AreAllTagsValid(
        const FGameplayTagContainer& InTags) -> bool
    {
        for (const auto& Tag : InTags)
        {
            if (NOT Tag.IsValid())
            { return false; }
        }

        return true;
    }

    auto Get_IsDefinitionValid(
        const FCk_NavFilter_Definition& InDefinition) -> bool
    {
        const auto AreaTagsAreValid =
            Get_AreAllTagsValid(InDefinition.Get_RequiredAreaTags()) &&
            Get_AreAllTagsValid(InDefinition.Get_ExcludedAreaTags());

        const auto RequiredAndExcludedDoNotConflict = NOT InDefinition.Get_RequiredAreaTags()
            .HasAnyExact(InDefinition.Get_ExcludedAreaTags());

        auto CostTagsAndValuesAreValid = true;
        for (const auto& CostEntry : InDefinition.Get_AreaCostMultipliers())
        {
            const auto CostIsValid = CostEntry.Key.IsValid() && FMath::IsFinite(CostEntry.Value) &&
                CostEntry.Value > 0.0f;
            if (NOT CostIsValid)
            {
                CostTagsAndValuesAreValid = false;
                break;
            }
        }

        return AreaTagsAreValid && RequiredAndExcludedDoNotConflict && CostTagsAndValuesAreValid;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface
{
    FFilterRegistrar::
        FFilterRegistrar(
            TFunction<void()> InRegistration)
    {
        ck_nav_surface_filter_definition::Get_PendingRegistrations().Add(MoveTemp(InRegistration));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryRegister_FilterDefinitions(
            const TMap<FGameplayTag, FCk_NavFilter_Definition>& InDefinitions)
        -> bool
    {
        auto BatchIsValid = true;

        for (const auto& Entry : InDefinitions)
        {
            const auto DefinitionIsValid = Entry.Key.IsValid() &&
                ck_nav_surface_filter_definition::Get_IsDefinitionValid(Entry.Value);
            if (NOT DefinitionIsValid)
            {
                BatchIsValid = false;
                break;
            }
        }

        auto& Definitions = ck_nav_surface_filter_definition::Get_Definitions();

        if (BatchIsValid)
        {
            for (const auto& Entry : InDefinitions)
            {
                const auto* Existing = Definitions.Find(Entry.Key);
                const auto DefinitionAgrees = Existing == nullptr ||
                    ck_nav_surface_filter_definition::Get_IsSameDefinition(*Existing, Entry.Value);
                if (NOT DefinitionAgrees)
                {
                    BatchIsValid = false;
                    break;
                }
            }
        }

        CK_ENSURE_IF_NOT(BatchIsValid,
            TEXT("Rejected nav filter definition batch: every tag and definition must be valid, costs finite and "
                 "positive, required/excluded tags non-conflicting, and existing meanings unchanged"))
        {}

        if (NOT BatchIsValid)
        { return false; }

        for (const auto& Entry : InDefinitions)
        {
            if (NOT Definitions.Contains(Entry.Key))
            { Definitions.Add(Entry.Key, Entry.Value); }
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Register_FilterDefinition(
            const FGameplayTag&             InFilterTag,
            const FCk_NavFilter_Definition& InDefinition)
        -> void
    {
        auto Definitions = TMap<FGameplayTag, FCk_NavFilter_Definition>{};
        Definitions.Add(InFilterTag, InDefinition);
        TryRegister_FilterDefinitions(Definitions);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryGet_FilterDefinition(
            const FGameplayTag& InFilterTag)
        -> TOptional<FCk_NavFilter_Definition>
    {
        if (NOT InFilterTag.IsValid())
        { return {}; }

        const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
        if (ck::IsValid(Settings))
        {
            if (const auto* Configured = Settings->Get_QueryFilters().Find(InFilterTag))
            {
                const auto* Definition = Configured->LoadSynchronous();

                const auto DefinitionIsValid = ck::IsValid(Definition);
                CK_ENSURE_IF_NOT(DefinitionIsValid,
                    TEXT("Nav QueryFilter tag [{}] maps to a filter definition that failed to load"),
                    InFilterTag)
                {}

                if (NOT DefinitionIsValid)
                { return {}; }

                const auto LoadedDefinitionIsValid =
                    ck_nav_surface_filter_definition::Get_IsDefinitionValid(Definition->Get_Definition());
                CK_ENSURE_IF_NOT(LoadedDefinitionIsValid,
                    TEXT("Nav QueryFilter tag [{}] maps to an invalid filter definition"), InFilterTag)
                {}

                if (NOT LoadedDefinitionIsValid)
                { return {}; }

                return Definition->Get_Definition();
            }
        }

        const auto& Definitions = ck_nav_surface_filter_definition::Get_SeededDefinitions();
        if (const auto* Native = Definitions.Find(InFilterTag))
        { return *Native; }

        CK_TRIGGER_ENSURE(
            TEXT("Nav QueryFilter tag [{}] has no mapping in Ck Navigation project settings or native registry"),
            InFilterTag);
        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
