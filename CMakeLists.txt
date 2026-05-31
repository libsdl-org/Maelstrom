cmake_minimum_required(VERSION 3.7...4.0)

set(MAJOR_VERSION 4)
set(MINOR_VERSION 0)
set(MICRO_VERSION 1)
set(SDL_REQUIRED_VERSION 3.4.2)
set(SDL_NET_REQUIRED_VERSION 3.2.0)

project(Maelstrom
    LANGUAGES C CXX
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)
if(APPLE)
    enable_language(OBJC)
endif()

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# on Web targets, we need CMake to generate a HTML webpage.
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX_CXX ".html" CACHE INTERNAL "")

    # Enable persistent storage
    set(SDL_EMSCRIPTEN_PERSISTENT_PATH /storage)
endif()

option(USE_VENDORED_PHYSFS "Use the vendored version of PhysFS" TRUE)
if (USE_VENDORED_PHYSFS)
    set(PHYSFS_BUILD_SHARED OFF)
    set(PHYSFS_BUILD_TEST OFF)
    add_subdirectory(external/physfs EXCLUDE_FROM_ALL)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PhysFS REQUIRED IMPORTED_TARGET physfs)
endif()

option(USE_VENDORED_SDL "Use the vendored version of SDL" TRUE)
if (USE_VENDORED_SDL)
    add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS Headers SDL3-shared)
    include(CMakeDependentOption)
    macro(dep_option _NAME _DESC _DEFLT _DEPTEST _FAILDFLT)
      cmake_dependent_option("${_NAME}" "${_DESC}" "${_DEFLT}" "${_DEPTEST}" "${_FAILDFLT}")
    endmacro()
    include("cmake/sdlcpu.cmake")
    include("cmake/sdlplatform.cmake")
endif()

option(USE_VENDORED_SDL_NET "Use the vendored version of SDL_net" TRUE)
if (USE_VENDORED_SDL_NET)
    set(original_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(external/SDL_net SDL_net EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS "${original_BUILD_SHARED_LIBS}")
else()
    find_package(SDL3_net ${SDL_NET_REQUIRED_VERSION} REQUIRED COMPONENTS Headers SDL3_net-shared)
endif()

add_subdirectory(maclib)

SDL_DetectTargetCPUArchitectures(SDL_CPU_NAMES)
SDL_DetectCMakePlatform()

set(MAELSTROM_SOURCES
	game/about.cpp
	game/about.h
	game/colortable.h
	game/continue.cpp
	game/continue.h
	game/controls.cpp
	game/controls.h
	game/fastrand.cpp
	game/fastrand.h
	game/game.cpp
	game/game.h
	game/gameinfo.cpp
	game/gameinfo.h
	game/gameover.cpp
	game/gameover.h
	game/init.cpp
	game/init.h
	game/load.cpp
	game/load.h
	game/lobby.cpp
	game/lobby.h
	game/Localization.h
	game/MacDialog.cpp
	game/MacDialog.h
	game/Maelstrom_Globals.h
	game/Maelstrom.h
	game/MaelstromUI.cpp
	game/MaelstromUI.h
	game/main.cpp
	game/main.h
	game/make.cpp
	game/make.h
	game/mods.cpp
	game/mods.h
	game/myerror.cpp
	game/myerror.h
	game/netplay.cpp
	game/netplay.h
	game/object.cpp
	game/object.h
	game/objects.cpp
	game/objects.h
	game/packet.h
	game/player.cpp
	game/player.h
	game/protocol.h
	game/rect.cpp
	game/rect.h
	game/replay.cpp
	game/replay.h
	game/scores.cpp
	game/scores.h
	game/shenobi.h
	game/steam.cpp
	game/steam.h

	screenlib/SDL_FrameBuf.cpp
	screenlib/SDL_FrameBuf.h
	screenlib/UIArea.cpp
	screenlib/UIArea.h
	screenlib/UIBaseElement.cpp
	screenlib/UIBaseElement.h
	screenlib/UIContainer.cpp
	screenlib/UIContainer.h
	screenlib/UIDialogButton.cpp
	screenlib/UIDialogButton.h
	screenlib/UIDialog.cpp
	screenlib/UIDialog.h
	screenlib/UIDrawEngine.cpp
	screenlib/UIDrawEngine.h
	screenlib/UIElementButton.cpp
	screenlib/UIElementButton.h
	screenlib/UIElementCheckbox.cpp
	screenlib/UIElementCheckbox.h
	screenlib/UIElement.cpp
	screenlib/UIElementDropdown.cpp
	screenlib/UIElementDropdown.h
	screenlib/UIElementEditbox.cpp
	screenlib/UIElementEditbox.h
	screenlib/UIElement.h
	screenlib/UIElementRadio.cpp
	screenlib/UIElementRadio.h
	screenlib/UIElementThumbstick.cpp
	screenlib/UIElementThumbstick.h
	screenlib/UIFontInterface.h
	screenlib/UIImageInterface.h
	screenlib/UIManager.cpp
	screenlib/UIManager.h
	screenlib/UIPanel.cpp
	screenlib/UIPanel.h
	screenlib/UISoundInterface.h
	screenlib/UITemplates.cpp
	screenlib/UITemplates.h
	screenlib/UITexture.cpp
	screenlib/UITexture.h

	utils/array.h
	utils/ErrorBase.h
	utils/files.c
	utils/files.h
	utils/hashtable.c
	utils/hashtable.h
	utils/loadxml.cpp
	utils/loadxml.h
	utils/prefs.cpp
	utils/prefs.h
	utils/rapidxml.h
	utils/rapidxml.hpp

	external/physfs/extras/physfssdl3.c

	miniz/miniz.c

	Maelstrom.rc
)

if(ANDROID)
    add_library(Maelstrom MODULE ${MAELSTROM_SOURCES})
    set_property(TARGET Maelstrom PROPERTY OUTPUT_NAME main)
else()
    add_executable(Maelstrom WIN32 ${MAELSTROM_SOURCES})
endif()

if(MSVC)
    target_compile_options(Maelstrom PRIVATE -W3)
else()
    target_compile_options(Maelstrom PRIVATE -Wall)
    target_compile_options(Maelstrom PRIVATE -Wextra)
    target_compile_options(Maelstrom PRIVATE -Wno-cast-function-type)
    target_compile_options(Maelstrom PRIVATE -Wno-sign-compare)
    target_compile_options(Maelstrom PRIVATE -Wno-unused-function)
    target_compile_options(Maelstrom PRIVATE -Wno-unused-parameter)
endif()

dep_option(STEAM "Build with Steam support" ON "WIN32 OR LINUX OR MACOS" OFF)

if(STEAM)
    target_compile_definitions(Maelstrom PRIVATE ENABLE_STEAM)
    add_library(SteamworksSDK::steam_api SHARED IMPORTED)
    set_property(TARGET SteamworksSDK::steam_api PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/external/SteamworksSDK/public")
    if(WIN32)
        set_property(TARGET SteamworksSDK::steam_api PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/win64/steam_api64.dll")
        set_property(TARGET SteamworksSDK::steam_api PROPERTY IMPORTED_IMPLIB   "${PROJECT_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/win64/steam_api64.lib")
    elseif(LINUX)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set_property(TARGET SteamworksSDK::steam_api PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/linux32/libsteam_api.so")
        else()
            set_property(TARGET SteamworksSDK::steam_api PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/linux64/libsteam_api.so")
        endif()
    elseif(MACOS)
        set_property(TARGET SteamworksSDK::steam_api PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/osx/libsteam_api.dylib")
    endif()
    target_link_libraries(Maelstrom PRIVATE SteamworksSDK::steam_api)
endif()

target_link_libraries(Maelstrom PRIVATE SDLmac)
if (USE_VENDORED_PHYSFS)
    target_link_libraries(Maelstrom PRIVATE physfs-static)
else()
    target_link_libraries(Maelstrom PRIVATE PkgConfig::PhysFS)
endif()
target_link_libraries(Maelstrom PRIVATE SDL3_net::SDL3_net)
target_link_libraries(Maelstrom PRIVATE SDL3::SDL3)

if(EMSCRIPTEN)
    # Increase the default stack size
    target_link_options(Maelstrom PRIVATE -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576)

    # Create a source map for debugging
    target_link_options(Maelstrom PRIVATE -gsource-map)

    # on the web, we have to put the files inside of the webassembly
    # somewhat unintuitively, this is done via a linker argument.
    target_link_libraries(Maelstrom PRIVATE
        "--preload-file \"${CMAKE_CURRENT_LIST_DIR}/Data@Data/\""
        "--preload-file \"${CMAKE_CURRENT_LIST_DIR}/mods@mods/\""
    )
else()
    option(STANDALONE_INSTALL "Build Maelstrom installed into a single directory" TRUE)

    if(STANDALONE_INSTALL)
        set(CMAKE_INSTALL_BINDIR ".")
        set(CMAKE_INSTALL_LIBDIR ".")
        set(CMAKE_INSTALL_DOCDIR "Docs")
        set(GAME_INSTALL_DATADIR ".")
        if(APPLE)
            set_property(TARGET Maelstrom SDL3-shared PROPERTY INSTALL_RPATH "@executable_path")
        else()
            set_property(TARGET Maelstrom PROPERTY INSTALL_RPATH "$ORIGIN")
            if(USE_VENDORED_SDL)
                set_property(TARGET SDL3-shared PROPERTY INSTALL_RPATH "$ORIGIN")
            endif()
        endif()
    else()
        include(GNUInstallDirs)
        set(GAME_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}")

        target_compile_definitions(Maelstrom PRIVATE
            MAELSTROM_USE_XDG_DIRS
            MAELSTROM_DATA=\"${CMAKE_INSTALL_PREFIX}/${GAME_INSTALL_DATADIR}/Data/\"
            MAELSTROM_MODS=\"${CMAKE_INSTALL_PREFIX}/${GAME_INSTALL_DATADIR}/mods/\"
        )
    endif()

    install(TARGETS Maelstrom LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    if (USE_VENDORED_SDL)
        install(TARGETS SDL3-shared LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" NAMELINK_SKIP RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
    if(STEAM)
        install(IMPORTED_RUNTIME_ARTIFACTS SteamworksSDK::steam_api)
    endif()
    install(DIRECTORY Data DESTINATION "${GAME_INSTALL_DATADIR}")
    install(DIRECTORY mods DESTINATION "${GAME_INSTALL_DATADIR}")

    file(GLOB docs "Docs/*.txt" "README*")
    install(FILES ${docs} "COPYING" TYPE DOC)

    #
    # Uninstall
    #
    configure_file(cmake/cmake_uninstall.cmake.in cmake_uninstall.cmake IMMEDIATE @ONLY)

    add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

    #
    # Source package
    #
    find_package(Git QUIET)

    if(GIT_FOUND)
        add_custom_target(package_sources
          COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/dist
          COMMAND ${GIT_EXECUTABLE} archive HEAD -o "${PROJECT_BINARY_DIR}/dist/${PROJECT_NAME}-${PROJECT_VERSION}.zip" --prefix "${PROJECT_NAME}-${PROJECT_VERSION}/"
          COMMAND ${GIT_EXECUTABLE} archive HEAD -o "${PROJECT_BINARY_DIR}/dist/${PROJECT_NAME}-${PROJECT_VERSION}.tar.gz" --prefix "${PROJECT_NAME}-${PROJECT_VERSION}/"
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          COMMENT "Creating source archive..."
          VERBATIM
        )
    else()
        message(WARNING "Git not found, 'package_sources' target will not be available.")
    endif()

    #
    # Binary package
    #

    if(MSVC)
        set(CPACK_GENERATOR "ZIP")
    else()
        set(CPACK_GENERATOR "TGZ")
    endif()
    configure_file(cmake/CPackProjectConfig.cmake.in CPackProjectConfig.cmake @ONLY)
    set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/CPackProjectConfig.cmake")
    # CPACK_SOURCE_PACKAGE_FILE_NAME must end with "-src" (so we can block creating a source archive)
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
    include(CPack)
endif()
