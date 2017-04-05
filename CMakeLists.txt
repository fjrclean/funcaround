cmake_minimum_required(VERSION 2.8)

project(cppgame)

set(SHARED_SRC )
set(CMAKE_C_FLAGS "-Wformat -m64")

add_executable(sv_${PROJECT_NAME} ${SHARED_SRC} "server.c")
add_executable(cl_${PROJECT_NAME} ${SHARED_SRC} "client.c")

#add_executable(test_actors_${PROJECT_NAME} ${SHARED_SRC} "test_actors.cpp")



include_directories (${PROJECT_SOURCE_DIR})

find_package(GLEW)
if(GLEW_FOUND)
    target_link_libraries(cl_${PROJECT_NAME} ${GLEW_LIBRARIES})
    include_directories(${GLEW_INCLUDE_DIRS})
endif(GLEW_FOUND)

find_package (OpenGL)
if (OPENGL_GLU_FOUND)
    target_link_libraries (cl_${PROJECT_NAME} ${OPENGL_glu_LIBRARY})
endif (OPENGL_GLU_FOUND)

find_package (glfw3 REQUIRED)
#if (OPENGL_GLU_FOUND)
    target_link_libraries (cl_${PROJECT_NAME} ${OPENGL_gl_LIBRARY} glfw)
#endif (OPENGL_GLU_FOUND)

find_package(PkgConfig REQUIRED)
if (PKG_CONFIG_FOUND)
	pkg_search_module(JSONCPP REQUIRED jsoncpp)
	include_directories (${JSONCPP_INCLUDE_DIRS})
	target_link_libraries (sv_${PROJECT_NAME} ${JSONCPP_STATIC_LIBRARIES})
	target_link_libraries (cl_${PROJECT_NAME} ${JSONCPP_STATIC_LIBRARIES})

#	pkg_search_module(GLFW REQUIRED glfw3)
#	include_directories (${GLFW_INCLUDE_DIRS})
#	target_link_libraries (cl_${PROJECT_NAME} ${GLFW_STATIC_LIBRARIES})

endif (PKG_CONFIG_FOUND)

#add_custom_target(CODEBLOCKS_PROJECT ALL
#   COMMAND cmake . -G"CodeBlocks - Unix Makefiles"
#)

# add target to generate various documentation:
# generate TODO.md list from git grep
# only supports single lines with @todo in them for now
add_custom_target(doc ALL
	COMMAND rm -f TODO.md
	COMMAND echo "*** Generated from git grep as cmake target, do not edit manually, your changes will be whiped.***" > TODO.md
	COMMAND echo "" >> TODO.md
	COMMAND git grep -n '@todo' -- './*' ':!CMakeLists.txt' > TODO.md
	COMMAND rm -f NOTES.md
	COMMAND echo "*** Generated from git grep as cmake target, do not edit manually, your changes will be whiped.***" > NOTES.md
	COMMAND echo "" >> NOTES.md
	COMMAND git grep -n '@note' -- './*' ':!CMakeLists.txt' > NOTES.md
)
