option(BUILD_DOCS "Build Doxygen documentation" OFF)

if(BUILD_DOCS) 
  find_package(Doxygen)

  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.public
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating public documetation"
  )

else()
  message(STATUS "Doxygen not found – documentation targets disabled")
endif()
