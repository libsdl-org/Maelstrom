LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
SDL_IMAGE_PATH := ../SDL_image
SDL_NET_PATH := ../SDL_net
SDL_TTF_PATH := ../SDL_ttf

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/$(SDL_IMAGE_PATH) $(LOCAL_PATH)/$(SDL_NET_PATH) $(LOCAL_PATH)/$(SDL_TTF_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	../../../mainstub.cpp			\
	../../../game/MacDialog.cpp     \
	../../../game/MaelstromUI.cpp   \
	../../../game/ServiceManager.cpp    \
	../../../game/about.cpp         \
	../../../game/continue.cpp      \
	../../../game/controls.cpp      \
	../../../game/fastrand.cpp      \
	../../../game/game.cpp          \
	../../../game/gameinfo.cpp      \
	../../../game/gameover.cpp      \
	../../../game/init.cpp          \
	../../../game/load.cpp          \
	../../../game/lobby.cpp         \
	../../../game/main.cpp          \
	../../../game/make.cpp          \
	../../../game/myerror.cpp       \
	../../../game/netplay.cpp       \
	../../../game/object.cpp        \
	../../../game/objects.cpp       \
	../../../game/player.cpp        \
	../../../game/rect.cpp          \
	../../../game/replay.cpp        \
	../../../game/scores.cpp        \
	../../../game/store.cpp         \
	../../../maclib/Mac_FontServ.cpp	\
	../../../maclib/Mac_Sound.cpp		\
	../../../physfs/archiver_dir.c		\
	../../../physfs/archiver_zip.c		\
	../../../physfs/physfs.c		\
	../../../physfs/physfs_byteorder.c	\
	../../../physfs/physfs_unicode.c	\
	../../../physfs/platform_beos.cpp	\
	../../../physfs/platform_macosx.c	\
	../../../physfs/platform_posix.c	\
	../../../physfs/platform_unix.c		\
	../../../physfs/platform_windows.c	\
	../../../screenlib/SDL_FrameBuf.cpp	\
	../../../screenlib/UIArea.cpp		\
	../../../screenlib/UIBaseElement.cpp	\
	../../../screenlib/UIContainer.cpp	\
	../../../screenlib/UIDialog.cpp		\
	../../../screenlib/UIDialogButton.cpp	\
	../../../screenlib/UIDrawEngine.cpp	\
	../../../screenlib/UIElement.cpp	\
	../../../screenlib/UIElementButton.cpp	\
	../../../screenlib/UIElementCheckbox.cpp	\
	../../../screenlib/UIElementEditbox.cpp	\
	../../../screenlib/UIElementRadio.cpp	\
	../../../screenlib/UIElementThumbstick.cpp	\
	../../../screenlib/UIManager.cpp	\
	../../../screenlib/UIPanel.cpp		\
	../../../screenlib/UITemplates.cpp	\
	../../../screenlib/UITexture.cpp	\
	../../../utils/files.c			\
	../../../utils/hashtable.c		\
	../../../utils/loadxml.cpp		\
	../../../utils/physfsrwops.c		\
	../../../utils/prefs.cpp	

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_net SDL2_ttf

LOCAL_LDLIBS := -lGLESv1_CM -llog -lz

include $(BUILD_SHARED_LIBRARY)
