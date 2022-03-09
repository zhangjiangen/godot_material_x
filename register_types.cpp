#include "register_types.h"

#include "material_x_3d.h"

static Ref<MTLXLoader> resource_format_mtlx;

void register_MaterialX_types() {
	resource_format_mtlx.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_mtlx);
}

void unregister_MaterialX_types() {
}
