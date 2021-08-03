#include <MaterialXView/Material.h>

#include <MaterialXRenderGlsl/External/GLew/glew.h>
#include <MaterialXRenderGlsl/GLTextureHandler.h>
#include <MaterialXRenderGlsl/GLUtil.h>

#include <MaterialXRender/Util.h>

#include <MaterialXFormat/Util.h>

namespace
{

const float PI = std::acos(-1.0f);

} // anonymous namespace

//
// Material methods
//

bool Material::loadSource(const mx::FilePath& vertexShaderFile, const mx::FilePath& pixelShaderFile, bool hasTransparency)
{
    _hasTransparency = hasTransparency;

    std::string vertexShader = mx::readFile(vertexShaderFile);
    if (vertexShader.empty())
    {
        return false;
    }

    std::string pixelShader = mx::readFile(pixelShaderFile);
    if (pixelShader.empty())
    {
        return false;
    }

    // TODO:
    // Here we set new source code on the _glProgram without rebuilding
    // the _hwShader instance. So the _hwShader is not in sync with the
    // _glProgram after this operation.
    _glProgram = mx::GlslProgram::create();
    _glProgram->addStage(mx::Stage::VERTEX, vertexShader);
    _glProgram->addStage(mx::Stage::PIXEL, pixelShader);
    _glProgram->build();

    updateUniformsList();

    return true;
}

void Material::updateUniformsList()
{
    _uniformVariable.clear();
    if (!_glProgram)
    {
        return;
    }

    for (const auto& pair : _glProgram->getUniformsList())
    {
        _uniformVariable.insert(pair.first);
    }
}

void Material::clearShader()
{
    _hwShader = nullptr;
    _glProgram = nullptr;
    _uniformVariable.clear();
}

bool Material::generateShader(mx::GenContext& context)
{
    if (!_elem)
    {
        return false;
    }

    _hasTransparency = mx::isTransparentSurface(_elem, context.getShaderGenerator().getTarget());

    mx::GenContext materialContext = context;
    materialContext.getOptions().hwTransparency = _hasTransparency;

    // Initialize in case creation fails and throws an exception
    clearShader();

    _hwShader = createShader("Shader", materialContext, _elem);
    if (!_hwShader)
    {
        return false;
    }

    _glProgram = mx::GlslProgram::create();
    _glProgram->setStages(_hwShader);
    _glProgram->build();

    updateUniformsList();

    return true;
}

bool Material::generateShader(mx::ShaderPtr hwShader)
{
    _hwShader = hwShader;

    _glProgram = mx::GlslProgram::create();
    _glProgram->setStages(hwShader);
    _glProgram->build();

    updateUniformsList();

    return true;
}
