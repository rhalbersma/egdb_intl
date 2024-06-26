file(RELATIVE_PATH project_relative_path ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
get_filename_component(t ${project_relative_path} PATH)

add_executable(${t}.main main.cpp)

# can be changed to 98 in single-threaded programs
set_target_properties(${t}.main PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

file(GLOB_RECURSE source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ../builddb/*.cpp
    ../egdb/*.cpp
    ../engine/*.cpp
    ../Huffman/*.cpp
    ../Re-pair/*.cpp
)

target_sources(${t}.main PRIVATE
    ${source_files}
)

target_include_directories(${t}.main PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(${t}.main PRIVATE
    pthread
)
