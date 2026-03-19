cmake_minimum_required(VERSION 3.20)
project(ScreenCover VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/application.cpp
    src/sequence_detector.cpp
    src/overlay_manager.cpp
    src/mode_overlay.cpp
    src/mode_poweroff.cpp
    src/tray_icon.cpp
)

# Header files
set(HEADERS
    src/application.h
    src/mode_base.h
    src/mode_overlay.h
    src/mode_poweroff.h
    src/sequence_detector.h
    src/overlay_manager.h
    src/tray_icon.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    user32
    gdi32
    shell32
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Compiler options for MSVC
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
