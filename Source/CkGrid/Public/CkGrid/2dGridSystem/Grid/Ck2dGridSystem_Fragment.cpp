#include "Ck2dGridSystem_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

ck::FFragment_2dGridSystem::FFragment_2dGridSystem() = default;

ck::FFragment_2dGridSystem::FFragment_2dGridSystem(FCk_Handle_SceneNode InPivot)
	: _CellEcsWorld(MakeUnique<ck::FEcsWorld>())
	, _Pivot(InPivot)
{
}

auto
	ck::FFragment_2dGridSystem::
	Request_CreateCellEntity()
	-> FCk_Handle
{
	if (NOT _CellEcsWorld.IsValid())
	{ return {}; }
	return UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_CellEcsWorld->Get_Registry());
}

auto
	ck::FFragment_2dGridSystem::
	Get_CellRegistry() const
	-> FCk_Registry
{
	if (NOT _CellEcsWorld.IsValid())
	{ return {}; }
	return _CellEcsWorld->Get_Registry();
}

// --------------------------------------------------------------------------------------------------------------------
