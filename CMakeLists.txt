# Yêu cầu CMake 3.25+ (giống meow-vm) 
cmake_minimum_required(VERSION 3.25)
project(SnakeGameAI LANGUAGES CXX)

# Thêm tùy chọn ENABLE_UNITY_BUILD (giống meow-vm) 
option(ENABLE_UNITY_BUILD "Enable Unity/ Jumbo build to reduce compiler overhead" ON)

# Đặt tiêu chuẩn C++20 (giống meow-vm) 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Tìm SDL2 & SDL2_ttf ---
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)  # vẫn giữ, nhưng xử lý fallback khi không có imported target
message(STATUS "Found SDL2: ${SDL2_LIBRARY}")
message(STATUS "Found SDL2_ttf: ${SDL2_TTF_LIBRARY}") 

# --- GFX (object library), tìm sources trong src/graphics nếu bạn đã di chuyển files ---
# file(GLOB GFX_SOURCES
#     "src/graphics/*.cpp"
# )

# if (GFX_SOURCES)
#     add_library(gfx_objects OBJECT ${GFX_SOURCES})
#     target_include_directories(gfx_objects PUBLIC
#         "${PROJECT_SOURCE_DIR}/include/graphics"
#     )
#     message(STATUS "GFX: Found ${GFX_SOURCES}")
# else()
#     message(WARNING "Không tìm thấy file nguồn GFX nào trong 'src/graphics/'.")
# endif()

# --- Executable ---
add_executable(snake_game
    "src/main.cpp"
)

set_target_properties(snake_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

target_include_directories(snake_game PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include"
)

# # --- Link GFX objects only if created ---
# if (TARGET gfx_objects)
#     target_link_libraries(snake_game PRIVATE
#         $<TARGET_OBJECTS:gfx_objects>
#     )
# endif()

# --- Link SDL2 and SDL2_ttf robustly ---
# SDL2
if (TARGET SDL2::SDL2)
    target_link_libraries(snake_game PRIVATE SDL2::SDL2)
elseif (DEFINED SDL2_LIBRARY)
    target_link_libraries(snake_game PRIVATE ${SDL2_LIBRARY})
else()
    message(WARNING "SDL2 target/library not found. You may need to set it up.")
endif()

# SDL2_ttf: prefer imported target, else fallback to variable(s)
if (TARGET SDL2::SDL2_ttf)
    target_link_libraries(snake_game PRIVATE SDL2::SDL2_ttf)
elseif (TARGET SDL2_ttf::SDL2_ttf)
    target_link_libraries(snake_game PRIVATE SDL2_ttf::SDL2_ttf)
elseif (DEFINED SDL2_TTF_LIBRARY)
    target_link_libraries(snake_game PRIVATE ${SDL2_TTF_LIBRARY})
elseif (DEFINED SDL2_TTF_LIBRARIES)
    target_link_libraries(snake_game PRIVATE ${SDL2_TTF_LIBRARIES})
else()
    message(WARNING "SDL2_ttf: no imported target found and no SDL2_TTF_LIBRARY variable set.")
endif()

# --- PCH (update path to where you moved pch.h) ---
set(PCH_HEADER "${PROJECT_SOURCE_DIR}/include/core/pch.h")
if (EXISTS "${PCH_HEADER}")
    message(STATUS "PCH: Tìm thấy ${PCH_HEADER} -> Đang bật PCH.")
    target_precompile_headers(snake_game PRIVATE "${PCH_HEADER}")
    if (TARGET gfx_objects)
        target_precompile_headers(gfx_objects PRIVATE "${PCH_HEADER}")
    endif()
else()
    message(STATUS "PCH: Không tìm thấy ${PCH_HEADER} -> Đã tắt PCH.")
endif()

# --- Unity Build ---
# 
if (ENABLE_UNITY_BUILD)
    message(STATUS "UNITY_BUILD: Đã bật (có thể tăng tốc build).")
    set_property(TARGET snake_game PROPERTY UNITY_BUILD ON)
    if (TARGET gfx_objects)
        set_property(TARGET gfx_objects PROPERTY UNITY_BUILD ON)
    endif()
else()
    message(STATUS "UNITY_BUILD: Đã tắt.")
endif()

# --- Sao chép Assets ---
# Quan trọng: Sao chép tài nguyên vào thư mục 'bin' để game có thể chạy
message(STATUS "Đang sao chép assets vào ${PROJECT_BINARY_DIR}/bin")
file(COPY "assets/CascadiaCode.ttf" DESTINATION "${PROJECT_BINARY_DIR}/bin")
file(COPY "assets/style.css" DESTINATION "${PROJECT_BINARY_DIR}/bin/assets")
# (Thêm bất kỳ file assets nào khác ở đây, ví dụ: "expert_data.bin")
# file(COPY "expert_data.bin" DESTINATION "${PROJECT_BINARY_DIR}/bin" ERROR_QUIET)