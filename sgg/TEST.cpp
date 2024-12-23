#include "headers/Graphics.h"

int main() {
    graphics::createWindow(1280, 720, "Test");
    graphics::setDrawFunction([] {
        graphics::Brush bck;
        bck.fill_color[0] =  0.5f;
        bck.fill_color[1] =  1.0f;
        bck.fill_color[2] =  0.9f;
        drawRect(1280/2,720/2,200,200, bck);
    });
    graphics::startMessageLoop();
}
