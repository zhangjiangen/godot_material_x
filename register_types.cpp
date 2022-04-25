#include "register_types.h"

#include "thirdparty/godot/material_x_3d.h"

static Ref<MTLXLoader> resource_format_mtlx;

void register_material_x_types() {
	GDREGISTER_CLASS(MTLXLoader);
	resource_format_mtlx.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_mtlx);
}

void unregister_material_x_types() {
	ResourceLoader::remove_resource_format_loader(resource_format_mtlx);
	resource_format_mtlx.unref();
}
