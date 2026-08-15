#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CkUsf/MultiPass/CkUsf_MultiPassDefinition.h"
#include "CkUsf_MultiPassRenderer.generated.h"

class UCkUsf_MultiPassDefinition;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;

// --------------------------------------------------------------------------------------------------------------------

// Drives a UCkUsf_MultiPassDefinition: per frame, draws each pass material into its (double-buffered)
// render target, binding buffer channels + uniforms (iResolution/iFrame/iTimeDelta); buffers ping-pong
// so a pass can read its own previous frame (feedback). The Image pass writes the display RT.
UCLASS(ClassGroup = (CkUsf), meta = (BlueprintSpawnableComponent))
class CKUSF_API UCkUsf_MultiPassRenderer : public UActorComponent
{
    GENERATED_BODY()

public:
    UCkUsf_MultiPassRenderer();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TObjectPtr<UCkUsf_MultiPassDefinition> _Definition;

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf",
              DisplayName = "[Ck][Usf] Get Output Render Target")
    UTextureRenderTarget2D* Get_OutputRenderTarget() const;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    void DoSetup();

    UPROPERTY(Transient)
    TMap<ECk_Usf_PassId, TObjectPtr<UTextureRenderTarget2D>> _BufferRead;

    UPROPERTY(Transient)
    TMap<ECk_Usf_PassId, TObjectPtr<UTextureRenderTarget2D>> _BufferWrite;

    UPROPERTY(Transient)
    TMap<ECk_Usf_PassId, TObjectPtr<UMaterialInstanceDynamic>> _PassMIDs;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> _DisplayRT;

    int32 _Frame = 0;
    bool _Initialized = false;
};
