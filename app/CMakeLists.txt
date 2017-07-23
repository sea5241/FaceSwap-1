
cmake_minimum_required(VERSION 3.4.1)

#include(src/main/cpp/dlib/cmake)

include_directories(src/main/cpp/include)


add_library(dlib STATIC src/main/cpp/dlib/all/source.cpp)


#编译源文件
file(GLOB my_source_path  src/main/cpp/*.cpp src/main/cpp/*.c)
add_library( native-lib SHARED ${my_source_path} )


add_library( lib_opencv SHARED IMPORTED)
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libopencv_java3.so)

#add_library( lib_dlib SHARED IMPORTED)
#set_target_properties(lib_dlib PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/main/#jniLibs/${ANDROID_ABI}/libdlib.so)


find_library(
              log-lib


              log )

find_library(jnigraphics-lib
        jnigraphics )

target_link_libraries(native-lib
dlib
                       ${log-lib}
                        ${jnigraphics-lib}
                       lib_opencv
		)