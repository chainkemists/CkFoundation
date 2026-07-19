#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkJoltStaticActor_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Typesafe handle for a baked-static-world attribution entity — ONE per source actor that contributes at
// least one baked Jolt body. The bodies carry this entity's id as their Jolt user-data, so a static-world
// hit resolves back to this handle exactly like a dynamic JoltBody hit does.
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKJOLT_API FCk_Handle_JoltStaticActor : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_JoltStaticActor); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_JoltStaticActor);

// --------------------------------------------------------------------------------------------------------------------
