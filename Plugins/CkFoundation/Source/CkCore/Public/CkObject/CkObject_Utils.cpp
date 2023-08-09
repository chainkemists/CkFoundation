#include "CkObject_Utils.h"

void foo()
{
    UObject* u = nullptr;
    UCk_Utils_Object_UE::Request_CloneObject(u, u);
}