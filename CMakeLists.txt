cmake_minimum_required(VERSION 3.25)

set(CMAKE_GENERATOR "Ninja")
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(rydor)

find_package(Vulkan REQUIRED)

# FetchContent_Declare(
#   cglm
#   GIT_REPOSITORY https://github.com/recp/cglm.git
#   GIT_TAG v0.9.6
#   GIT_SHALLOW ON
#   EXCLUDE_FROM_ALL
# )

# set(CGLM_SHARED OFF CACHE INTERNAL "")
# set(CGLM_STATIC OFF CACHE INTERNAL "")
# set(CGLM_USE_C99 OFF CACHE INTERNAL "")
# set(CGLM_USE_TEST OFF CACHE INTERNAL "")

add_executable(
	${PROJECT_NAME}
	src/main.cpp
	src/utils.cpp
)

include(cmake/CPM.cmake)

CPMAddPackage(
	URI "gh:libsdl-org/SDL#release-3.2.14"
	OPTIONS "SDL_AUDIO_DEFAULT OFF" 
	"SDL_GPU_DEFAULT OFF"
	"SDL_RENDER_DEFAULT OFF"
	"SDL_CAMERA_DEFAULT OFF"
	"SDL_JOYSTICK_DEFAULT OFF"
	"SDL_HAPTIC_DEFAULT OFF"
	"SDL_POWER_DEFAULT OFF"
	"SDL_SENSOR_DEFAULT OFF"
	"SDL_DIALOG_DEFAULT OFF"
	"SDL_TEST_LIBRARY OFF"
	"SDL_OPENGLES OFF"
	"SDL_OPENGL OFF"
	"SDL_DIRECTX OFF"
)

if(WIN32)
	#target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
	# -Wl,-subsystem,windows
	set_target_properties(${PROJECT_NAME}
		PROPERTIES
        LINK_FLAGS_DEBUG "-mconsole"
        LINK_FLAGS_RELEASE "-mwindows"
        LINK_FLAGS_RELWITHDEBINFO "-mwindows"
        LINK_FLAGS_MINSIZEREL "-mwindows"
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
	SDL3::SDL3
	Vulkan::Vulkan
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)

if (WIN32)
set(SPIRV_COMPILER "$ENV{VULKAN_SDK}/Bin/glslc.exe")
elseif(UNIX)
set(SPIRV_COMPILER "$ENV{VULKAN_SDK}/bin/glslc")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
	"${CMAKE_SOURCE_DIR}/shaders/*.vert"
	"${CMAKE_SOURCE_DIR}/shaders/*.frag"
)

foreach(GLSL_FILE ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
	set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
		COMMAND ${SPIRV_COMPILER} ${GLSL_FILE} -o ${SPIRV}
		DEPENDS ${GLSL_FILE}
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL_FILE)

add_custom_target(
	process-shaders
	DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} process-shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_BINARY_DIR}/shaders"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)