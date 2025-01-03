#include "headers/Graphics.h"

int main() {
    graphics::createWindow(1280, 720, "Test");
    graphics::setFont(R"(C:\Users\stili\Desktop\FakeMinecraft\assets\fonts\arial.ttf)");
    graphics::setDrawFunction([] {
        graphics::Brush bck;
        bck.fill_color[0] =  0.5f;
        bck.fill_color[1] =  1.0f;
        bck.fill_color[2] =  0.9f;
        drawRect(1280/2,720/2,200,200, bck);
        drawText(1280/2,720/2, 200, "fagget", bck);
    });
    graphics::startMessageLoop();
}
