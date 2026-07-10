#include "CkLoadingProcess_Interface.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ICk_LoadingProcess::
    Get_ShouldShowLoadingScreen(
        UObject* InTestObject,
        FString& OutReason)
    -> bool
{
    if (ck::Is_NOT_Valid(InTestObject))
    { return false; }

    const auto LoadObserver = Cast<ICk_LoadingProcess>(InTestObject);

    if (ck::Is_NOT_Valid(LoadObserver, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    auto ObserverReason = FString{};
    if (NOT LoadObserver->Get_ShouldShowLoadingScreen(ObserverReason))
    { return false; }

    CK_ENSURE_IF_NOT(NOT ObserverReason.IsEmpty(),
        TEXT("[{}] wants to show the loading screen but failed to set a reason why"), InTestObject)
    {
        OutReason = TEXT("(no reason given)");
        return true;
    }

    OutReason = ObserverReason;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
