#include "core/system.h"
#include "graphics/gfx.h"
#include "core/common.h"
#include <iostream>

void init() {
    if (!gfx::init("Rắn Săn Mồi AI - Phiên Bản Hoàn Chỉnh", WINDOW_W, WINDOW_H)) {
        std::cerr << "Lỗi: Không thể khởi tạo GFX.\n";
        exit(1);
    }
    
    // Ban da cung cap file assets/CascadiaCode.ttf
    if (!gfx::add_font("small", "CascadiaCode.ttf", 18) ||
        !gfx::add_font("medium", "CascadiaCode.ttf", 24) ||
        !gfx::add_font("large", "CascadiaCode.ttf", 48)) {
        std::cerr << "Lỗi: Không tìm thấy font assets/CascadiaCode.ttf hoặc không thể mở." << std::endl;
        gfx::shutdown();
        exit(1);
    }
}

void cleanup() {
    gfx::shutdown();
}
