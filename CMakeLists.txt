cmake_minimum_required(VERSION 3.21)

project(
	WornCarryWeight
	VERSION 1.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")

set(SOURCES
	src/main.cpp
	src/Hooks.cpp
	src/Settings.cpp
	src/Menu.cpp
	src/CarryWeightManager.cpp
)

set(HEADERS
	src/PCH.h
	src/Hooks.h
	src/Settings.h
	src/Menu.h
	src/CarryWeightManager.h
	src/SKSEMenuFramework.h
)

add_library(${PROJECT_NAME} SHARED
	${SOURCES}
	${HEADERS}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	_UNICODE
	UNICODE
	NOMINMAX
	_CRT_SECURE_NO_WARNINGS
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	CommonLibSSE::CommonLibSSE
	spdlog::spdlog
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
	src/PCH.h
)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
		/sdl
		/utf-8
		/Zi
		/permissive-
		/Zc:preprocessor
		/EHsc
		/W4
		/WX-
		/wd4100  # unreferenced formal parameter
		/wd4189  # local variable initialized but not referenced
	)

	target_link_options(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:DEBUG>:/INCREMENTAL>"
		"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

# Copy INI file for convenience
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/WornCarryWeight.ini
	${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins/WornCarryWeight.ini
	COPYONLY
)




