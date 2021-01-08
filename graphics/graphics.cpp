#include "graphics.h"
#include "GLbackend.h"


static graphics::GLBackend * engine = nullptr;

namespace graphics
{
	float getDeltaTime()
	{
		return engine->getDeltaTime();
	}

	float getGlobalTime()
	{
		return engine->getGlobalTime();
	}

	void drawRect(float center_x, float center_y, float width, float height, const Brush & brush)
	{
		engine->drawRect(center_x, center_y, width, height, brush);
	}

	void drawLine(float x1, float y1, float x2, float y2, const Brush & brush)
	{
		engine->drawLine(x1, y1, x2, y2, brush);
	}

	bool setFont(std::string fontname)
	{
		return engine->setFont(fontname);
	}

	void drawText(float pos_x, float pos_y, float size, const std::string & text, const Brush & brush)
	{
		engine->drawText(pos_x, pos_y, size, text, brush);
	}

	void drawDisk(float x, float y, float radius, const Brush & brush)
	{
		engine->drawSector(x, y, 0, 360, 0.0f, radius, brush);
	}

	void drawSector(float cx, float cy, float radius1, float radius2, float start_angle, float end_angle, const Brush & brush)
	{
		engine->drawSector(cx, cy, start_angle, end_angle, radius1, radius2, brush);
	}

	void setOrientation(float angle)
	{
		engine->setOrientation(angle);
	}

	void setScale(float sx, float sy)
	{
		engine->setScale(sx, sy, 1.0f);
	}

	void resetPose()
	{
		engine->resetPose();
	}

	void playSound(std::string soundfile, float volume, bool looping)
	{
		engine->playSound(soundfile, volume, looping);
	}

	void stopMusic(int fade_time)
	{
		engine->stopMusic(fade_time);
	}

	void playMusic(std::string soundfile, float volume, bool looping, int fade_time )
	{
		engine->playMusic(soundfile, volume, looping, fade_time);
	}

	void createWindow(int width, int height, std::string title)
	{
		if (!engine)
			engine = new GLBackend(width, height, title);
		engine->init();
		engine->show(true);
	}

	void setWindowBackground(Brush style)
	{
		engine->setBackgroundColor(style.fill_color[0], style.fill_color[1], style.fill_color[2]);
	}

	void destroyWindow()
	{
		engine->cleanup();
		delete engine;
		engine = nullptr;
	}

	void startMessageLoop()
	{
		bool running = true;
		engine->draw();
		while (running)
		{
			running = engine->processMessages();
			
		}
	}

	void setCanvasSize(float w, float h)
	{
		engine->setCanvasSize(w, h);
	}

	void setCanvasScaleMode(scale_mode_t sm)
	{
		engine->setCanvasMode((int)sm);
	}

	void setFullScreen(bool fs)
	{
		engine->setFullscreen(fs);
	}

	float windowToCanvasX(float x, bool clamped)
	{
		return engine->WindowToCanvasX(x, clamped);
	}

	float windowToCanvasY(float y, bool clamped)
	{
		return engine->WindowToCanvasY(y, clamped);
	}

	void setDrawFunction(std::function<void()> fdraw)
	{
		engine->setDrawCallback(fdraw);
	}

	void setUpdateFunction(std::function<void(float)> fupdate)
	{
		engine->setIdleCallback(fupdate);
	}

	void setResizeFunction(std::function<void(int, int)> fresize)
	{
		engine->setResizeCallback(fresize);
	}

	void setUserData(const void* user_data) {
		engine->setUserData(user_data);
	}

	void* getUserData() {
		return engine->getUserData();
	}

	void getMouseState(MouseState & ms)
	{
		ms.dragging = engine->isMouseDragging();
		bool ba[3]; 
		engine->getMouseButtonState(ba);
		ms.button_left_down = ba[0];
		ms.button_middle_down = ba[1];
		ms.button_right_down = ba[2];
		engine->getMouseButtonPressed(ba);
		ms.button_left_pressed = ba[0];
		ms.button_middle_pressed = ba[1];
		ms.button_right_pressed = ba[2];
		engine->getMouseButtonReleased(ba);
		ms.button_left_released = ba[0];
		ms.button_middle_released = ba[1];
		ms.button_right_released = ba[2];
		int x, y;
		engine->getMousePosition(&x, &y);
		ms.cur_pos_x = x;
		ms.cur_pos_y = y;
		engine->getPrevMousePosition(&x, &y);
		ms.prev_pos_x = x;
		ms.prev_pos_y = y;
	}
	bool getKeyState(scancode_t key)
	{
		return engine->getKeyState(key);
	}
}

