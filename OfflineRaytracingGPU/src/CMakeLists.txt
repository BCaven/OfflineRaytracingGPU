add_executable(${NAME}
    VulkanHelper.cpp
    EngineObject.cpp
    ${CMAKE_SOURCE_DIR}/assets/shader.slang)

target_include_directories(${NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/tinyobj
    SYSTEM $ENV{VULKAN_SDK}/include)

target_compile_definitions(${NAME} PRIVATE VK_NO_PROTOTYPES)
if (WIN32)
    target_compile_definitions(${NAME} PRIVATE _CRT_SECURE_NO_WARNINGS VK_USE_PLATFORM_WIN32_KHR NOMINMAX _USE_MATH_DEFINES)
elseif (LINUX)
    target_compile_definitions(${NAME} PRIVATE VK_USE_PLATFORM_WAYLAND_KHR NOMINMAX _USE_MATH_DEFINES)
else()
    target_compile_definitions(${NAME} PRIVATE NOMINMAX _USE_MATH_DEFINES)
endif()

if (MSVC)
    target_compile_options(${NAME} PRIVATE /EHsc)
endif()

set_target_properties(${NAME} PROPERTIES DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_compile_features(${NAME} PRIVATE cxx_std_20)
target_link_libraries(${NAME} PRIVATE ${SDL_LIBRARY} ktx ${Slang_LIBRARY})