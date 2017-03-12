cmake_minimum_required(VERSION 2.8)

project(cppgame)


set(SHARED_SRC "WorldObject.cpp" "Actor.cpp")

add_executable(sv_${PROJECT_NAME} ${SHARED_SRC} "server.cpp")
add_executable(cl_${PROJECT_NAME} "client.cpp")

find_package(PkgConfig REQUIRED)
if (PKG_CONFIG_FOUND)
	pkg_search_module(JSONCPP REQUIRED jsoncpp)
	include_directories (${JSONCPP_INCLUDE_DIRS})
	target_link_libraries (sv_${PROJECT_NAME} ${JSONCPP_STATIC_LIBRARIES})
	target_link_libraries (cl_${PROJECT_NAME} ${JSONCPP_STATIC_LIBRARIES})
endif (PKG_CONFIG_FOUND)
