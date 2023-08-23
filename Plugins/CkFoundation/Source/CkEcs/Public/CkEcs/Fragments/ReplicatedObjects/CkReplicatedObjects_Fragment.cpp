#include "CkReplicatedObjects_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

FCk_Fragment_ReplicatedObjects_Params::
    FCk_Fragment_ReplicatedObjects_Params(
        FCk_ReplicatedObjects InReplicatedObjects)
        :_ReplicatedObjects(std::move(InReplicatedObjects))
{
}

}
