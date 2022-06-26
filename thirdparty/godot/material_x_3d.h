#pragma once

#include "MaterialXCore/Generated.h"

#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/resource_format_loader.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

#include <MaterialXRenderGlsl/GLTextureHandler.h>
#include <MaterialXRenderGlsl/GLUtil.h>
#include <MaterialXRenderGlsl/TextureBaker.h>

#include <MaterialXRender/Harmonics.h>
#include <MaterialXRender/OiioImageLoader.h>
#include <MaterialXRender/StbImageLoader.h>
#include <MaterialXRender/Util.h>

#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXGenShader/ShaderTranslator.h>

#include <MaterialXFormat/Environ.h>
#include <MaterialXFormat/Util.h>

#include <MaterialXCore/Util.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/UnitSystem.h>
#include <MaterialXRenderGlsl/GlslProgram.h>

#include <iostream>

using namespace godot;
namespace mx = MaterialX;
class MTLXLoader : public ResourceFormatLoader {
	GDCLASS(MTLXLoader, ResourceFormatLoader);

	mx::ImageHandlerPtr imageHandler = mx::GLTextureHandler::create(mx::StbImageLoader::create());

protected:
	static void _bind_methods() {}

public:
	virtual Variant _load(const String &path, const String &original_path, bool use_sub_threads, int64_t cache_mode) const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	MTLXLoader() {}
};

using MaterialPtr = std::shared_ptr<class Material>;

class DocumentModifiers {
public:
	mx::StringMap remapElements;
	mx::StringSet skipElements;
	std::string filePrefixTerminator;
};