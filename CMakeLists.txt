cmake_minimum_required(VERSION 3.0...4.0)
project(Maelstrom C CXX)

include(GNUInstallDirs)

if(WIN32)
    set(GAME_INSTALLDIR ".")
else()
    set(GAME_INSTALLDIR "${CMAKE_INSTALL_PREFIX}/games/${PROJECT_NAME}")
endif()

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(maclib)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/SDL_net EXCLUDE_FROM_ALL)

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
	game/shinobi.h
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

	miniz/miniz.c

	Maelstrom.rc
)

add_executable(Maelstrom WIN32 ${MAELSTROM_SOURCES})

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

if(NOT WIN32)
  check_c_source_compiles("
      #include <ifaddrs.h>
      int main() {
        struct ifaddrs* ifap;
        getifaddrs(&ifap);
        return 0;
      }
    " HAVE_GETIFADDRS)
  if(HAVE_GETIFADDRS)
    target_compile_definitions(Maelstrom PRIVATE HAVE_GETIFADDRS)
  endif()
endif()

dep_option(STEAM "Build with Steam support" ON "WIN32 OR LINUX OR MACOS" OFF)

if(STEAM)
    target_compile_definitions(Maelstrom PRIVATE ENABLE_STEAM)
	target_include_directories(Maelstrom PRIVATE "${CMAKE_SOURCE_DIR}/external/SteamworksSDK/public")
	if(WIN32)
		target_link_directories(Maelstrom PRIVATE "${CMAKE_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/win64")
		target_link_libraries(Maelstrom PRIVATE steam_api64)
	elseif(LINUX)
		target_link_directories(Maelstrom PRIVATE "${CMAKE_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/linux64")
		target_link_libraries(Maelstrom PRIVATE steam_api)
	elseif(MACOS)
		target_link_directories(Maelstrom PRIVATE "${CMAKE_SOURCE_DIR}/external/SteamworksSDK/redistributable_bin/osx")
		target_link_libraries(Maelstrom PRIVATE steam_api)
	endif()
endif()

target_link_libraries(Maelstrom PRIVATE SDLmac)
target_link_libraries(Maelstrom PRIVATE SDL3::SDL3)
target_link_libraries(Maelstrom PRIVATE SDL3_net::SDL3_net)

install(TARGETS Maelstrom DESTINATION "${GAME_INSTALLDIR}")
install(DIRECTORY Data DESTINATION "${GAME_INSTALLDIR}")

file(GLOB docs "Docs/*.txt" "README*")
install(FILES ${docs} "COPYING" "COPYING.GPL" DESTINATION "${GAME_INSTALLDIR}/Docs")

if(NOT TARGET uninstall)
  configure_file(cmake/cmake_uninstall.cmake.in cmake_uninstall.cmake IMMEDIATE @ONLY)

  add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

