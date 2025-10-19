

// ========== example_main.cpp ==========
// Minimal demo using the gfx API. Put this next to gfx.h/gfx.cpp and compile/link with SDL2/TTF.


#include "gfx.h"
#include <iostream>

int main() {
    if (!gfx::init("Demo", 800, 600)) return -1;
    if (!gfx::add_font("ui", "assets/Roboto-Regular.ttf", 18)) {
        std::cerr << "Failed to load font";
        // continue anyway; draw_text will no-op
    }

    gfx::texture_id logo = gfx::create_texture_from_bmp("assets/logo.bmp");

    bool running = true;
    while (running) {
        gfx::event e;
        while ((e = gfx::poll_event()).type != gfx::event::Type::NONE) {
            if (e.type == gfx::event::Type::QUIT) running = false;
            if (e.type == gfx::event::Type::KEYDOWN && e.key == gfx::key::ESCAPE) running = false;
            if (e.type == gfx::event::Type::KEYDOWN && e.key == gfx::key::F) gfx::toggle_fullscreen();
        }

        gfx::begin_draw(gfx::colors::NightSky);
        gfx::draw_text("Xin chao the gioi!", "ui", 20, 20, gfx::colors::White);
        if (logo) gfx::draw_texture(logo, 400, 300, 0, 0, 0.0, true);
        gfx::end_draw();

        gfx::delay(16);
    }

    if (logo) gfx::destroy_texture(logo);
    gfx::shutdown();
    return 0;
}
