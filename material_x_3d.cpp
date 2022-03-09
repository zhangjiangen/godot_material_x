#include "material_x_3d.h"

mx::FileSearchPath getDefaultSearchPath() {
	mx::FilePath modulePath = mx::FilePath::getModulePath();
	mx::FilePath installRootPath = modulePath.getParentPath();
	mx::FilePath devRootPath =
			installRootPath.getParentPath().getParentPath().getParentPath();

	mx::FileSearchPath searchPath;
	searchPath.append(installRootPath);
	searchPath.append(devRootPath);

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

RES MTLXLoader::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	mx::FilePath materialFilename =
			"resources/Materials/Examples/StandardSurface/"
			"standard_surface_default.mtlx";
	mx::FileSearchPath searchPath = getDefaultSearchPath();
	mx::FilePathVec libraryFolders = { "libraries" };

	int bakeWidth = -1;
	int bakeHeight = -1;
	std::string bakeFormat;
	DocumentModifiers modifiers;
	bool bakeHdr = false;

	std::string bakeFilename;

	mx::ImageHandlerPtr imageHandler =
			mx::GLTextureHandler::create(mx::StbImageLoader::create());

	materialFilename = p_path.utf8().get_data();
	// Defaults to PNG
	if (bakeFormat == std::string("HDR") || bakeFormat == std::string("hdr")) {
		bakeHdr = true;
	} else if (bakeFormat == std::string("EXR") || bakeFormat == std::string("exr")) {
#if MATERIALX_BUILD_OIIO
		imageHandler->addLoader(mx::OiioImageLoader::create());
#else
		std::cout << "OpenEXR is not supported\n";
		return RES();
#endif
	} else if (bakeFormat == std::string("TIFF") || bakeFormat == std::string("TIF") || bakeFormat == std::string("tif") || bakeFormat == std::string("tif")) {
#if MATERIALX_BUILD_OIIO
		imageHandler->addLoader(mx::OiioImageLoader::create());
#else
		std::cout << "Tiff is not supported\n";
		return RES();
#endif
	}

	// const std::string options =
	// 		" Options: \n"
	// 		"    --material [FILENAME]          Specify the filename of the MTLX "
	// 		"document to be baked to a filename\n"
	// 		"(defaults to png)\n"
	// 		"    --path [FILEPATH]              Specify an additional absolute search "
	// 		"path location (e.g. '/projects/MaterialX').  This path will be queried "
	// 		"when locating standard data libraries, XInclude references, and "
	// 		"referenced images.\n"
	// 		"    --library [FILEPATH]           Specify an additional relative path to "
	// 		"a custom data library folder (e.g. 'libraries/custom').  MaterialX files "
	// 		"at the root of this folder will be included in all content documents.\n"
	// 		"    --bakeWidth [INTEGER]          Specify the target width for texture baking (defaults to maximum image width of the source document)\n"
	// 		"    --bakeHeight [INTEGER]         Specify the target height for texture baking (defaults to maximum image height of the source document)\n"
	// 		"    --bakeFilename [STRING]        Specify the output document filename for texture baking\n"
	// 		"    --remap [TOKEN1:TOKEN2]        Specify the remapping from one token "
	// 		"to another when MaterialX document is loaded\n"
	// 		"    --skip [NAME]                  Specify to skip elements matching the "
	// 		"given name attribute\n"
	// 		"    --terminator [STRING]          Specify to enforce the given "
	// 		"terminator string for file prefixes\n"
	// 		"    --help                         Display the complete list of "
	// 		"command-line options\n";
	//
	//     if (token == "--path") {
	//         searchPath.append(mx::FileSearchPath(nextToken));
	//     } else if (token == "--library") {
	//         libraryFolders.push_back(nextToken);
	//     } else if (token == "--bakeWidth") {
	//         parseToken(nextToken, "integer", bakeWidth);
	//     } else if (token == "--bakeHeight") {
	//         parseToken(nextToken, "integer", bakeHeight);
	//     } else if (token == "--bakeFilename") {
	//         parseToken(nextToken, "string", bakeFilename);
	//     } else if (token == "--remap") {
	//         mx::StringVec vec = mx::splitString(nextToken, ":");
	//         if (vec.size() == 2) {
	//             modifiers.remapElements[vec[0]] = vec[1];
	//         } else if (!nextToken.empty()) {
	//             std::cout << "Unable to parse token following command-line option: "
	//                         << token << std::endl;
	//         }
	//     } else if (token == "--skip") {
	//         modifiers.skipElements.insert(nextToken);
	//     } else if (token == "--terminator") {
	//         modifiers.filePrefixTerminator = nextToken;

	imageHandler->setSearchPath(searchPath);

	std::vector<MaterialPtr> materials;

	// Document management
	mx::DocumentPtr dependLib = mx::createDocument();
	mx::StringSet skipLibraryFiles;
	mx::DocumentPtr stdLib;
	mx::StringSet xincludeFiles;

	mx::StringVec distanceUnitOptions;
	mx::LinearUnitConverterPtr distanceUnitConverter;

	bool bakeAverage = false;
	bool bakeOptimize = true;

	mx::UnitConverterRegistryPtr unitRegistry =
			mx::UnitConverterRegistry::create();

	mx::GenContext context = mx::GlslShaderGenerator::create();

	// Initialize search paths.
	for (const mx::FilePath &path : searchPath) {
		context.registerSourceCodeSearchPath(path / "libraries");
	}

	std::vector<MaterialPtr> newMaterials;
	// Load source document.
	mx::DocumentPtr doc = mx::createDocument();
	try {
		stdLib = mx::createDocument();
		xincludeFiles = mx::loadLibraries(libraryFolders, searchPath, stdLib);
		// Import libraries.
		if (xincludeFiles.empty()) {
			std::cerr << "Could not find standard data libraries on the given "
						 "search path: "
					  << searchPath.asString() << std::endl;
			return RES();
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
		return RES();
	}

	doc->importLibrary(stdLib);

	MaterialX::FilePath parentPath = materialFilename.getParentPath();
	searchPath.append(materialFilename.getParentPath());

	// Set up read options.
	mx::XmlReadOptions readOptions;
	readOptions.readXIncludeFunction = [](mx::DocumentPtr doc,
											   const mx::FilePath &materialFilename,
											   const mx::FileSearchPath &searchPath,
											   const mx::XmlReadOptions *newReadoptions) {
		mx::FilePath resolvedFilename = searchPath.find(materialFilename);
		if (resolvedFilename.exists()) {
			readFromXmlFile(doc, resolvedFilename, searchPath, newReadoptions);
		} else {
			std::cerr << "Include file not found: " << materialFilename.asString()
					  << std::endl;
		}
	};
	mx::readFromXmlFile(doc, materialFilename, searchPath, &readOptions);

	// Apply modifiers to the content document.
	applyModifiers(doc, modifiers);

	// Validate the document.
	std::string message;
	if (!doc->validate(&message)) {
		std::cerr << "*** Validation warnings for "
				  << materialFilename.getBaseName() << " ***" << std::endl;
		std::cerr << message;
		std::cout << message << std::endl;
	}

	if (!doc) {
		return RES();
	}
	imageHandler->setSearchPath(searchPath);

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
	baker->setAverageImages(bakeAverage);
	baker->setOptimizeConstants(bakeOptimize);

	// Assign our existing image handler, releasing any existing render
	// resources for cached images.
	imageHandler->releaseRenderResources();
	baker->setImageHandler(imageHandler);
	mx::FilePath fileName = bakeFilename;
	if (fileName.isEmpty()) {
		fileName = materialFilename.getBaseName();
	}
	if (fileName.getExtension() != mx::MTLX_EXTENSION) {
		fileName.addExtension(mx::MTLX_EXTENSION);
	}
	bakeFilename = fileName;

	// Bake all materials in the active document.
	try {
		baker->bakeAllMaterials(doc, searchPath, bakeFilename);
	} catch (std::exception &e) {
		std::cerr << "Error in texture baking: " << e.what() << std::endl;
	}

	// Release any render resources generated by the baking process.
	imageHandler->releaseRenderResources();

    Ref<StandardMaterial3D> mat;
    mat.instantiate();

	return mat;
}

void MTLXLoader::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("mtlx");
}

bool MTLXLoader::handles_type(const String &p_type) const {
	return (p_type == "StandardMaterial3D");
}

String MTLXLoader::get_resource_type(const String &p_path) const {
	return "StandardMaterial3D";
}
