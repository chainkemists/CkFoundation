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

            if (Other == nullptr || NOT FMath::IsNearlyEqual(Entry.Value, *Other))
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
        Register_FilterDefinition(
            const FGameplayTag&             InFilterTag,
            const FCk_NavFilter_Definition& InDefinition)
        -> void
    {
        const auto RegistrationIsValid = InFilterTag.IsValid();
        CK_ENSURE_IF_NOT(RegistrationIsValid,
            TEXT("Rejected nav filter definition registration: invalid tag"))
        { return; }

        auto& Definitions = ck_nav_surface_filter_definition::Get_Definitions();

        const auto* Existing = Definitions.Find(InFilterTag);

        if (Existing == nullptr)
        {
            Definitions.Add(InFilterTag, InDefinition);
            return;
        }

        const auto DefinitionAgrees =
            ck_nav_surface_filter_definition::Get_IsSameDefinition(*Existing, InDefinition);

        CK_ENSURE_IF_NOT(DefinitionAgrees,
            TEXT("Nav filter tag [{}] is already registered with a different definition and was "
                 "re-registered. The FIRST definition stands."),
            InFilterTag)
        { return; }
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
                    TEXT("Nav QueryFilter tag [{}] maps to a filter definition that failed to load — using default filter"),
                    InFilterTag)
                { return {}; }

                return Definition->Get_Definition();
            }
        }

        const auto& Definitions = ck_nav_surface_filter_definition::Get_SeededDefinitions();
        if (const auto* Native = Definitions.Find(InFilterTag))
        { return *Native; }

        CK_TRIGGER_ENSURE(
            TEXT("Nav QueryFilter tag [{}] has no mapping in Ck Navigation project settings — using default filter"),
            InFilterTag);
        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
