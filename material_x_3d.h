#pragma once

#include "source/MaterialXCore/Generated.h"

#include "core/io/resource_loader.h"
#include "scene/resources/material.h"

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

namespace mx = MaterialX;
class MTLXLoader : public ResourceFormatLoader {

	mx::ImageHandlerPtr imageHandler = mx::GLTextureHandler::create(mx::StbImageLoader::create());

public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
	MTLXLoader() {}
};


using MaterialPtr = std::shared_ptr<class Material>;

class DocumentModifiers {
public:
	mx::StringMap remapElements;
	mx::StringSet skipElements;
	std::string filePrefixTerminator;
};