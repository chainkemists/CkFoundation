#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/GameInstance.h>

#include "CkGameInstance.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API UCk_GameInstance_UE : public UGameInstance
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameInstance_UE);

public:
    auto Init() -> void override;

    auto Shutdown() -> void override;

private:
    static TWeakObjectPtr<ThisType> _Instance;

public:
    static auto Get_Instance() -> ThisType*;
};

// --------------------------------------------------------------------------------------------------------------------
