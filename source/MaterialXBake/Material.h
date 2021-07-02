#ifndef MATERIALXVIEW_MATERIAL_H
#define MATERIALXVIEW_MATERIAL_H

#include <MaterialXRenderGlsl/GlslProgram.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/UnitSystem.h>

namespace mx = MaterialX;

using MaterialPtr = std::shared_ptr<class Material>;

class DocumentModifiers
{
  public:
    mx::StringMap remapElements;
    mx::StringSet skipElements;
    std::string filePrefixTerminator;
};

class Material
{
  public:
    Material() :
        _hasTransparency(false)
    {
    }
    ~Material() { }

    static MaterialPtr create()
    {
        return std::make_shared<Material>();
    }

    /// Return the document associated with this material
    mx::DocumentPtr getDocument() const
    {
        return _doc;
    }

    /// Set the renderable element associated with this material
    void setDocument(mx::DocumentPtr doc)
    {
        _doc = doc;
    }

    /// Return the renderable element associated with this material
    mx::TypedElementPtr getElement() const
    {
        return _elem;
    }

    /// Set the renderable element associated with this material
    void setElement(mx::TypedElementPtr val)
    {
        _elem = val;
    }

    /// Return the material node associated with this material
    mx::NodePtr getMaterialNode() const
    {
        return _materialNode;
    }

    /// Set the material node associated with this material
    void setMaterialNode(mx::NodePtr node)
    {
        _materialNode = node;
    }

    /// Get any associated udim identifier
    const std::string& getUdim()
    {
        return _udim;
    }

    /// Set udim identifier
    void setUdim(const std::string& val)
    {
        _udim = val;
    }

    /// Load shader source from file.
    bool loadSource(const mx::FilePath& vertexShaderFile,
                    const mx::FilePath& pixelShaderFile,
                    bool hasTransparency);

    /// Generate a shader from our currently stored element and
    /// the given generator context.
    bool generateShader(mx::GenContext& context);

    /// Generate a shader from the given hardware shader.
    bool generateShader(mx::ShaderPtr hwShader);

    /// Copy shader from one material to this one
    void copyShader(MaterialPtr material)
    {
        _hwShader = material->_hwShader;
        _glProgram = material->_glProgram;
    }

    /// Return the underlying hardware shader.
    mx::ShaderPtr getShader() const
    {
        return _hwShader;
    }

    /// Return the underlying GLSL program.
    mx::GlslProgramPtr getProgram() const
    {
        return _glProgram;
    }

    /// Return true if this material has transparency.
    bool hasTransparency() const
    {
        return _hasTransparency;
    }

  protected:
    void clearShader();
    void updateUniformsList();

  protected:
    mx::ShaderPtr _hwShader;
    mx::GlslProgramPtr _glProgram;

    mx::DocumentPtr _doc;
    mx::TypedElementPtr _elem;
    mx::NodePtr _materialNode;

    std::string _udim;
    bool _hasTransparency;
    mx::StringSet _uniformVariable;

    mx::ImageVec _boundImages;
};

#endif // MATERIALXVIEW_MATERIAL_H
