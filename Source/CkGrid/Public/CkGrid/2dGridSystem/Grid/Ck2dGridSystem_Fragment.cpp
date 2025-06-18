#include "Ck2dGridSystem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
	ck::FFragment_2dGridSystem_Current::
	Request_CreateCellEntity()
	-> FCk_Handle
{
	return UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_CellRegistry);
}

// --------------------------------------------------------------------------------------------------------------------
