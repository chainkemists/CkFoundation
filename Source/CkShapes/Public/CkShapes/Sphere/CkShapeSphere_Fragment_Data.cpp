#include "CkShapeSphere_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
	FCk_ShapeSphere_Dimensions::
	operator==(
		const ThisType& InOther) const
	-> bool
{
	return FMath::IsNearlyEqual(_Radius, InOther._Radius);
}

// --------------------------------------------------------------------------------------------------------------------

