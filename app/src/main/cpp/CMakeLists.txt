cmake_minimum_required(VERSION 3.22.1)

project("openflight")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(main SHARED
    main.cpp
    EGLManager.cpp
    Renderer.cpp
    Shader.cpp
    Mesh.cpp
    Camera.cpp
    MeshLoader.cpp
    ImageLoader.cpp
    UIButton.cpp
)

target_link_libraries(main
    android
    log
    EGL
    GLESv2
    -Wl,--whole-archive
    native_app_glue
    -Wl,--no-whole-archive
)