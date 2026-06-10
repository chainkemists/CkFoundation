#include "CkRenderTarget_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"
#include "CkRenderTarget/Pixels/CkRenderTarget_PixelMath.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_Settings_UE::
    Get_ChunkSizeBytes()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_RenderTarget_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::render_target::transport::ChunkSizeBytes; }

    return Settings->Get_ChunkSizeBytes();
}

auto
    UCk_Utils_RenderTarget_Settings_UE::
    Get_MaxBytesPerStreamPerTick()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_RenderTarget_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::render_target::transport::MaxBytesPerStreamPerTick; }

    return Settings->Get_MaxBytesPerStreamPerTick();
}

auto
    UCk_Utils_RenderTarget_Settings_UE::
    Get_BlockSize()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_RenderTarget_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::render_target::pixel::DefaultBlockSize; }

    return Settings->Get_BlockSize();
}

auto
    UCk_Utils_RenderTarget_Settings_UE::
    Get_MaxManagedSize()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_RenderTarget_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 1024; }

    return Settings->Get_MaxManagedSize();
}

auto
    UCk_Utils_RenderTarget_Settings_UE::
    Get_ClientUploadMaxBytesPerSecond()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_RenderTarget_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 262144; }

    return Settings->Get_ClientUploadMaxBytesPerSecond();
}

// --------------------------------------------------------------------------------------------------------------------
