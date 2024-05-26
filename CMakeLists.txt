cmake_minimum_required(VERSION 3.24)
set(CMAKE_INSTALL_PREFIX "/Users/eprager/local")

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
						   "/Users/eprager/local/include/"
                           "/usr/local/include/"
						   "/opt/local/include/"
						   )

target_link_directories(${PROJECT_NAME} PUBLIC
						"/Users/eprager/local/lib/"
						"/opt/local/lib/"
						)

target_link_libraries(${PROJECT_NAME}
                      ESDL
					  SDL2_image
					  vulkan
                      mattresses
					  )

include(CMakePrintHelpers)
cmake_print_variables(PROJECT_NAME)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "lib"
		PUBLIC_HEADER DESTINATION "include/evk"
)




add_executable(test
            "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${HEADERS}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/test/vert.vert"
            "${CMAKE_CURRENT_SOURCE_DIR}/test/frag.frag"
            )

#execute_process(COMMAND "./compile.sh"
#				WORKING_DIRECTORY "test")

#file(COPY "test/Shaders/"
#     DESTINATION "Shaders/")

target_include_directories(test PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}/include/"
						   "/usr/local/include/"
                           "/Users/eprager/local/include/"
                           "/opt/local/include/"
                           )

target_link_directories(test PUBLIC
                        "/usr/local/lib/"
						"/Users/eprager/local/lib/"
                        "/opt/local/lib/"
                        )

set_property(TARGET test PROPERTY CXX_STANDARD 20)

target_link_libraries(test SDL2 mattresses vulkan evk)

#set_target_properties(test PROPERTIES OUTPUT_NAME evk)
