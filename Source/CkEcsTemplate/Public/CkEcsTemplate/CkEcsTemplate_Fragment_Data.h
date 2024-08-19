#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

#include "CkEcsTemplate_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSTEMPLATE_API FCk_Fragment_EcsTemplate_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EcsTemplate_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSTEMPLATE_API FCk_Fragment_MultipleEcsTemplate_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleEcsTemplate_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_EcsTemplate_ParamsData> _EcsTemplateParams;

public:
    CK_PROPERTY_GET(_EcsTemplateParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleEcsTemplate_ParamsData, _EcsTemplateParams);
};

// --------------------------------------------------------------------------------------------------------------------