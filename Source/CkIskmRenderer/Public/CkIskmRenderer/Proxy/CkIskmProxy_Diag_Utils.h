#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmProxy_Diag_Utils.generated.h"

class UAnimInstance;

// --------------------------------------------------------------------------------------------------------------------
// Diagnostic accessors for IskmProxy internals.
//
// Use case: regression tests that need to observe state which the public
// `UCk_Utils_IskmProxy_UE` API intentionally does not expose (raw SKMC, raw
// AnimInstance pointer, etc.). Production game code should NOT call these —
// they leak implementation details that may change without notice.
//
// Naming: this class is separate from `UCk_Utils_IskmProxy_UE` to make the
// "diagnostic / test-only" intent visible at every call site.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IskmProxy"))
class CKISKMRENDERER_API UCk_Utils_IskmProxy_Diag_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmProxy_Diag_UE);

public:
    // Returns the raw UAnimInstance currently driving the proxy's BaseSKMC,
    // or nullptr if the SKMC has none. Test-only — pointer equality across
    // calls is the signal of interest (e.g. asserting no teardown happened).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy|Diag",
        DisplayName="[Ck][IskmProxy][Diag] Get AnimInstance")
    static UAnimInstance*
    Get_AnimInstance(const FCk_Handle_IskmProxy& InHandle);
};
