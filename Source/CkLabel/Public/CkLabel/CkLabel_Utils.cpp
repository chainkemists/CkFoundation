#include "CkLabel_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkLabel/CkLabel_Fragment.h"

#include <GameplayTagsManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_label
{
    struct FGameplayLabel_Tags final : public FGameplayTagNativeAdder
    {
    public:
        virtual ~FGameplayLabel_Tags() = default;

    protected:
        auto AddTags() -> void override
        {
            auto& Manager = UGameplayTagsManager::Get();

            _None = Manager.AddNativeGameplayTag(TEXT("Ck.Label.None"));
        }

    private:
        FGameplayTag _None;

        static FGameplayLabel_Tags _Tags;

    public:
        static auto Get_None() -> FGameplayTag
        {
            return _Tags._None;
        }
    };

    FGameplayLabel_Tags FGameplayLabel_Tags::_Tags;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameplayLabel_UE::
    Add(
        FCk_Handle& InHandle,
        FGameplayTag InLabel)
    -> void
{
    if (Has(InHandle))
    {
        CK_ENSURE(MatchesExact(InHandle, InLabel),
            TEXT("Unable to add Label [{}]. Entity [{}] already has the Label [{}]."),
            InLabel,
            InHandle,
            Get_Label(InHandle));

        return;
    }

    InHandle.Add<ck::FFragment_GameplayLabel>(DoGet_LabelOrNone(InLabel));

    if (ck::IsValid(InLabel))
    {
        UCk_Utils_Handle_UE::Set_DebugName(InHandle, InLabel.GetTagName());
    }
}

auto
    UCk_Utils_GameplayLabel_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_GameplayLabel>();
}

auto
    UCk_Utils_GameplayLabel_UE::
    Ensure(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Gameplay Label"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_GameplayLabel_UE::
    Get_Label(
        const FCk_Handle& InHandle)
    -> FGameplayTag
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_GameplayLabel>().Get_Label();
}

auto
    UCk_Utils_GameplayLabel_UE::
    Get_IsUnnamedLabel(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Gameplay Label"), InHandle)
    { return false; }

    const auto& Label = Get_Label(InHandle);
    if (Label == ck_label::FGameplayLabel_Tags::Get_None())
    { return true; }

    return false;
}

auto
    UCk_Utils_GameplayLabel_UE::
    DoGet_LabelOrNone(
        const FGameplayTag& InTag)
    -> FGameplayTag
{
    if (ck::Is_NOT_Valid(InTag))
    { return ck_label::FGameplayLabel_Tags::Get_None(); }

    return InTag;
}

auto
    UCk_Utils_GameplayLabel_UE::
    Matches(
        const FCk_Handle& InHandle,
        const FGameplayTag InTagToMatch)
    -> bool
{
    const auto Label = Get_Label(InHandle);

    return Label.MatchesTag(DoGet_LabelOrNone(InTagToMatch));
}

auto
    UCk_Utils_GameplayLabel_UE::
    MatchesExact(
        const FCk_Handle& InHandle,
        const FGameplayTag InTagToMatch)
    -> bool
{
    const auto Label = Get_Label(InHandle);

    return Label.MatchesTagExact(DoGet_LabelOrNone(InTagToMatch));
}

auto
    UCk_Utils_GameplayLabel_UE::
    MatchesAny(
        const FCk_Handle& InHandle,
        const FGameplayTagContainer& InTagsToMatch)
    -> bool
{
    const auto Label = Get_Label(InHandle);

    return Label.MatchesAny(InTagsToMatch);
}

auto
    UCk_Utils_GameplayLabel_UE::
    MatchesAnyExact(
        const FCk_Handle& InHandle,
        const FGameplayTagContainer& InTagsToMatch)
    -> bool
{
    const auto Label = Get_Label(InHandle);

    return Label.MatchesAnyExact(InTagsToMatch);
}

// --------------------------------------------------------------------------------------------------------------------
