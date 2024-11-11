#include "CkMessaging_Fragment_Data.h"

#include <NativeGameplayTags.h>
#include <Misc/DataValidation.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Message, TEXT("Message"));

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_Message_Definition_PDA::
    IsDataValid(
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(Context);

    if (IsTemplate())
    { return Result; }

    if (ck::Is_NOT_Valid(_MessageName))
    {
        Context.AddError(FText::FromString(ck::Format_UE(TEXT("Message Definition [{}] is missing a valid Message Name"), this)));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    return Result;
}
#endif

auto
    UCk_Message_Definition_PDA::
    GetPrimaryAssetId() const
    -> FPrimaryAssetId
{
    return FPrimaryAssetId(_AssetRegistryCategory, GetFName());
}

// --------------------------------------------------------------------------------------------------------------------
