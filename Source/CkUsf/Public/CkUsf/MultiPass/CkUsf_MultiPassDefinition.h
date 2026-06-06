#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CkUsf_MultiPassDefinition.generated.h"

class UCkUsf_LookDefinition;

// --------------------------------------------------------------------------------------------------------------------

// Shadertoy-style pass slots. Buffers are double-buffered render targets (feedback);
// Image is the final pass that writes the display render target.
UENUM(BlueprintType)
enum class ECk_Usf_PassId : uint8
{
    BufferA,
    BufferB,
    BufferC,
    BufferD,
    Image
};

// --------------------------------------------------------------------------------------------------------------------

// Binds one of a pass's iChannel inputs to another pass's buffer render target.
// (Texture / cubemap channels are handled by the pass LookDefinition's own _DefaultTexturePath.)
USTRUCT(BlueprintType)
struct CKUSF_API FCkUsf_BufferChannelBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    int32 _ChannelIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_PassId _Buffer = ECk_Usf_PassId::BufferA;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUSF_API FCkUsf_PassDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_PassId _Id = ECk_Usf_PassId::Image;

    // The pass shader, authored as a normal look that writes EmissiveColor (= fragColor.rgb).
    // Its params should include the iChannelN texture inputs + iResolution/iFrame/iTimeDelta.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TObjectPtr<UCkUsf_LookDefinition> _Look;

    // Which iChannelN inputs read which buffer's render target.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FCkUsf_BufferChannelBinding> _BufferChannels;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKUSF_API UCkUsf_MultiPassDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Passes run in array order each frame; buffers swap (ping-pong) afterward. Put Image last.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FCkUsf_PassDef> _Passes;

    // Square render-target resolution for all buffers + the display RT.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    int32 _Resolution = 512;
};
