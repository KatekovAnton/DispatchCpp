cmake_minimum_required(VERSION 2.8.8)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_NAME dispatchcpp)
set(TARGET_NAME ${PROJECT_NAME})
project(${PROJECT_NAME} C CXX)



IF(APPLE)
    SET (PROJECT_OS_MACOS TRUE BOOL INTERNAL)
    add_definitions(-DPROJECTN_TARGET_OS_MAC )
ENDIF (APPLE)
IF(WIN32)
    SET (PROJECT_OS_WIN32 TRUE BOOL INTERNAL)
    add_definitions(-DPROJECTN_TARGET_OS_WIN )
ENDIF (WIN32)
if(UNIX AND NOT APPLE) 
    SET (PROJECT_OS_LINUX TRUE BOOL INTERNAL)
    add_definitions(-DPROJECTN_TARGET_OS_LINUX )
ENDIF (UNIX AND NOT APPLE)



if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()



# platform-independant sources
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
include_directories("${SRC_DIR}")
include_directories("${SRC_DIR}/platform")

file(GLOB_RECURSE PROJECT_SOURCES 
    ${SRC_DIR}/*.cpp
    ${SRC_DIR}/*.hpp
    ${SRC_DIR}/*.h
)

# platform-dependant sources
if (PROJECT_OS_MACOS)
    # tested with Xcode 
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/mac/platform_mac.mm")
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/mac/platform_mac.h")
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/mac/smc.h")
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/mac/SMCWrapper.h")
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/mac/SMCWrapper.m")

    include_directories("${SRC_DIR}/platform/mac")
endif()
if (PROJECT_OS_WIN32)
    # tested with
    # cmake -G "Visual Studio 16 2019" ..
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/win32/platform_win.cpp")
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/win32/platform_win.h")
    
    include_directories("${SRC_DIR}/platform/win32")
endif()
if (PROJECT_OS_LINUX)
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/linux/platform_lin.cpp")
    list (APPEND PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/platform/linux/platform_lin.h")
    
    include_directories("${SRC_DIR}/platform/linux")
endif()



if(CMAKE_GENERATOR STREQUAL Xcode)

    add_executable(${PROJECT_NAME} MACOSX_BUNDLE 
        ${PROJECT_SOURCES})

    # set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/Sources/Game/Mac/GameInfo.plist)

elseif(PROJECT_OS_MACOS)

    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14")
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE 
        ${PROJECT_SOURCES})

    # set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/Sources/Game/Mac/GameInfo.plist)

    add_custom_command(TARGET ${PROJECT_NAME} 
        POST_BUILD COMMAND 
        ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../../Frameworks/"
        $<TARGET_FILE:${PROJECT_NAME}>)

elseif(WIN32)

    add_executable(${PROJECT_NAME} WIN32 
        ${PROJECT_SOURCES})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS /SUBSYSTEM:CONSOLE
    )
        
else ()

    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
    
endif()



if (PROJECT_OS_MACOS)

    list(APPEND SRE_INCLUDE_DIRS /System/Library/Frameworks)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-reinterpret-base-class -Wno-inconsistent-missing-override)
    
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework CoreVideo")
    target_link_libraries(${PROJECT_NAME} "-framework AppKit")
    target_link_libraries(${PROJECT_NAME} "-framework Carbon")
    target_link_libraries(${PROJECT_NAME} "-framework ForceFeedback")
    target_link_libraries(${PROJECT_NAME} "-framework SystemConfiguration")
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework GLKit")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
    target_link_libraries(${PROJECT_NAME} "-framework Security")
    target_link_libraries(${PROJECT_NAME} "-framework AudioToolbox")
    target_link_libraries(${PROJECT_NAME} "-framework CoreAudio")
    target_link_libraries(${PROJECT_NAME} "-lbsm")
    target_link_libraries(${PROJECT_NAME} "-lc")
    target_link_libraries(${PROJECT_NAME} libiconv.dylib)
    
endif()
if (PROJECT_OS_LINUX)

    target_link_libraries(${PROJECT_NAME} pthread)

endif()
if (PROJECT_OS_WIN32)

    target_link_libraries(${PROJECT_NAME} dbghelp)

    set(LIBS opengl32 gdi32 ole32 user32 winmm imm32 version setupapi)
    target_link_libraries(${PROJECT_NAME} ${LIBS})

endif()
