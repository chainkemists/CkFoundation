#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkMemory/CkMemory_Subsystem.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkMemory_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKMEMORY_API UCk_Utils_Memory_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Memory_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Memory",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static FCk_Utils_Memory_MemoryCountSnapshot_Result
    Get_MemoryCountSnapshot(
        const UObject* InContext = nullptr);
};

// --------------------------------------------------------------------------------------------------------------------
