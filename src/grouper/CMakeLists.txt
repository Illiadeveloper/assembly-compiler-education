add_library(assembly_grouper STATIC Grouper.cpp)
add_library(assembly::grouper ALIAS assembly_grouper)
target_include_directories(assembly_grouper PUBLIC ${PROJECT_SOURCE_DIR}/include)
