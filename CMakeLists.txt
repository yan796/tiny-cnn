#Command line options
#-DUSE_TBB=ON/OFF  (default off)
#-DUSE_OMP=ON/OFF  (default off)
#-DUSE_AF=ON/OFF  (default off)
#-DUSE_SSE=ON/OFF  (default on)
#-DUSE_AVX=ON/OFF  (default on)



# ----------------------------------------------------------------------------
#   MACRO definition
# ----------------------------------------------------------------------------


MACRO(GET_HDRS ROOT_DIR TINY_CNN_HRDS)
    SET(${TINY_CNN_HRDS} "")  #empty variable
    file(GLOB HDRS "${ROOT_DIR}*.h")
    foreach(l ${TINY_CNN_HRDS})
        list(APPEND ${TINY_CNN_HRDS} "${ROOT_DIR}${HDRS}" )
    endforeach()
ENDMACRO()

    ###macro needed for ArrayFire
    # A macro to build an ArrayFire example
    # For most uses only FIND_PACKAGE(ArrayFire REQUIRED), ADD_EXECUTABLE(...)
    # and TARGET_LINK_LIBRARIES(... ${ARRAYFIRE_LIBRARIES}) are needed
    MACRO(BUILD_FILE EXAMPLE_NAME EXAMPLE_SOURCE BACKEND_NAME BACKEND_LIBRARIES OTHER_LIBRARIES OUT_DIR_NAME)
        ADD_EXECUTABLE(${EXAMPLE_NAME}_${BACKEND_NAME} ${EXAMPLE_SOURCE} )
        TARGET_LINK_LIBRARIES(${EXAMPLE_NAME}_${BACKEND_NAME} ${BACKEND_LIBRARIES} ${OTHER_LIBRARIES})
        # SET_TARGET_PROPERTIES(example_${EXAMPLE_NAME}_${BACKEND_NAME}
        #     PROPERTIES
        #     OUTPUT_NAME ${EXAMPLE_NAME}_${BACKEND_NAME}
        #     RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR_NAME}
        #     FOLDER "Examples/${BACKEND_NAME}")
    ENDMACRO()

    # A macro to build a list of files
    # For most uses only FIND_PACKAGE(ArrayFire REQUIRED), ADD_EXECUTABLE(...)
    # and TARGET_LINK_LIBRARIES(... ${ARRAYFIRE_LIBRARIES}) are needed
    MACRO(BUILD_ALL FILES BACKEND_NAME BACKEND_LIBRARIES OTHER_LIBRARIES)

        FOREACH(FILE ${FILES})
            GET_FILENAME_COMPONENT(EXAMPLE ${FILE} NAME_WE)
            GET_FILENAME_COMPONENT(FULL_DIR_NAME ${FILE} PATH)
            GET_FILENAME_COMPONENT(DIR_NAME ${FULL_DIR_NAME} NAME)

            BUILD_FILE(${EXAMPLE} ${FILE} ${BACKEND_NAME} "${BACKEND_LIBRARIES}" "${OTHER_LIBRARIES}" ${DIR_NAME})
        ENDFOREACH()
    ENDMACRO()




# ----------------------------------------------------------------------------
#   Basic Configuration
# ----------------------------------------------------------------------------
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(tiny_cnn)
set(PROJECT_VERSION "0.0.1")
string(REGEX MATCHALL "[0-9]" PROJECT_VERSION_PARTS "${PROJECT_VERSION}")
list(GET PROJECT_VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
list(GET PROJECT_VERSION_PARTS 1 PROJECT_VERSION_MINOR)
list(GET PROJECT_VERSION_PARTS 2 PROJECT_VERSION_PATCH)
set(PROJECT_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

#------------------------------------------------------
# Build type
#------------------------------------------------------

IF(NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE "Release" )
ENDIF()

#------------------------------------------------------
# Lib Names and Dirs
#------------------------------------------------------

if(WIN32)
    # Postfix of DLLs:
    SET(PROJECT_DLLVERSION "${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${PROJECT_VERSION_PATCH}")
    SET(RUNTIME_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for dlls and binaries")
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for binaries")
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for dlls")
else()
    # Postfix of so's:
    set(PROJECT_DLLVERSION)
    SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX}/lib/cmake/ /usr/lib/cmake)
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

OPTION(USE_TBB 	"Set to ON to use tbb" OFF)
OPTION(USE_OMP 	"Set to ON to use omp" OFF)
OPTION(USE_AF 	"Set to ON to use array fire" OFF)
OPTION(USE_SSE 	"Set to ON to use sse" ON)
OPTION(USE_AVX 	"Set to ON to use avx" ON)

# ----------------------------------------------------------------------------
#   Default variables
# ----------------------------------------------------------------------------
 SET(ROOT_DIR  ${PROJECT_SOURCE_DIR}/tiny_cnn/  )  ##default for all, can be changed if AF is detected
 GET_HDRS("${ROOT_DIR}" "${TINY_CNN_HRDS}")
 SET(FILES example/sample_train.cpp example/sample_test.cpp test/unitary_test.cpp)

# ----------------------------------------------------------------------------
#   Find Dependencies
# ----------------------------------------------------------------------------

IF(USE_TBB)
    find_package(TBB REQUIRED)
    add_definitions(-DCNN_USE_TBB)
    include_directories(${TBB_INCLUDE_DIRS})
    include_directories(${ROOT_DIR})
    link_directories(${TBB_LIBRARY_DIRS})
    #SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} ${TBB_LIBRARIES}  )
    BUILD_ALL("${FILES}" tbb "" "${TINY_CNN_HRDS};${TBB_LIBRARIES}")
ELSEIF(USE_OMP)
find_package(OpenMP  REQUIRED)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        add_definitions( -DCNN_USE_OMP)
        include_directories(${ROOT_DIR})
        BUILD_ALL("${FILES}" omp "" "${TINY_CNN_HRDS}")
ELSEIF(USE_AF)

	FIND_PACKAGE(CUDA QUIET)
    FIND_PACKAGE(OpenCL QUIET)
    FIND_PACKAGE(ArrayFire REQUIRED)

    ###change global for arrayfire
    #the root
    SET(ROOT_DIR  ${PROJECT_SOURCE_DIR}/af_tiny_cnn/  )
    GET_HDRS("${ROOT_DIR}" "${TINY_CNN_HRDS}")
    SET(FILES example/af_sample_train.cpp example/af_sample_test.cpp test/af_unitary_test.cpp)


    MESSAGE(STATUS "EXAMPLES: CPU backend is ON. ${TINY_CNN_HRDS}")

    INCLUDE_DIRECTORIES(${ArrayFire_INCLUDE_DIRS})
    include_directories(${ROOT_DIR})

    if(${ArrayFire_CPU_FOUND})  # variable defined by FIND(ArrayFire ...)
        MESSAGE(STATUS "EXAMPLES: CPU backend is ON.")
        SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} ${ArrayFire_CPU_LIBRARIES} )

        BUILD_ALL("${FILES}" afcpu ${ArrayFire_CPU_LIBRARIES} "${TINY_CNN_HRDS}")

    elseif(TARGET afcpu)        # variable defined by the ArrayFire build tree
        MESSAGE(STATUS "EXAMPLES: CPU backend is ON.")
        SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} afcpu )

        BUILD_ALL("${FILES}" afcpu afcpu "${TINY_CNN_HRDS}")

    else()
        MESSAGE(STATUS "EXAMPLES: CPU backend is OFF. afcpu was not found.")
    endif()

    if (${CUDA_FOUND})
        if(${ArrayFire_CUDA_FOUND})  # variable defined by FIND(ArrayFire ...)
            FIND_LIBRARY( CUDA_NVVM_LIBRARY
              NAMES "nvvm"
              PATH_SUFFIXES "nvvm/lib64" "nvvm/lib"
              PATHS ${CUDA_TOOLKIT_ROOT_DIR}
              DOC "CUDA NVVM Library"
              )
            MESSAGE(STATUS "EXAMPLES: CUDA backend is ON.")

        BUILD_ALL("${FILES}" cuda ${ArrayFire_CUDA_LIBRARIES} "${CUDA_CUBLAS_LIBRARIES};${CUDA_LIBRARIES};${CUDA_cusolver_LIBRARY};${CUDA_CUFFT_LIBRARIES};${CUDA_NVVM_LIBRARY};${CUDA_CUDA_LIBRARY}${TINY_CNN_HRDS}")

         SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} ${ArrayFire_CUDA_LIBRARIES} "${CUDA_CUBLAS_LIBRARIES};${CUDA_LIBRARIES};${CUDA_cusolver_LIBRARY};${CUDA_CUFFT_LIBRARIES};${CUDA_NVVM_LIBRARY};${CUDA_CUDA_LIBRARY}" )
        elseif(TARGET afcuda)        # variable defined by the ArrayFire build tree
            MESSAGE(STATUS "EXAMPLES: CUDA backend is ON.")

            BUILD_ALL("${FILES}" cuda afcuda "${TINY_CNN_HRDS}")

            #BUILD_ALL("${FILES}" cuda afcuda "")
            SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} afcuda )
        else()
            MESSAGE(STATUS "EXAMPLES: CUDA backend is OFF. afcuda was not found")
        endif()
    else()
        MESSAGE(STATUS "EXAMPLES: CUDA backend is OFF. CUDA was not found")
    endif()

    if (${OpenCL_FOUND})
        if(${ArrayFire_OpenCL_FOUND})  # variable defined by FIND(ArrayFire ...)
            MESSAGE(STATUS "EXAMPLES: OPENCL backend is ON.")
            #BUILD_ALL("${FILES}" opencl ${ArrayFire_OpenCL_LIBRARIES} "${OpenCL_LIBRARIES}")
            SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} ${ArrayFire_OpenCL_LIBRARIES} "${OpenCL_LIBRARIES}" )
        elseif(TARGET afopencl)        # variable defined by the ArrayFire build tree
            MESSAGE(STATUS "EXAMPLES: OPENCL backend is ON.")
            #BUILD_ALL("${FILES}" opencl afopencl ${OpenCL_LIBRARIES})
            SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} afopencl ${OpenCL_LIBRARIES} )
        else()
            MESSAGE(STATUS "EXAMPLES: OPENCL backend is OFF. afopencl was not found")
        endif()
    else()
        MESSAGE(STATUS "EXAMPLES: OPENCL backend is OFF. OPENCL was not found")
    endif()

ELSE() #compile single core / no lib
    include_directories(${ROOT_DIR})
    BUILD_ALL("${FILES}" cpu "" "${TINY_CNN_HRDS}")
ENDIF()
# ----------------------------------------------------------------------------
# COMPILER OPTIONS
# ----------------------------------------------------------------------------

if (MSVC)
    IF(USE_SSE )
        add_definitions(-DCNN_USE_SSE)
        SET(EXTRA_C_FLAGS  " ${EXTRA_C_FLAGS} /arch:SSE2 ")
    ENDIF()
    IF(USE_AVX  )
        add_definitions(-DCNN_USE_AVX)
        SET(EXTRA_C_FLAGS  "${EXTRA_C_FLAGS}  /arch:AVX ")
    ENDIF()

ELSE()#MSVC

    SET(EXTRA_C_FLAGS_RELEASE " /Ox ")
    IF(USE_SSE)
        add_definitions(-DCNN_USE_SSE)
        SET(EXTRA_C_FLAGS  " ${EXTRA_C_FLAGS} -msse3 ")
    ENDIF()
    IF(USE_AVX)
        add_definitions(-DCNN_USE_AVX)
        SET(EXTRA_C_FLAGS  "${EXTRA_C_FLAGS} -mavx ")
    ENDIF()
    SET(EXTRA_C_FLAGS " ${EXTRA_C_FLAGS} -std=c++11 ")
    SET(EXTRA_C_FLAGS_RELEASE " -O3 ")
    SET(EXTRA_C_FLAGS_DEBUG " -g3 ")


ENDIF()


    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS_RELEASE "${EXTRA_C_FLAGS_RELEASE}")
    SET(CMAKE_CXX_FLAGS_DEBUG "${EXTRA_C_FLAGS_DEBUG}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_EXE_LINKER_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${EXTRA_EXE_LINKER_FLAGS_RELEASE}")
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${EXTRA_EXE_LINKER_FLAGS_DEBUG}")

 
#------------------------------------------------
# TARGETS
#------------------------------------------------

# LINK_LIBRARIES(${REQUIRED_LIBRARIES} )
# INCLUDE_DIRECTORIES(${ROOT_DIR})

# if (USE_AF)
#    # ADD_EXECUTABLE(af_unitary_test test/af_test.cpp  ${TINY_CNN_HRDS})
# else()
#     ADD_EXECUTABLE(sample_train example/sample_train.cpp ${TINY_CNN_HRDS})
#     ADD_EXECUTABLE(sample_test example/sample_test.cpp  ${TINY_CNN_HRDS})
#     ADD_EXECUTABLE(unitary_test test/unitary_test.cpp  ${TINY_CNN_HRDS})
# endif()


# ----------------------------------------------------------------------------
# display status message for important variables
# ----------------------------------------------------------------------------
message( STATUS )
MESSAGE( STATUS "-------------------------------------------------------------------------------" )
message( STATUS "General configuration for ${PROJECT_NAME} ${PROJECT_VERSION}")
MESSAGE( STATUS "-------------------------------------------------------------------------------" )


#message( STATUS "Boost_FOUND=${Boost_FOUND}   USE_TBB=${USE_TBB} USE_OMP=${USE_OMP} USE_SSE=${USE_SSE} USE_AVX=${USE_AVX}")


MESSAGE( STATUS "-------------------------------------------------------------------------------" )

message( STATUS )
message("    Built as dynamic libs?:" ${BUILD_SHARED_LIBS})
message("    Compiler:"                   "${CMAKE_COMPILER}"   "${CMAKE_CXX_COMPILER}")
message( STATUS "C++ flags (Release):       ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
message( STATUS "C++ flags (Debug):         ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
message( STATUS "CMAKE_CXX_FLAGS:         ${CMAKE_CXX_FLAGS}")
message( STATUS "CMAKE_BINARY_DIR:         ${CMAKE_BINARY_DIR}")

MESSAGE( STATUS )
MESSAGE( STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}" )
MESSAGE( STATUS "REQUIRED_LIBRARIES = ${REQUIRED_LIBRARIES}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
MESSAGE( STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}" )
MESSAGE( STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}")
MESSAGE( STATUS )
MESSAGE( STATUS )
MESSAGE( STATUS "Change a value with: cmake -D<Variable>=<Value>" )
MESSAGE( STATUS )
