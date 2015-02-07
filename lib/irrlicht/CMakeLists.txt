# CMakeLists.txt for Irrlicht in STK
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/"
                    "${JPEG_INCLUDE_DIR}"
                    "${PNG_INCLUDE_DIRS}"
                    "${ZLIB_INCLUDE_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}/../zlib/") # For zconf.h on WIN32

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

if(APPLE)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/source/Irrlicht/MacOSX" "${CMAKE_CURRENT_SOURCE_DIR}/source/Irrlicht")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386 -F/Library/Frameworks")
endif()

add_definitions(-DNDEBUG=1 -DIRRLICHT_EXPORTS=1 -DPNG_THREAD_UNSAFE_OK -DPNG_NO_MMX_CODE -DPNG_NO_MNG_FEATURES)
if(MSVC)
  add_definitions(/D_IRR_STATIC_LIB_)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS) # Shut up about unsafe stuff
elseif(MINGW)
  add_definitions(-D_IRR_STATIC_LIB_)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Shut up about unsafe stuff
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -pipe -O3  -fno-exceptions  -fstrict-aliasing -fexpensive-optimizations -I/usr/X11R6/include -std=gnu++0x")
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -pipe -O3  -fno-exceptions  -fstrict-aliasing -fexpensive-optimizations -I/usr/X11R6/include")
endif()

# Xrandr
if(UNIX AND USE_XRANDR)
    add_definitions(-DNO_IRR_LINUX_X11_VIDMODE_)
    add_definitions(-D_IRR_LINUX_X11_RANDR_)
endif()

file(GLOB_RECURSE IRR_DEVICES_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/Device/*.h")
file(GLOB_RECURSE IRR_DEVICES_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/Device/*.cpp")
file(GLOB_RECURSE IRR_LOADERS_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/Loaders/*.h")
file(GLOB_RECURSE IRR_LOADERS_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/Loaders/*.cpp")
file(GLOB_RECURSE IRR_WRITERS_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/Writers/*.h")
file(GLOB_RECURSE IRR_WRITERS_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/Writers/*.cpp")
file(GLOB_RECURSE IRR_GUI_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/GUI/*.h")
file(GLOB_RECURSE IRR_GUI_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/GUI/*.cpp")
file(GLOB IRR_SCENENODES_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/SceneNodes/*.h")
file(GLOB IRR_SCENENODES_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/SceneNodes/*.cpp")
file(GLOB IRR_SCENENODES_PARTICLES_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/SceneNodes/Particles/*.h")
file(GLOB IRR_SCENENODES_PARTICLES_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/Irrlicht/SceneNodes/Particles/*.cpp")

file(GLOB IRR_MATHS_INCLUDE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "Include/Maths/*.h")
file(GLOB IRR_GUI_INCLUDE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "Include/GUI/*.h")
file(GLOB IRR_SCENENODES_INCLUDE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "Include/SceneNodes/*.h")

include(../../cmake/SourceGroupFunctions.cmake)
source_group_hierarchy(IRR_DEVICES_SOURCES IRR_DEVICES_HEADERS)
source_group_hierarchy(IRR_LOADERS_SOURCES IRR_LOADERS_HEADERS)
source_group_hierarchy(IRR_WRITERS_SOURCES IRR_WRITERS_HEADERS)
source_group_hierarchy(IRR_GUI_SOURCES IRR_GUI_HEADERS)
source_group_hierarchy(IRR_SCENENODES_SOURCES IRR_SCENENODES_HEADERS)
source_group_hierarchy(IRR_SCENENODES_PARTICLES_SOURCES IRR_SCENENODES_PARTICLES_HEADERS)

source_group_hierarchy(IRR_MATHS_INCLUDE IRR_MATHS_INCLUDE)
source_group_hierarchy(IRR_GUI_INCLUDE IRR_GUI_INCLUDE)
source_group_hierarchy(IRR_SCENENODES_INCLUDE IRR_SCENENODES_INCLUDE)

set(IRRLICHT_SOURCES
${IRR_DEVICES_SOURCES}
${IRR_LOADERS_SOURCES}
${IRR_WRITERS_SOURCES}
${IRR_GUI_SOURCES}
${IRR_SCENENODES_SOURCES}
${IRR_SCENENODES_PARTICLES_SOURCES}
source/Irrlicht/CZBuffer.cpp
source/Irrlicht/CLimitReadFile.cpp
source/Irrlicht/CVideoModeList.cpp
source/Irrlicht/CDefaultGUIElementFactory.cpp
source/Irrlicht/CTerrainTriangleSelector.cpp
source/Irrlicht/CFileSystem.cpp
source/Irrlicht/os.cpp
source/Irrlicht/CFPSCounter.cpp
source/Irrlicht/CZipReader.cpp
source/Irrlicht/CDefaultSceneNodeAnimatorFactory.cpp
source/Irrlicht/COpenGLSLMaterialRenderer.cpp
source/Irrlicht/CTriangleBBSelector.cpp
source/Irrlicht/COpenGLTexture.cpp
source/Irrlicht/CAttributes.cpp
source/Irrlicht/CTriangleSelector.cpp
source/Irrlicht/CColorConverter.cpp
source/Irrlicht/CMeshCache.cpp
source/Irrlicht/CMemoryFile.cpp
source/Irrlicht/CMountPointReader.cpp
source/Irrlicht/CSceneCollisionManager.cpp
source/Irrlicht/CImage.cpp
source/Irrlicht/CTarReader.cpp
source/Irrlicht/COpenGLParallaxMapRenderer.cpp
source/Irrlicht/CLogger.cpp
source/Irrlicht/CMeshManipulator.cpp
source/Irrlicht/CWriteFile.cpp
source/Irrlicht/COctreeTriangleSelector.cpp
source/Irrlicht/CFileList.cpp
source/Irrlicht/COSOperator.cpp
source/Irrlicht/COpenGLExtensionHandler.cpp
source/Irrlicht/CXMLWriter.cpp
source/Irrlicht/CSceneManager.cpp
source/Irrlicht/Irrlicht.cpp
source/Irrlicht/COpenGLShaderMaterialRenderer.cpp
source/Irrlicht/COpenGLDriver.cpp
source/Irrlicht/irrXML.cpp
source/Irrlicht/CSkinnedMesh.cpp
source/Irrlicht/CXMLReader.cpp
source/Irrlicht/CDummyTransformationSceneNode.cpp
source/Irrlicht/CGeometryCreator.cpp
source/Irrlicht/CNullDriver.cpp
source/Irrlicht/CReadFile.cpp
source/Irrlicht/CMetaTriangleSelector.cpp
source/Irrlicht/CDefaultSceneNodeFactory.cpp
source/Irrlicht/glext.h
source/Irrlicht/CMeshCache.h
source/Irrlicht/CAttributes.h
source/Irrlicht/CMountPointReader.h
source/Irrlicht/CSkinnedMesh.h
source/Irrlicht/COpenGLParallaxMapRenderer.h
source/Irrlicht/COpenGLShaderMaterialRenderer.h
source/Irrlicht/COctreeTriangleSelector.h
source/Irrlicht/COpenGLTexture.h
source/Irrlicht/Octree.h
source/Irrlicht/os.h
source/Irrlicht/CDefaultGUIElementFactory.h
source/Irrlicht/CTriangleBBSelector.h
source/Irrlicht/S2DVertex.h
source/Irrlicht/CDefaultSceneNodeAnimatorFactory.h
source/Irrlicht/resource.h
source/Irrlicht/SoftwareDriver2_helper.h
source/Irrlicht/CMeshManipulator.h
source/Irrlicht/IImagePresenter.h
source/Irrlicht/IZBuffer.h
source/Irrlicht/wglext.h
source/Irrlicht/CTimer.h
source/Irrlicht/COpenGLSLMaterialRenderer.h
source/Irrlicht/MacOSX/AppDelegate.h
source/Irrlicht/MacOSX/CIrrDeviceMacOSX.h
source/Irrlicht/MacOSX/OSXClipboard.h
source/Irrlicht/CSceneManager.h
source/Irrlicht/COpenGLDriver.h
source/Irrlicht/CSceneCollisionManager.h
source/Irrlicht/CDummyTransformationSceneNode.h
source/Irrlicht/CReadFile.h
source/Irrlicht/COSOperator.h
source/Irrlicht/CZBuffer.h
source/Irrlicht/CImage.h
source/Irrlicht/COpenGLExtensionHandler.h
source/Irrlicht/CTriangleSelector.h
source/Irrlicht/CDefaultSceneNodeFactory.h
source/Irrlicht/CXMLReaderImpl.h
source/Irrlicht/COpenGLMaterialRenderer.h
source/Irrlicht/CVideoModeList.h
source/Irrlicht/CColorConverter.h
source/Irrlicht/SoftwareDriver2_compile_config.h
source/Irrlicht/CXMLReader.h
source/Irrlicht/CFileSystem.h
source/Irrlicht/S4DVertex.h
source/Irrlicht/CBlit.h
source/Irrlicht/CZipReader.h
source/Irrlicht/IAttribute.h
source/Irrlicht/CNullDriver.h
source/Irrlicht/CWriteFile.h
source/Irrlicht/glxext.h
source/Irrlicht/CMetaTriangleSelector.h
source/Irrlicht/CTarReader.h
source/Irrlicht/CXMLWriter.h
source/Irrlicht/ITriangleRenderer.h
source/Irrlicht/CTerrainTriangleSelector.h
source/Irrlicht/CAttributeImpl.h
source/Irrlicht/CGeometryCreator.h
source/Irrlicht/BuiltInFont.h
source/Irrlicht/CMemoryFile.h
source/Irrlicht/CFPSCounter.h
source/Irrlicht/CFileList.h
source/Irrlicht/CLimitReadFile.h

${IRR_DEVICES_HEADERS}
${IRR_LOADERS_HEADERS}
${IRR_WRITERS_HEADERS}
${IRR_GUI_HEADERS}
${IRR_SCENENODES_HEADERS}
${IRR_SCENENODES_PARTICLES_HEADERS}
${IRR_MATHS_INCLUDE}
${IRR_GUI_INCLUDE}
${IRR_SCENENODES_INCLUDE}
include/ESceneNodeAnimatorTypes.h
include/IParticleGravityAffector.h
include/IEventReceiver.h
include/EMessageBoxFlags.h
include/IParticleSphereEmitter.h
include/IParticleFadeOutAffector.h
include/IGeometryCreator.h
include/irrunpack.h
include/IParticleRingEmitter.h
include/IMaterialRenderer.h
include/SVertexIndex.h
include/SMaterialLayer.h
include/irrMap.h
include/EMaterialTypes.h
include/IAnimatedMeshSceneNode.h
include/IParticleCylinderEmitter.h
include/IAttributeExchangingObject.h
include/IVertexBuffer.h
include/ISkinnedMesh.h
include/irrList.h
include/SAnimatedMesh.h
include/IParticleSystemSceneNode.h
include/ITerrainSceneNode.h
include/SMeshBuffer.h
include/ILogger.h
include/ILightManager.h
include/IMeshBuffer.h
include/irrlicht.h
include/ITimer.h
include/IImage.h
include/path.h
include/IMetaTriangleSelector.h
include/ISceneUserDataSerializer.h
include/irrArray.h
include/irrString.h
include/IMesh.h
include/IMeshLoader.h
include/CMeshBuffer.h
include/ETerrainElements.h
include/EDebugSceneTypes.h
include/driverChoice.h
include/IParticleAttractionAffector.h
include/irrAllocator.h
include/IDummyTransformationSceneNode.h
include/SSkinMeshBuffer.h
include/ISceneLoader.h
include/IrrlichtDevice.h
include/IMaterialRendererServices.h
include/SColor.h
include/IXMLWriter.h
include/CDynamicMeshBuffer.h
include/IFileArchive.h
include/IVideoDriver.h
include/irrMath.h
include/ISceneNode.h
include/irrpack.h
include/ISceneManager.h
include/IImageWriter.h
include/EGUIElementTypes.h
include/IParticleMeshEmitter.h
include/CIndexBuffer.h
include/IAnimatedMesh.h
include/SMaterial.h
include/EDriverFeatures.h
include/IVideoModeList.h
include/fast_atof.h
include/ISceneNodeAnimatorFactory.h
include/SVertexManipulator.h
include/ITextSceneNode.h
include/IDynamicMeshBuffer.h
include/SExposedVideoData.h
include/IReferenceCounted.h
include/irrXML.h
include/SKeyMap.h
include/ECullingTypes.h
include/ITriangleSelector.h
include/coreutil.h
include/SMeshBufferTangents.h
include/ILightSceneNode.h
include/ISceneCollisionManager.h
include/heapsort.h
include/Keycodes.h
include/SParticle.h
include/EDriverTypes.h
include/IFileSystem.h
include/SMesh.h
include/IParticleAffector.h
include/EShaderTypes.h
include/SceneParameters.h
include/IImageLoader.h
include/IParticleBoxEmitter.h
include/ICursorControl.h
include/ICameraSceneNode.h
include/IMeshManipulator.h
include/CVertexBuffer.h
include/SSharedMeshBuffer.h
include/IMeshWriter.h
include/IMeshSceneNode.h
include/IParticleAnimatedMeshSceneNodeEmitter.h
include/IMeshCache.h
include/EMeshWriterEnums.h
include/EPrimitiveTypes.h
include/SViewFrustum.h
include/IXMLReader.h
include/EHardwareBufferFlags.h
include/IOSOperator.h
include/IGPUProgrammingServices.h
include/IrrCompileConfig.h
include/ISceneNodeFactory.h
include/EMaterialFlags.h
include/EDeviceTypes.h
include/EAttributes.h
include/S3DVertex.h
include/IWriteFile.h
include/irrTypes.h
include/IParticleEmitter.h
include/SLight.h
include/IReadFile.h
include/ESceneNodeTypes.h
include/IIndexBuffer.h
include/EGUIAlignment.h
include/SIrrCreationParameters.h
include/IFileList.h
include/SMeshBufferLightMap.h
include/IRandomizer.h
include/ISceneNodeAnimator.h
include/IAttributes.h
include/IParticleRotationAffector.h
include/IShaderConstantSetCallBack.h
include/ITexture.h
)

if(APPLE)
    set(IRRLICHT_SOURCES
        ${IRRLICHT_SOURCES}
        source/Irrlicht/MacOSX/AppDelegate.mm
        source/Irrlicht/MacOSX/CIrrDeviceMacOSX.mm
        source/Irrlicht/MacOSX/OSXClipboard.mm)

    #list(APPEND CMAKE_C_SOURCE_FILE_EXTENSIONS mm)
    #set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS cpp)

    set_source_files_properties(source/Irrlicht/MacOSX/AppDelegate.mm PROPERTIES COMPILE_FLAGS "-x objective-c++ -O3  -fno-rtti")
    set_source_files_properties(source/Irrlicht/MacOSX/AppDelegate.mm PROPERTIES LANGUAGE C)

    set_source_files_properties(source/Irrlicht/MacOSX/CIrrDeviceMacOSX.mm PROPERTIES COMPILE_FLAGS "-x objective-c++ -O3  -fno-rtti")
    set_source_files_properties(source/Irrlicht/MacOSX/CIrrDeviceMacOSX.mm PROPERTIES LANGUAGE C)

    set_source_files_properties(source/Irrlicht/MacOSX/OSXClipboard.mm PROPERTIES COMPILE_FLAGS "-x objective-c++ -O3 -fno-rtti")
    set_source_files_properties(source/Irrlicht/MacOSX/OSXClipboard.mm PROPERTIES LANGUAGE C)
endif()

add_library(stkirrlicht ${IRRLICHT_SOURCES})
target_link_libraries(stkirrlicht ${PNG_LIBRARY} ${JPEG_LIBRARY} ${ZLIB_LIBRARY})



