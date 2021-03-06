find_package(Boost COMPONENTS system filesystem REQUIRED)

aux_source_directory("common" COMMON)
aux_source_directory("types" TYPES)
aux_source_directory("utils" UTILS)

SET(KMSC_SOURCE main.cpp version.cpp MediaServerServiceHandler.cpp
              ModuleLoader.cpp ${COMMON} ${TYPES} ${UTILS})

set(CFLAGS "-DHAVE_NETINET_IN_H -DUSE_BOOST_THREAD -DHAVE_INTTYPES_H ")

set_source_files_properties(${KMSC_SOURCE}
		PROPERTIES COMPILE_FLAGS ${CFLAGS})

add_executable(kurento ${KMSC_SOURCE})
add_dependencies(kurento kmsiface)
add_dependencies(kurento plugins)

target_link_libraries(kurento kmshttpep)
target_link_libraries(kurento kmsiface ${THRIFT_LIBRARIES} ${EVENT_LIBRARIES})
target_link_libraries(kurento ${GSTREAMER_LIBRARIES} ${GLIBMM_LIBRARIES})
target_link_libraries(kurento ${GSTREAMER_SDP_LIBRARIES} )
target_link_libraries(kurento ${UUID_LIBRARIES})
target_link_libraries(kurento ${GLIB2_LIBRARIES} -lpthread)
target_link_libraries(kurento ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
target_link_libraries(kurento ${GMODULE_LIBRARIES})
include_directories(kurento ${CMAKE_SOURCE_DIR}/httpepserver)
include_directories(kurento ${CMAKE_BINARY_DIR})
include_directories(kurento ${THRIFT_INCLUDE_DIRS} ${GLIBMM_INCLUDE_DIRS})
include_directories(kurento ${GSTREAMER_INCLUDE_DIRS})
include_directories(kurento ${UUID_INCLUDE_DIRS})
include_directories(kurento ${KMSIFACE_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/server)
include_directories(kurento ${GMODULE_INCLUDE_DIR})

install(TARGETS kurento RUNTIME DESTINATION bin)
