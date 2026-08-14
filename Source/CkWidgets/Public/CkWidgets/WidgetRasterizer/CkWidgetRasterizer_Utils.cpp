#include "CkWidgets/WidgetRasterizer/CkWidgetRasterizer_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Blueprint/UserWidget.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SVirtualWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/HittestGrid.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "Misc/App.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_WidgetRasterizer_UE::
    Get_CanRasterizeOnThisProcess()
    -> bool
{
    // FWidgetRenderer needs a real Slate renderer + RHI; -nullrhi / dedicated server / commandlet have none.
    return FApp::CanEverRender() && NOT IsRunningDedicatedServer() && FSlateApplication::IsInitialized();
}

auto
    UCk_Utils_WidgetRasterizer_UE::
    RenderWidgetToTexture(
        UUserWidget* InWidget,
        FIntPoint InRenderSize,
        ECk_WidgetRasterizer_GammaCorrection InGammaCorrection)
    -> UTexture2D*
{
    if (ck::Is_NOT_Valid(InWidget))
    { return nullptr; }

    if (NOT Get_CanRasterizeOnThisProcess())
    { return nullptr; }

    if (InRenderSize.X <= 0 || InRenderSize.Y <= 0)
    { return nullptr; }

    const FVector2D DrawSize(InRenderSize.X, InRenderSize.Y);

    // ---- Render target (BGRA8 to match FColor readback + the transient texture) ----

    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->ClearColor = FLinearColor::Transparent;
    RenderTarget->bAutoGenerateMips = false;
    RenderTarget->InitCustomFormat(InRenderSize.X, InRenderSize.Y, PF_B8G8R8A8, /*bInForceLinearGamma*/ false);
    RenderTarget->UpdateResourceImmediate(true);

    // ---- Widget renderer + headless virtual window ----
    // The RT is left sRGB (encodes on write). With GammaCorrection::Enabled the renderer ALSO
    // encodes, double-applying gamma (washed-out); Disabled lets the sRGB RT do the single encode.

    const auto UseGammaCorrection = InGammaCorrection == ECk_WidgetRasterizer_GammaCorrection::Enabled;
    FWidgetRenderer* WidgetRenderer = new FWidgetRenderer(UseGammaCorrection);
    if (WidgetRenderer == nullptr)
    { return nullptr; }

    const TSharedRef<SWidget> SlateWidget = InWidget->TakeWidget();

    const TSharedRef<SVirtualWindow> VirtualWindow = SNew(SVirtualWindow).Size(DrawSize);
    VirtualWindow->SetContent(SlateWidget);
    VirtualWindow->Resize(DrawSize);

    const TSharedRef<FHittestGrid> HitTestGrid = MakeShared<FHittestGrid>();

    // Pass 1 — prepass: AutoWrapText / justification reference the geometry cached during Arrange, so text
    // renders unwrapped / mis-justified without a layout pass before the real draw.
    WidgetRenderer->SetIsPrepassNeeded(true);
    WidgetRenderer->DrawWindow(RenderTarget, HitTestGrid.Get(), VirtualWindow, 1.0f, DrawSize, 0.0f);

    // Pass 2 — actual draw.
    WidgetRenderer->DrawWindow(RenderTarget, HitTestGrid.Get(), VirtualWindow, 1.0f, DrawSize, 0.0f);

    FlushRenderingCommands();

    // ---- Readback -> transient texture ----

    FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
    if (RenderTargetResource == nullptr)
    {
        BeginCleanup(WidgetRenderer);
        return nullptr;
    }

    TArray<FColor> Pixels;
    FReadSurfaceDataFlags ReadFlags(RCM_UNorm, CubeFace_MAX);
    ReadFlags.SetLinearToGamma(false);

    if (NOT RenderTargetResource->ReadPixels(Pixels, ReadFlags) ||
        Pixels.Num() != InRenderSize.X * InRenderSize.Y)
    {
        BeginCleanup(WidgetRenderer);
        return nullptr;
    }

    UTexture2D* OutTexture = UTexture2D::CreateTransient(InRenderSize.X, InRenderSize.Y, PF_B8G8R8A8);
    if (ck::Is_NOT_Valid(OutTexture))
    {
        BeginCleanup(WidgetRenderer);
        return nullptr;
    }

    OutTexture->SRGB = true;
    OutTexture->NeverStream = true;

    FTexture2DMipMap& Mip = OutTexture->GetPlatformData()->Mips[0];
    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Mip.BulkData.Unlock();
    OutTexture->UpdateResource();

    // FWidgetRenderer must be torn down on the render thread (FDeferredCleanupInterface).
    BeginCleanup(WidgetRenderer);

    return OutTexture;
}
