cmake_minimum_required(VERSION 2.8)

project(cppgame)


set(SHARED_SRC "WorldObject.cpp" "Actor.cpp" "network.cpp" "Weapon.cpp" "Human.cpp" "Alien.cpp")

add_executable(sv_${PROJECT_NAME} ${SHARED_SRC} "server.cpp")
add_executable(cl_${PROJECT_NAME} ${SHARED_SRC} "client.cpp")

#add_executable(test_actors_${PROJECT_NAME} ${SHARED_SRC} "test_actors.cpp")

include_directories (${PROJECT_SOURCE_DIR})

find_package(PkgConfig REQUIRED)
if (PKG_CONFIG_FOUND)
	pkg_search_module(JSONCPP REQUIRED jsoncpp)
	include_directories (${JSONCPP_INCLUDE_DIRS})
	target_link_libraries (sv_${PROJECT_NAME} ${JSONCPP_STATIC_LIBRARIES})
	target_link_libraries (cl_${PROJECT_NAME} ${JSONCPP_STATIC_LIBRARIES})

	pkg_search_module(GLFW REQUIRED glfw3)
	include_directories (${GLFW_INCLUDE_DIRS})
	target_link_libraries (cl_${PROJECT_NAME} ${GLFW_STATIC_LIBRARIES})

endif (PKG_CONFIG_FOUND)

find_package(GLEW)
if(GLEW_FOUND)
    target_link_libraries(cl_${PROJECT_NAME} ${GLEW_LIBRARIES})
    include_directories(${GLEW_INCLUDE_DIRS})
endif(GLEW_FOUND)

find_package (OpenGL)
if (OPENGL_GLU_FOUND)
    target_link_libraries (cl_${PROJECT_NAME} ${OPENGL_glu_LIBRARY})
endif (OPENGL_GLU_FOUND)
