project(postgirl)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -std=c++11 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -std=c++11 -O3")

include_directories("include/"
					"third_party/imgui/"
                    "third_party/gl3w/"
                    )

file(GLOB all_postgirl_src
    "src/main.cpp"
	"third_party/imgui/*.cpp"
    "third_party/gl3w/GL/gl3w.c"
)

find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

add_executable(postgirl ${all_postgirl_src})
target_link_libraries(postgirl GL ${GLFW_STATIC_LIBRARIES} ${CURL_LIBRARIES})
