cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE HEADERS
	 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	 "include/*.hpp"
	 )

file(GLOB_RECURSE SOURCES
	 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	 "src/*.cpp"
	 )

project(evk CXX)

add_library(${PROJECT_NAME}
			"${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}"
			"${CMAKE_CURRENT_SOURCE_DIR}/${HEADERS}"
			)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC
						   "${CMAKE_CURRENT_SOURCE_DIR}/include/"
						   "/usr/local/include/"
                           "/usr/local/include/mat4x4/"
                           "/usr/local/include/ESDL/"
                           "/usr/local/include/vulkan/"
                           "/usr/local/include/vma/"
						   "/opt/local/include/"
						   "/opt/local/include/SDL2/"
						   )

target_link_directories(${PROJECT_NAME} PUBLIC
						"/usr/local/lib/"
						"/opt/local/lib/"
						)

target_link_libraries(${PROJECT_NAME}
                      ESDL
					  SDL2_image
					  vulkan
                      mat4x4
					  )

include(CMakePrintHelpers)
cmake_print_variables(PROJECT_NAME)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "lib"
		PUBLIC_HEADER DESTINATION "include/evk"
)

