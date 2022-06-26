#include "material_x_3d.h"

#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/config_file.hpp"
#include "godot_cpp/classes/directory.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/resource_saver.hpp"
#include "godot_cpp/classes/image.hpp"

mx::FileSearchPath getDefaultSearchPath(mx::GenContext context) {
	mx::FilePath modulePath = mx::FilePath::getModulePath();
	mx::FilePath installRootPath = modulePath.getParentPath();
	mx::FilePath devRootPath =
			installRootPath.getParentPath().getParentPath().getParentPath();

	mx::FileSearchPath searchPath;
	searchPath.append(installRootPath);
	searchPath.append(devRootPath);
	searchPath.append(ProjectSettings::get_singleton()->globalize_path("res://").utf8().get_data());
	for (const mx::FilePath &path : searchPath) {
		context.registerSourceCodeSearchPath(path / "libraries");
	}
	return searchPath;
}

void applyModifiers(mx::DocumentPtr doc, const DocumentModifiers &modifiers) {
	for (mx::ElementPtr elem : doc->traverseTree()) {
		if (modifiers.remapElements.count(elem->getCategory())) {
			elem->setCategory(modifiers.remapElements.at(elem->getCategory()));
		}
		if (modifiers.remapElements.count(elem->getName())) {
			elem->setName(modifiers.remapElements.at(elem->getName()));
		}
		mx::StringVec attrNames = elem->getAttributeNames();
		for (const std::string &attrName : attrNames) {
			if (modifiers.remapElements.count(elem->getAttribute(attrName))) {
				elem->setAttribute(
						attrName, modifiers.remapElements.at(elem->getAttribute(attrName)));
			}
		}
		if (elem->hasFilePrefix() && !modifiers.filePrefixTerminator.empty()) {
			std::string filePrefix = elem->getFilePrefix();
			if (!mx::stringEndsWith(filePrefix, modifiers.filePrefixTerminator)) {
				elem->setFilePrefix(filePrefix + modifiers.filePrefixTerminator);
			}
		}
		std::vector<mx::ElementPtr> children = elem->getChildren();
		for (mx::ElementPtr child : children) {
			if (modifiers.skipElements.count(child->getCategory()) ||
					modifiers.skipElements.count(child->getName())) {
				elem->removeChild(child->getName());
			}
		}
	}

	// Remap references to unimplemented shader nodedefs.
	for (mx::NodePtr materialNode : doc->getMaterialNodes()) {
		for (mx::NodePtr shader : getShaderNodes(materialNode)) {
			mx::NodeDefPtr nodeDef = shader->getNodeDef();
			if (nodeDef && !nodeDef->getImplementation()) {
				std::vector<mx::NodeDefPtr> altNodeDefs =
						doc->getMatchingNodeDefs(nodeDef->getNodeString());
				for (mx::NodeDefPtr altNodeDef : altNodeDefs) {
					if (altNodeDef->getImplementation()) {
						shader->setNodeDefString(altNodeDef->getName());
					}
				}
			}
		}
	}
}

Variant get_value_as_material_x_variant(mx::InputPtr p_input) {
	mx::ValuePtr value = p_input->getValue();
	if (value->getTypeString() == "float") {
		return value->asA<float>();
	} else if (value->getTypeString() == "integer") {
		return value->asA<int>();
	} else if (value->getTypeString() == "boolean") {
		return value->asA<bool>();
	} else if (value->getTypeString() == "color3") {
		mx::Color3 color = value->asA<mx::Color3>();
		return Color(color[0], color[1], color[2]);
	} else if (value->getTypeString() == "color4") {
		mx::Color4 color = value->asA<mx::Color4>();
		return Color(color[0], color[1], color[2], color[3]);
	} else if (value->getTypeString() == "vector2") {
		mx::Vector2 vector_2 = value->asA<mx::Vector2>();
		return Vector2(vector_2[0], vector_2[1]);
	} else if (value->getTypeString() == "vector3") {
		mx::Vector3 vector_3 = value->asA<mx::Vector3>();
		return Vector3(vector_3[0], vector_3[1], vector_3[2]);
	} else if (value->getTypeString() == "vector4") {
		mx::Vector4 vector_4 = value->asA<mx::Vector4>();
		return Color(vector_4[0], vector_4[1], vector_4[2], vector_4[3]);
	} else if (value->getTypeString() == "matrix33") {
		// Matrix33 m = value->asA<Matrix33>();
		// TODO: fire 2022-03-11 add basis
	} else if (value->getTypeString() == "matrix44") {
		// Matrix44 m = value->asA<Matrix44>();
		// TODO: fire 2022-03-11 add transform
	}
	return Variant();
}

Error load_mtlx_document(mx::DocumentPtr p_doc, String p_path, mx::GenContext context) {
	mx::FilePath materialFilename = ProjectSettings::get_singleton()->globalize_path(p_path).utf8().get_data();
	std::vector<MaterialPtr> materials;
	mx::DocumentPtr dependLib = mx::createDocument();
	mx::StringSet skipLibraryFiles;
	mx::DocumentPtr stdLib;
	mx::StringSet xincludeFiles;

	mx::StringVec distanceUnitOptions;
	mx::LinearUnitConverterPtr distanceUnitConverter;

	mx::UnitConverterRegistryPtr unitRegistry =
			mx::UnitConverterRegistry::create();
	// Initialize search paths.
	mx::FileSearchPath searchPath = getDefaultSearchPath(context);
	try {
		stdLib = mx::createDocument();
		mx::FilePathVec libraryFolders;
		libraryFolders.push_back(ProjectSettings::get_singleton()->globalize_path("res://libraries").utf8().get_data());
		mx::StringSet xincludeFiles = mx::loadLibraries(libraryFolders, searchPath, stdLib);
		// Import libraries.
		if (xincludeFiles.empty()) {
			std::cerr << "Could not find standard data libraries on the given "
						 "search path: "
					  << searchPath.asString() << std::endl;
			return FAILED;
		}

		// Initialize color management.
		mx::DefaultColorManagementSystemPtr cms =
				mx::DefaultColorManagementSystem::create(
						context.getShaderGenerator().getTarget());
		cms->loadLibrary(stdLib);
		context.getShaderGenerator().setColorManagementSystem(cms);

		// Initialize unit management.
		mx::UnitSystemPtr unitSystem =
				mx::UnitSystem::create(context.getShaderGenerator().getTarget());
		unitSystem->loadLibrary(stdLib);
		unitSystem->setUnitConverterRegistry(unitRegistry);
		context.getShaderGenerator().setUnitSystem(unitSystem);
		context.getOptions().targetDistanceUnit = "meter";

		// Initialize unit management.
		mx::UnitTypeDefPtr distanceTypeDef = stdLib->getUnitTypeDef("distance");
		distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
		unitRegistry->addUnitConverter(distanceTypeDef, distanceUnitConverter);
		mx::UnitTypeDefPtr angleTypeDef = stdLib->getUnitTypeDef("angle");
		mx::LinearUnitConverterPtr angleConverter =
				mx::LinearUnitConverter::create(angleTypeDef);
		unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

		// Create the list of supported distance units.
		auto unitScales = distanceUnitConverter->getUnitScale();
		distanceUnitOptions.resize(unitScales.size());
		for (auto unitScale : unitScales) {
			int location = distanceUnitConverter->getUnitAsInteger(unitScale.first);
			distanceUnitOptions[location] = unitScale.first;
		}

		// Clear user data on the generator.
		context.clearUserData();
	} catch (std::exception &e) {
		std::cerr << "Failed to load standard data libraries: " << e.what()
				  << std::endl;
		return FAILED;
	}

	p_doc->importLibrary(stdLib);

	MaterialX::FilePath parentPath = materialFilename.getParentPath();
	searchPath.append(materialFilename.getParentPath());

	// Set up read options.
	mx::XmlReadOptions readOptions;
	readOptions.readXIncludeFunction = [](mx::DocumentPtr p_doc,
											   const mx::FilePath &materialFilename,
											   const mx::FileSearchPath &searchPath,
											   const mx::XmlReadOptions *newReadoptions) {
		mx::FilePath resolvedFilename = searchPath.find(materialFilename);
		if (resolvedFilename.exists()) {
			readFromXmlFile(p_doc, resolvedFilename, searchPath, newReadoptions);
		} else {
			std::cerr << "Include file not found: " << materialFilename.asString()
					  << std::endl;
		}
	};
	mx::readFromXmlFile(p_doc, materialFilename, searchPath, &readOptions);

	DocumentModifiers modifiers;
	// TODO: fire 2022-03-11 Does nothing yet.
	// Apply modifiers to the content document.
	applyModifiers(p_doc, modifiers);

	// Validate the document.
	std::string message;
	ERR_FAIL_COND_V_MSG(!p_doc->validate(&message), FAILED, String("Validation warnings for ") + String(message.c_str()));
	return OK;
}

Variant MTLXLoader::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int64_t p_cache_mode) const {
	mx::GenContext context = mx::GlslShaderGenerator::create();
	String folder = "res://.godot/imported/" + p_original_path.get_file().get_basename() +
			"-" + p_original_path.md5_text() + "/";
	String mtlx = folder + p_original_path.get_file().get_basename() +
			"-" + p_original_path.md5_text() + ".mtlx";
	std::string bakeFilename = ProjectSettings::get_singleton()->globalize_path(mtlx).utf8().get_data();
	mx::FileSearchPath searchPath = getDefaultSearchPath(context);
	mx::FilePath materialFilename = ProjectSettings::get_singleton()->globalize_path(p_path).utf8().get_data();
	searchPath.append(materialFilename.getParentPath());
	mx::FilePathVec libraryFolders;
	libraryFolders.push_back(ProjectSettings::get_singleton()->globalize_path(p_original_path.get_base_dir()).utf8().get_data());
	mx::DocumentPtr stdLib;
	stdLib = mx::createDocument();
	mx::StringSet xincludeFiles = mx::loadLibraries(libraryFolders, searchPath, stdLib);
	{
		// Load source document.
		mx::DocumentPtr doc = mx::createDocument();
		Error err;
		try {
			err = load_mtlx_document(doc, p_path, context);
		} catch (std::exception &e) {
			ERR_PRINT(String("Can't load materials. Error: ") + String(e.what()));
			return Ref<Resource>();
		}

		if (err != OK) {
			return Ref<Resource>();
		}
		int bakeWidth = -1;
		int bakeHeight = -1;
		std::string bakeFormat;
		bool bakeHdr = false;
		imageHandler->setSearchPath(searchPath);

		if (bakeFormat == std::string("EXR") || bakeFormat == std::string("exr")) {
			bakeHdr = true;
#if MATERIALX_BUILD_OIIO
			imageHandler->addLoader(mx::OiioImageLoader::create());
#else
			ERR_PRINT("OpenEXR is not supported");
			return ERR_UNAVAILABLE;
#endif
		}
		// Compute baking resolution.
		mx::ImageVec imageVec = imageHandler->getReferencedImages(doc);
		auto maxImageSize = mx::getMaxDimensions(imageVec);
		if (bakeWidth == -1) {
			bakeWidth = std::max(maxImageSize.first, (unsigned int)4);
		}
		if (bakeHeight == -1) {
			bakeHeight = std::max(maxImageSize.second, (unsigned int)4);
		}

		// Construct a texture baker.
		mx::Image::BaseType baseType =
				bakeHdr ? mx::Image::BaseType::FLOAT : mx::Image::BaseType::UINT8;
		mx::TextureBakerPtr baker =
				mx::TextureBaker::create(bakeWidth, bakeHeight, baseType);
		baker->setupUnitSystem(stdLib);
		baker->setDistanceUnit(context.getOptions().targetDistanceUnit);
		bool bakeAverage = false;
		bool bakeOptimize = true;
		baker->setAverageImages(bakeAverage);
		baker->setOptimizeConstants(bakeOptimize);
		Directory d;
		d.make_dir_recursive(folder);

		// Bake all materials in the active document.
		try {
			baker->bakeAllMaterials(doc, searchPath, bakeFilename);
		} catch (std::exception &e) {
			ERR_PRINT("Can't bake materials.");
		}

		// Release any render resources generated by the baking process.
		imageHandler->releaseRenderResources();
	}
	mx::DocumentPtr new_doc = mx::createDocument();
	Error err;
	try {
		err = load_mtlx_document(new_doc, bakeFilename.c_str(), mx::GlslShaderGenerator::create());
	} catch (std::exception &e) {
		ERR_PRINT("Can't load materials.");
		return Ref<Resource>();
	}

	xincludeFiles = mx::loadLibraries(libraryFolders, searchPath, stdLib);
	if (err != OK) {
		return Ref<Resource>();
	}
	std::vector<mx::TypedElementPtr> renderable_materials;
	findRenderableElements(new_doc, renderable_materials);
	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	for (size_t i = 0; i < renderable_materials.size(); i++) {
		const mx::TypedElementPtr &element = renderable_materials[i];
		if (!element || !element->isA<mx::Node>()) {
			continue;
		}
		const mx::NodePtr &node = element->asA<mx::Node>();
		for (mx::NodePtr node_inputs : mx::getShaderNodes(node)) {
			const std::string &node_name = node_inputs->getName();
			// print(String("MaterialX material name " + String(node_name.c_str())));
			const std::string &category_name = node_inputs->getCategory();
			// print(String("MaterialX material type " + String(category_name.c_str())));
			for (mx::InputPtr input : node_inputs->getInputs()) {
				const std::string &input_name = input->getName();
				// print(String("MaterialX input " + String(input_name.c_str())));
				if (input->hasOutputString()) {
					mx::NodeGraphPtr node_graph = new_doc->getChildOfType<mx::NodeGraph>(input->getNodeGraphString());
					if (!node_graph) {
						continue;
					}
					mx::OutputPtr output = node_graph->getOutput(input->getOutputString());
					mx::NodePtr image_node = node_graph->getNode(output->getNodeName());
					if (!image_node) {
						continue;
					}
					if (!image_node->getInputs().size()) {
						continue;
					}
					String filepath = image_node->getInputs()[0]->getValueString().c_str();
					if (input_name == "normal") {
						mx::NodePtr normal_image_node = node_graph->getNode(image_node->getInputs()[0]->getNodeName());
						if (!normal_image_node->getInputs().size()) {
							continue;
						}
						filepath = normal_image_node->getInputs()[0]->getValueString().c_str();
					}
					filepath = filepath.replace("\\", "/");
					filepath = ProjectSettings::get_singleton()->localize_path(filepath);
					filepath = filepath.lstrip("res://");
					// String line = String("MaterialX attribute filepath " + filepath);
					// print(String("MaterialX attribute name " + String(input->getOutputString().c_str())));
					// print(line);
					Ref<ImageTexture> tex;
					tex.instantiate();
					Ref<Image> mtlx_image;
					mtlx_image.instantiate();
					err = mtlx_image->load(filepath);
					if (err == OK) {
						mtlx_image->generate_mipmaps();
						tex->create_from_image(mtlx_image);
					}
					if (input_name == "base_color") {
						mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_TEXTURE_FORCE_SRGB, true);
						mat->set_texture(BaseMaterial3D::TextureParam::TEXTURE_ALBEDO, tex);
					} else if (input_name == "metallic") {
						if (mat->get_metallic() == 0.0f) {
							mat->set_metallic(1.0f);
						}
						mat->set_metallic_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_BLUE);
						mat->set_texture(BaseMaterial3D::TEXTURE_METALLIC, tex);
					} else if (input_name == "roughness") {
						mat->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, tex);
						mat->set_roughness_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_GREEN);
					} else if (input_name == "normal") {
						mat->set_feature(StandardMaterial3D::FEATURE_NORMAL_MAPPING, true);
						mat->set_texture(StandardMaterial3D::TEXTURE_NORMAL, tex);
					} else if (input_name == "emissive") {
						mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
						mat->set_texture(BaseMaterial3D::TEXTURE_EMISSION, tex);
					} else if (input_name == "occlusion") {
						mat->set_texture(BaseMaterial3D::TEXTURE_AMBIENT_OCCLUSION, tex);
						mat->set_ao_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
					}
					continue;
				}
				Variant v = get_value_as_material_x_variant(input);
				// <input name="transmission" type="float" value="0" />
				// <input name="specular_color" type="color3" value="1, 1, 1" />
				// <input name="ior" type="float" value="1.5" />
				// <input name="alpha" type="float" value="1" />
				// <input name="sheen_color" type="color3" value="0, 0, 0" />
				// <input name="sheen_roughness" type="float" value="0" />
				// <input name="clearcoat" type="float" value="0" />
				// <input name="clearcoat_roughness" type="float" value="0" />
				// <input name="clearcoat_normal" type="vector3" value="0, 0, 1" />
				// <input name="thickness" type="float" value="0" />
				// <input name="attenuation_distance" type="float" value="100000" />
				// <input name="attenuation_color" type="color3" value="0, 0, 0" />
				if (input_name == "base_color") {
					Color c = mat->get_albedo();
					Color variant_color = v;
					c.r = variant_color.r;
					c.g = variant_color.g;
					c.b = variant_color.b;
					c.a = variant_color.a;
					mat->set_albedo(c);
				} else if (input_name == "metallic") {
					mat->set_metallic(v);
				} else if (input_name == "roughness") {
					mat->set_roughness(v);
				} else if (input_name == "specular") {
					mat->set_specular(v);
				} else if (input_name == "normal") {
					mat->set_feature(StandardMaterial3D::FEATURE_NORMAL_MAPPING, true);
				} else if (input_name == "emissive") {
					mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
					mat->set_emission(v);
				} else if (input_name == "alpha_mode") {
					if (v) {
						mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
						mat->set_alpha_antialiasing(BaseMaterial3D::ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE);
						mat->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_ALWAYS);
					}
				} else if (input_name == "alpha_cutoff") {
					mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
					mat->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_ALWAYS);
					mat->set_alpha_scissor_threshold(v);
				} else if (input_name == "base_color") {
					Color c = mat->get_albedo();
					c.a = c.a * Color(v).a;
					mat->set_albedo(c);
				}
				// print(String("MaterialX attribute value ") + String(v));
			}
		}
		break;
	}
	Ref<ConfigFile> config;
	config.instantiate();
	String new_path = String("res://.godot/imported/") + p_original_path.get_file() + "-" + p_original_path.md5_text() + ".res";
	Ref<Directory> dir_access;
	dir_access.instantiate();
	if (dir_access->file_exists(p_original_path.get_basename() + ".import")) {
		config->load(p_original_path.get_basename() + ".import");
	}
	config->set_value("remap", "importer", "material");
	config->set_value("remap", "type", "StandardMaterial3D");
	config->set_value("remap", "path", new_path);
	config->set_value("deps", "source_file", p_original_path);
	config->set_value("deps", "dest_files", new_path);
	config->save(p_original_path.get_basename() + ".import");
	ResourceSaver saver;
 	saver.save(new_path, mat);
	return OK;
}

PackedStringArray MTLXLoader::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("mtlx");
	return extensions;
}
