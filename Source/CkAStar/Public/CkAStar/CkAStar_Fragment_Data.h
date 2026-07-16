#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkAStar_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AStarSearchStatus : uint8
{
	Idle,
	InProgress,
	Complete,
	Failed,
	CostThresholdReached
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AStarSearchStatus);

// --------------------------------------------------------------------------------------------------------------------
