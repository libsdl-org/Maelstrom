add_library(SDLmac STATIC
    Mac_Compat.h
    Mac_FontServ.cpp
    Mac_FontServ.h
    Mac_Resource.cpp
    Mac_Resource.h
    Mac_Sound.cpp
    Mac_Sound.h
    Mac_Wave.cpp
    Mac_Wave.h
    applefile.h
    macbinary.h
    bitesex.h
)
target_include_directories(SDLmac PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(SDLmac PRIVATE SDL3::SDL3)
