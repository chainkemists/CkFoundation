#include "CkLabel_Utils.h"

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
        FCk_Handle InHandle,
        const FGameplayTag& InLabel)
    -> void
{
    if (ck::IsValid(InLabel))
    {
        InHandle.Add<ck::FFragment_GameplayLabel>(InLabel);
    }
    else
    {
        InHandle.Add<ck::FFragment_GameplayLabel>(ck_label::FGameplayLabel_Tags::Get_None());
    }
}

auto
    UCk_Utils_GameplayLabel_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_GameplayLabel>();
}

auto
    UCk_Utils_GameplayLabel_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Gameplay Label"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_GameplayLabel_UE::
    Get_Label(
        FCk_Handle InHandle)
    -> FGameplayTag
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_GameplayLabel>().Get_Label();
}

// --------------------------------------------------------------------------------------------------------------------
