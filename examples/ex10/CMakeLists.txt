SET(ex10_src ex10.cpp)

ADD_EXECUTABLE(ex10 ${ex10_src})

if(ENABLE_MPI)
  LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/third-party/parmetis-4.0.3/libparmetis)
  TARGET_LINK_LIBRARIES(ex10 FemTech parmetis ${lapackblas_libraries})
else()
  LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)
  TARGET_LINK_LIBRARIES(ex10 FemTech)
endif(ENABLE_MPI)

file(COPY 1-elt-tet.k DESTINATION .)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results/vtu)

# Define debugger options when working with windows & visual studio generator
# We set them here but could be passed as options if required
# We use a template for the same, replacing the options we need
if(WIN32 AND MSVC)
	set( DEBUG_COMMAND "C:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe" )
	set( DEBUG_COMMAND_ARGUMENTS "-n 1 \"$(TargetPath)\" 1-elt-tet.k" )
	configure_file( template.vcxproj.user.in ${CMAKE_BINARY_DIR}/examples/ex10/ex10.vcxproj.user @ONLY )
endif()
