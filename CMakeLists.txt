cmake_minimum_required(VERSION 3.25)

set(CMAKE_GENERATOR "Ninja")
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(rydor)

file(GLOB_RECURSE sources 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} ${sources})

if(WIN32)
	target_link_options(${PROJECT_NAME} PRIVATE ${SUBSYSTEM_LINKER_OPTIONS} -Xlinker /subsystem:windows)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

set(RYDOR_LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(RYDOR_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEMP_DIR ${CMAKE_SOURCE_DIR}/temp)
set(SDL2_SRC_URL "https://github.com/libsdl-org/SDL/releases/download/release-2.30.5/SDL2-2.30.5.tar.gz")
set(SDL2_SRC_NAME "SDL2-2.30.5")

if(NOT EXISTS ${RYDOR_LIB_DIR} OR NOT EXISTS ${RYDOR_INCLUDE_DIR})
	message(STATUS "Creating necessary directories to contain dependencies...")
	file(MAKE_DIRECTORY ${RYDOR_LIB_DIR} ${RYDOR_LIB_DIR}/shared ${RYDOR_INCLUDE_DIR})
endif()

if(NOT EXISTS ${RYDOR_LIB_DIR}/SDL2 OR NOT EXISTS ${RYDOR_INCLUDE_DIR}/SDL2)
	file(MAKE_DIRECTORY ${TEMP_DIR})
	file(DOWNLOAD ${SDL2_SRC_URL} ${TEMP_DIR}/${SDL2_SRC_NAME}.tar.gz SHOW_PROGRESS)
	execute_process(COMMAND tar -xzvf ${TEMP_DIR}/${SDL2_SRC_NAME}.tar.gz -C ${TEMP_DIR} RESULT_VARIABLE unzip_res)
	if(NOT unzip_res EQUAL 0)
		message(FATAL_ERROR "Failed to unzip SDL2 source package!")
	endif()

	execute_process(COMMAND cmake -S ${TEMP_DIR}/${SDL2_SRC_NAME} -B ${TEMP_DIR}/${SDL2_SRC_NAME}/build 
		-G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ 
		-DSDL_VIDEO:BOOL=1 -DSDL_RENDER_D3D:BOOL=0 -DSDL_DIRECTX:BOOL=0 -DSDL_3DNOW:BOOL=0 
		-DSDL_DUMMYAUDIO:BOOL=0 -DSDL_OPENGL:BOOL=0 -DSDL_DUMMYVIDEO:BOOL=0 -DSDL_DISKAUDIO:BOOL=0 
		-DSDL_STATIC:BOOL=0 -DSDL_OPENGLES:BOOL=0 -DSDL_ALTIVEC:BOOL=0 -DSDL2_DISABLE_INSTALL:BOOL=1
		-DSDL_WASAPI:BOOL=0 -DCMAKE_BUILD_TYPE:STRING=Release -DSDL_AUDIO:BOOL=0 -DSDL_LIBUDEV:BOOL=0 
		-DSDL_ASSERTIONS:STRING=auto -DSDL_TEST:BOOL=0
	)

	execute_process(COMMAND ninja -C ${TEMP_DIR}/${SDL2_SRC_NAME}/build)

	file(COPY ${TEMP_DIR}/${SDL2_SRC_NAME}/build/include/SDL2 DESTINATION ${RYDOR_INCLUDE_DIR})
	file(COPY_FILE ${TEMP_DIR}/${SDL2_SRC_NAME}/build/include-config-release/SDL2/SDL_config.h ${RYDOR_INCLUDE_DIR}/SDL2/SDL_config.h)
	file(MAKE_DIRECTORY ${RYDOR_LIB_DIR}/SDL2)
	file(COPY_FILE ${TEMP_DIR}/${SDL2_SRC_NAME}/build/SDL2.lib ${RYDOR_LIB_DIR}/SDL2/SDL2.lib)
	file(COPY_FILE ${TEMP_DIR}/${SDL2_SRC_NAME}/build/SDL2main.lib ${RYDOR_LIB_DIR}/SDL2/SDL2main.lib)
	file(COPY_FILE ${TEMP_DIR}/${SDL2_SRC_NAME}/build/SDL2.dll ${RYDOR_LIB_DIR}/shared/SDL2.dll)
endif()	

if(EXISTS ${TEMP_DIR})
	file(REMOVE_RECURSE ${TEMP_DIR})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
	${RYDOR_LIB_DIR}/SDL2/SDL2main.lib
	${RYDOR_LIB_DIR}/SDL2/SDL2.lib 
	Vulkan::Vulkan
)

target_include_directories(${PROJECT_NAME} PRIVATE ${RYDOR_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)

add_custom_target(copy-shared-lib ALL 
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${RYDOR_LIB_DIR}/shared"
	"${CMAKE_BINARY_DIR}"
	COMMENT "Copying shared libs..."
)

#add_dependencies(${PROJECT_NAME} copy-shared-lib)