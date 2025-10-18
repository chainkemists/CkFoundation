namespace utils_prefab
{
    FCk_Handle
    Create_ProbeNode(
        FCk_Handle_Transform InOwner,
        FCk_AnyShape InShape,
        FCk_Fragment_Probe_ParamsData InProbeParams,
        FTransform InLocalTransform = FTransform(),
        FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto SceneNode = utils_scene_node::Create(InOwner, InLocalTransform);
        auto Entity = SceneNode.H();

        utils_shapes::Add(Entity, InShape);

        auto TransformHandle = Entity.To_FCk_Handle_Transform();
        utils_probe::Add(TransformHandle, InProbeParams, InDebugInfo);

        return Entity;
    }

    FCk_Handle
    Create_ProbeNode_Box(
        FCk_Handle_Transform InOwner,
        FVector InHalfExtents,
        FCk_Fragment_Probe_ParamsData InProbeParams,
        FTransform InLocalTransform = FTransform(),
        FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto SceneNode = utils_scene_node::Create(InOwner, InLocalTransform);
        auto Entity = SceneNode.H();
        auto TransformHandle = Entity.To_FCk_Handle_Transform();

        utils_probe::Add_Box(TransformHandle, InHalfExtents, InProbeParams, InDebugInfo);

        return Entity;
    }

    FCk_Handle
    Create_ProbeNode_Sphere(
        FCk_Handle_Transform InOwner,
        float32 InRadius,
        FCk_Fragment_Probe_ParamsData InProbeParams,
        FTransform InLocalTransform = FTransform(),
        FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto SceneNode = utils_scene_node::Create(InOwner, InLocalTransform);
        auto Entity = SceneNode.H();
        auto TransformHandle = Entity.To_FCk_Handle_Transform();

        utils_probe::Add_Sphere(TransformHandle, InRadius, InProbeParams, InDebugInfo);

        return Entity;
    }

    FCk_Handle
    Create_ProbeNode_Capsule(
        FCk_Handle_Transform InOwner,
        float32 InHalfHeight,
        float32 InRadius,
        FCk_Fragment_Probe_ParamsData InProbeParams,
        FTransform InLocalTransform = FTransform(),
        FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto SceneNode = utils_scene_node::Create(InOwner, InLocalTransform);
        auto Entity = SceneNode.H();
        auto TransformHandle = Entity.To_FCk_Handle_Transform();

        utils_probe::Add_Capsule(TransformHandle, InHalfHeight, InRadius, InProbeParams, InDebugInfo);

        return Entity;
    }

    FCk_Handle
    Create_ProbeNode_Cylinder(
        FCk_Handle_Transform InOwner,
        float32 InHalfHeight,
        float32 InRadius,
        FCk_Fragment_Probe_ParamsData InProbeParams,
        FTransform InLocalTransform = FTransform(),
        FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto SceneNode = utils_scene_node::Create(InOwner, InLocalTransform);
        auto Entity = SceneNode.H();
        auto TransformHandle = Entity.To_FCk_Handle_Transform();

        utils_probe::Add_Cylinder(TransformHandle, InHalfHeight, InRadius, InProbeParams, InDebugInfo);

        return Entity;
    }
}