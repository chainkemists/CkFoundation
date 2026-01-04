#include "CkShapeCapsule_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
	FCk_ShapeCapsule_Dimensions::
	operator==(
		const ThisType& InOther) const
	-> bool
{
	return FMath::IsNearlyEqual(_HalfHeight, InOther._HalfHeight) &&
		FMath::IsNearlyEqual(_Radius, InOther._Radius);
}

// --------------------------------------------------------------------------------------------------------------------

