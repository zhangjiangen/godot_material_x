#!python

import os, sys, platform, json, subprocess

def add_sources(sources, dirpath, extension):
    for f in os.listdir(dirpath):
        if f.endswith("." + extension):
            sources.append(dirpath + "/" + f)


def replace_flags(flags, replaces):
    for k, v in replaces.items():
        if k in flags:
            flags[flags.index(k)] = v


env = Environment()
opts = Variables(["customs.py"], ARGUMENTS)
opts.Update(env)

env = SConscript("godot-cpp/SConstruct")
replace_flags(env["CCFLAGS"], {
    "-mios-simulator-version-min=10.0": "-mios-simulator-version-min=11.0",
    "-miphoneos-version-min=10.0": "-miphoneos-version-min=11.0",
})
env = env.Clone()

# Patch mingw SHLIBSUFFIX.
if env["platform"] == "windows" and env["use_mingw"]:
    env["SHLIBSUFFIX"] = ".dll"

opts.Update(env)

target = env["target"]
result_path = os.path.join("bin", "extension", "material_x" if env["target"] == "release" else "material_x_debug")

# Our includes and sources
env.Append(CPPPATH=["include"])
env.Append(CPPPATH=["source"])
env.Append(CPPPATH=["thirdparty/godot"])
env.Append(CPPPATH=["godot-cpp/include"])
env.Append(CPPPATH=["godot-cpp/gen/include"])

sources = []
sources.append(
    [
        "thirdparty/godot/material_x_3d.cpp",
        Glob("source/MaterialXBake/*.cpp"),
        Glob("source/MaterialXCore/*.cpp"),
        Glob("source/MaterialXFormat/*.cpp"),
        Glob("source/MaterialXFormat/PugiXML/*.cpp"),
        Glob("source/MaterialXGenShader/*.cpp"),
        Glob("source/MaterialXGenShader/Nodes/*.cpp"),
        Glob("source/MaterialXGenGlsl/*.cpp"),
        Glob("source/MaterialXGenGlsl/Nodes/*.cpp"),
        Glob("source/MaterialXRender/*.cpp"),
        Glob("source/MaterialXRenderGlsl/External/GLew/*.cpp"),
        Glob("source/MaterialXRenderGlsl/*.cpp"),
        Glob("source/MaterialXRenderHw/WindowWrapper.cpp"),
    ]
)

if env["platform"] == "linuxbsd":
    env.Append(LIBS=["libXt"])
    sources.append(["source/MaterialXRenderHw/SimpleWindowLinux.cpp"])
elif env["platform"] == "osx":
    sources.append(["source/MaterialXRenderHw/SimpleWindowMac.cpp"])
elif env["platform"] == "windows":
    sources.append(["source/MaterialXRenderHw/SimpleWindowWindows.cpp"])

if env["platform"] == "osx":
    env.Append(LINKFLAGS=["-framework", "OpenGL", "-framework", "Foundation", "-framework", "Cocoa", "-lGLU", "-lglut"])
elif env["platform"] == "windows":
    env.Append(LINKFLAGS=["-lopengl32", "-lglu32", "-lgdi32"])

env.Append(CPPDEFINES=["GL_SILENCE_DEPRECATION"])

sources.append("thirdparty/godot/init_gdextension.cpp")

# Make the shared library
result_name = "material_x.{}.{}.{}{}".format(env["platform"], env["target"], env["arch_suffix"], env["SHLIBSUFFIX"])

library = env.SharedLibrary(target=os.path.join(result_path, "lib", result_name), source=sources)
Default(library)

# GDNativeLibrary
gdnlib = "material_x"
if target != "release":
    gdnlib += "_debug"
ext = ".gdextension"
# extfile = env.Substfile(os.path.join(result_path, gdnlib + ext), "misc/material_x" + ext, SUBST_DICT={
#     "{GDNATIVE_PATH}": gdnlib,
#     "{TARGET}": env["target"],
# })
# Default(extfile)