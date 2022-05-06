#include "register_types.h"

#include "thirdparty/godot/material_x_3d.h"

static Ref<MTLXLoader> resource_format_mtlx;

void initialize_material_x_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    GDREGISTER_CLASS(MTLXLoader);
    resource_format_mtlx.instantiate();
    ResourceLoader::add_resource_format_loader(resource_format_mtlx);
}

void uninitialize_material_x_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    ResourceLoader::remove_resource_format_loader(resource_format_mtlx);
    resource_format_mtlx.unref();
}