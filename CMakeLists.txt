cmake_minimum_required(VERSION 3.3)

PROJECT (FemTech)

# set install rules (windows only)
if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

include_directories(include)

option(ENABLE_MPI "Enable MPI parallelization" ON)

#SET(PARALLEL 0 CACHE INTEGER "Defines a Parallel Build")

# Define a set of values to identify each MPI implementation supported (when enabled)
#set(MS_MPI "Microsoft")
#set(INTEL_MPI "Intel")
#set(DEFAULT_MPI "Default")  # Let's cmake automatically pick one

# Define the MPI to be used at each environment (when enabled ENABLE_MPI)
#set(SELECTED_MPI ${DEFAULT_MPI})
# At windows use MS one by default
#if(WIN32)
#	set(SELECTED_MPI ${MS_MPI})
#        message("Set MS_MPI") # debugging purposes
#endif(WIN32)

if(ENABLE_MPI)
#	set(MPI_IMPLEMENTATION ${SELECTED_MPI} CACHE STRING "MPI Implementation used by the software")
#	set_property(CACHE MPI_IMPLEMENTATION PROPERTY STRINGS ${MS_MPI} ${INTEL_MPI} ${DEFAULT_MPI})

#	message("MPI_IMPLEMENTATION: ${MPI_IMPLEMENTATION}") # debugging purposes

	find_package(MPI REQUIRED)

#	if("${MPI_IMPLEMENTATION}" STREQUAL "${MS_MPI}")
#		# Modify the compiler obtained by find_package with the user defined settings
#		set(MS_MPI_EXECUTABLE "C:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe")
#		set(MPIEXEC_EXECUTABLE ${MS_MPI_EXECUTABLE})
		#message("MPIEXEC_EXECUTABLE: ${MPIEXEC_EXECUTABLE}") # debugging purposes
#	endif()

	include_directories(${MPI_INCLUDE_PATH})
	set(CMAKE_C_FLAGS "${CMAKE_FLAGS} ${MPI_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_EXE_LINKER_FLAGS}")
	add_subdirectory(third-party/parmetis-4.0.3)
	include_directories(third-party/parmetis-4.0.3/include)
	include_directories(third-party/parmetis-4.0.3/metis/include)
#	unset(PARALLEL CACHE)
#	SET(PARALLEL 1 CACHE INTEGER "Defines a Parallel Build")
#	add_definitions( -DPARALLEL=${PARALLEL} )
endif(ENABLE_MPI)


#============================================================================
# BLAS/Lapack
#==============================================================================
find_package(BLAS)
find_package(LAPACK)
if(LAPACK_FOUND)
  message("Found Lapack")
  message("Lapack link library is ${LAPACK_LIBRARIES}")
endif()
if(BLAS_FOUND)
  message("Found BLAS")
  message("BLAS link library is ${BLAS_LIBRARIES}")
endif()
if(LAPACK_FOUND AND BLAS_FOUND)
   set(lapackblas_libraries ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
endif()
#=============================================================================



ADD_SUBDIRECTORY (src)




#===============================================================================
# Examples and tests
#===============================================================================
option(EXAMPLES "Activate examples" OFF)
if(EXAMPLES)
  add_subdirectory(examples)
endif()
