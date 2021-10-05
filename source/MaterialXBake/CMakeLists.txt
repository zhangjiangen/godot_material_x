file(GLOB materialx_source "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB materialx_headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")

add_executable(MaterialXBake ${materialx_source} ${materialx_headers})

set(LIBS
    MaterialXBake
    MaterialXFormat
    MaterialXGenGlsl
    MaterialXRenderGlsl)

if (MATERIALX_BUILD_GEN_OSL)
    LIST(APPEND LIBS MaterialXGenOsl)
endif()
if (MATERIALX_BUILD_GEN_MDL)
    LIST(APPEND LIBS MaterialXGenMdl)
endif()

target_link_libraries(
    ${LIBS})

target_include_directories(
    MaterialXBake
    PRIVATE
    ${NANOGUI_INCLUDE_DIRS}
    ${NANOGUI_EXTRA_INCS})

if(MATERIALX_BUILD_OIIO AND OPENIMAGEIO_ROOT_DIR)
    add_custom_command(TARGET MaterialXView POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${OPENIMAGEIO_ROOT_DIR}/bin ${CMAKE_CURRENT_BINARY_DIR})
endif()

install(TARGETS MaterialXBake
    EXPORT MaterialX
    RUNTIME DESTINATION bin)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/MaterialXBake.pdb"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/" OPTIONAL)
