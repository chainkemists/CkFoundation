#include "CkInventory_DataOnly_RequestTraits.h"

#include "CkInventory/Inventory/CkInventory_Fragment.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

namespace ck
{
    auto TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>::Setup_PerShape(
        FCk_Handle_Inventory& InInventoryEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates) -> void
    {
        InInventoryEntity.Add<FTag_Inventory_DataOnly>();

        const auto BoundValue = (InParams.Get_BoundMode() != ECk_Inventory_DataOnly_BoundMode::Unbounded)
            ? InParams.Get_BoundLimit()
            : ck::Inventory::UnboundedBoundLimit;

        const auto AttrParams = FCk_Fragment_IntegerAttribute_ParamsData(
            TAG_IntegerAttribute_InventoryBoundMax, BoundValue);

        UCk_Utils_IntegerAttribute_UE::Add(InInventoryEntity, AttrParams, InReplicates);
    }
}
