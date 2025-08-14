#pragma once

// Standard library headers
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Format/CkFormat_Defaults.h"
#include "CkCore/Log/CkLog.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

#include <Engine.h>
#include <CoreSharedPCH.h>
#include <EngineSharedPCH.h>

#pragma warning(push)
#pragma warning(disable: 4191)

#if WITH_ANGELSCRIPT_CK
#include <angelscript.h>
#include <AngelscriptManager.h>
#include <AngelscriptDocs.h>
#include <as_scriptengine.h>
#endif

#if WITH_EDITOR
#include <UnrealEd.h>
#endif
