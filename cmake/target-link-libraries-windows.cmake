target_link_libraries(ShadowMapping glad glfw3dll)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
   target_link_libraries(ShadowMapping FreeImaged)
else()
   target_link_libraries(ShadowMapping FreeImage)
endif()