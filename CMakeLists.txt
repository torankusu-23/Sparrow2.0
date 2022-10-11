cmake_minimum_required(VERSION 3.14)

project(Sparrow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CONFIGURATION_TYPES "Release")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -GL -fp:fast")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -ffast-math")
endif()

set(HEADERS
        	public/Camera.h
        	public/constant.h
        	public/Controller.h
        	public/Image.h
        	public/Model.h
        	public/Pipeline.h
	public/Sample.h
	public/Scene.h
        	public/struct.h
	public/tools.h
        	public/Win.h
	public/Light.h
        	shader/shader.h
        )

set(SOURCES
        	private/Camera.cpp
        	private/Controller.cpp
        	private/Image.cpp
        	private/Model.cpp
        	private/Pipeline.cpp
        	private/Sample.cpp
        	private/Scene.cpp
        	private/Shading.cpp
        	private/struct.cpp
        	private/tools.cpp
        	private/Win.cpp
	private/Light.cpp
        	main.cpp
        )

add_executable(Sparrow  ${HEADERS} ${SOURCES})

if(MSVC)
    target_compile_options(Sparrow PRIVATE /fp:fast)
else()
    target_compile_options(Sparrow PRIVATE -ffast-math)
    target_link_libraries(Sparrow  m)
endif()

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT Sparrow)
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${HEADERS} ${SOURCES})