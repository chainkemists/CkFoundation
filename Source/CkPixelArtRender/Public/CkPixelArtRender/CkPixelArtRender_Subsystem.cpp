#include "CkPixelArtRender/CkPixelArtRender_Subsystem.h"

#include "CkPixelArtRender/CkPixelArtRender_Log.h"
#include "CkPixelArtRender/CkPixelArtRender_ViewExtension.h"

#include "SceneViewExtension.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PixelArtRender_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _ViewExtension = FSceneViewExtensions::NewExtension<FCk_PixelArt_ViewExtension>();

    UE_LOG(LogCkPixelArt, Display, TEXT("PixelArt scene view extension registered"));
}

auto
    UCk_PixelArtRender_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (_ViewExtension.IsValid())
    {
        // The renderer holds its own references to registered extensions, so dropping ours is not enough to stop
        // callbacks. Replacing the activation functors with one that always answers false is: a false answer
        // suppresses every callback for that frame, whoever is still holding the extension alive.
        _ViewExtension->IsActiveThisFrameFunctions.Empty();

        auto IsActiveFunctor = FSceneViewExtensionIsActiveFunctor{};
        IsActiveFunctor.IsActiveFunction =
            [](const ISceneViewExtension* InExtension, const FSceneViewExtensionContext& InContext)
            {
                return TOptional<bool>{false};
            };

        _ViewExtension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);

        // Put r.ScreenPercentage back before letting go: the extension's destructor does this too, but only once
        // the renderer has dropped its last reference, which is not guaranteed to be now.
        _ViewExtension->Request_RestoreScreenPercentage();
        _ViewExtension.Reset();
    }

    Super::Deinitialize();
}
