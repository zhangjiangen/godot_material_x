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
opts.Add(EnumVariable("godot_version", "The Godot target version", "4", ["3", "4"]))
opts.Update(env)

if env["godot_version"] == "3":
    env = SConscript("godot-cpp-3.x/SConstruct")

    # Patch base env
    replace_flags(env["CCFLAGS"], {
        "-mios-simulator-version-min=10.0": "-mios-simulator-version-min=11.0",
        "-miphoneos-version-min=10.0": "-miphoneos-version-min=11.0",
        "/std:c++14": "/std:c++17",
        "-std=c++14": "-std=c++17",
    })

    env = env.Clone()

    if env["target"] == "debug":
        env.Append(CPPDEFINES=["DEBUG_ENABLED"])

    if env["platform"] == "windows" and env["use_mingw"]:
        env.Append(LINKFLAGS=["-static-libgcc"])

    # Normalize suffix
    if env["platform"] in ["windows", "linux"]:
        env["arch"] = "x86_32" if env["bits"] == "32" else "x86_64"
        env["arch_suffix"] = env["arch"]
    elif env["platform"] == "osx":
        env["arch"] = env["macos_arch"]
        env["arch_suffix"] = env["arch"]
    elif env["platform"] == "ios":
        env["arch"] = "arm32" if env["ios_arch"] == "armv7" else env["ios_arch"]
        env["arch_suffix"] = env["ios_arch"] + (".simulator" if env["ios_simulator"] else "")
    elif env["platform"] == "android":
        env["arch"] = {
            "armv7": "arm32",
            "arm64v8": "arm64",
            "x86": "x86_32",
            "x86_64": "x86_64",
        }[env["android_arch"]]
        env["arch_suffix"] = env["arch"]
else:
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
sources = []
sources.append(
    [
        "thirdparty/godot/material_x_3d.cpp",
        "source/MaterialXBake/*.cpp",
        "source/MaterialXCore/*.cpp",
        "source/MaterialXFormat/*.cpp",
        "source/MaterialXFormat/PugiXML/*.cpp",
        "source/MaterialXGenShader/*.cpp",
        "source/MaterialXGenShader/Nodes/*.cpp",
        "source/MaterialXGenGlsl/*.cpp",
        "source/MaterialXGenGlsl/Nodes/*.cpp",
        "source/MaterialXRender/*.cpp",
        "source/MaterialXRenderGlsl/External/GLew/*.cpp",
        "source/MaterialXRenderGlsl/*.cpp",
        "source/MaterialXRenderHw/WindowWrapper.cpp",
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
    
env.Append(CPPDEFINES=["GL_SILENCE_DEPRECATION"])

sources.append("src/init_gdextension.cpp")

# Make the shared library
result_name = "material_x.{}.{}.{}{}".format(env["platform"], env["target"], env["arch_suffix"], env["SHLIBSUFFIX"])

library = env.SharedLibrary(target=os.path.join(result_path, "lib", result_name), source=sources)
Default(library)

# GDNativeLibrary
gdnlib = "material_x"
if target != "release":
    gdnlib += "_debug"
ext = ".tres" if env["godot_version"] == "3" else ".gdextension"
extfile = env.Substfile(os.path.join(result_path, gdnlib + ext), "misc/material_x" + ext, SUBST_DICT={
    "{GDNATIVE_PATH}": gdnlib,
    "{TARGET}": env["target"],
})
Default(extfile)