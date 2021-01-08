#pragma once
#include <string>
#include <functional>
#include "scancodes.h"

/** \file graphics.h This is the *only* library header file that is required to be included by your application.
	It contains the declaration of all SGG library functions and data structures.
*/

/** \namespace graphics includes all functions of the Simple Game Graphics library.
*/
namespace graphics
{
	/** The available modes for adapting the canvas to the application window.
	*/
	typedef enum {
		CANVAS_SCALE_WINDOW = 0,
		CANVAS_SCALE_STRETCH,
		CANVAS_SCALE_FIT
	}
	scale_mode_t;

	/** Encapsulates the superset of drawing attributes for all supported primitives and draw calls. These include 
	    the primary fill color, the use of gradient fill or not, the secondary fill color and fill direction used 
		by the gradient, a texture image to be blended with the underlying color, the outline color and width and 
		the opacity for the fill and outline colors. The use of these attributes is demonstrated in the example 
		image below:

	    \image html brush1.jpg
	*/
	struct Brush
	{
		float fill_color[3] = {1.f,1.f,1.f};               ///< The primary fill color for shapes.
														   ///<

		float fill_secondary_color[3] = { 1.f,1.f,1.f };   ///< The fill color used as a secondary one when using a color gradient.
														   ///<

		float fill_opacity = 1.0f;                         ///< The opacity (1-transparency) of the primary fill color. A (maximum)
														   ///< value of 1.0f means "fully opaque", while a value of 0.0f means 
											               ///< fully transparent (invisible).
		
		float fill_secondary_opacity = 1.f;				   ///< The opacity of the second fill color, used for the gradient.
														   ///<

		float outline_color[3] = { 1.f,1.f,1.f };          ///< The color of the shape outline (where applicable). Most drawn
														   ///< shapes, like disks, line segments and rectangles use this 
														   ///< property to set the outline stroke color.

		float outline_opacity = 1.f;                       ///< The opacity of the outline. Maximum value is 1.0f (fully opaque),
														   ///< and the minimum value is 0.0f (fully transparent). 

		float outline_width = 1.0f;						   ///< The stroke width in pixel units. Can accept fractional values. 
														   ///< 

		std::string texture = "";                          ///< The filename of a bitmap to project over and blend with the 
														   ///< underlying color of the fill. The image is multiplicatively
														   ///< combined with the entire fill motif, respecting also the opacity.
														   ///< Currently, only PNG images are supported, with transparency. 
														   ///< Images of arbitrary size are supported but are internally upscaled
														   ///< to the nearest power of two in each dimension, using linear 
														   ///< interpolation. The bitmap, regardless of its aspect ratio,
														   ///< covers the entire shape end to end, according to its own 
														   ///< parameterization. This means that a square image drawn on a non-square
														   ///< rectangle will stretch the image. To avoid this, the drawn rectangle
														   ///< should follow the aspect ratio of the image. By default, no image is 
														   ///< used. When an image filename is provided, it is loaded once and 
														   ///< internally cached for repeated use. 
														   ///< 
														   ///< Keep in mind that when using a bitmap on a drawable shape, the 
														   ///< image parametric space (expressed as u,v coordinates in the image below)
														   ///< is mapped to corresponding parameters of the underlying shape.
														   ///< For example, for the disk shape, the horizontal axis of the image is 
														   ///< mapped to the circumferance of the disk, while the height is mapped to
														   ///< the disk radius. 
														   ///< \image html uv.jpg

		bool gradient = false;							   ///< Enables or disables the gradient fill of a shape.
														   ///<

		float gradient_dir_u = 0.0f;					   ///< The gradient by default uses the v dimension of the parametric shape.
														   ///< In practice though, the calculation is as follows: s = u*gradient_dir_u + v*gradient_dir_v,
														   ///< where s is the color mixing parameter, clamped to [0.0,1.0].
														   ///< This means that we can change the linear parameters gradient_dir_u/v to bias the 
														   ///< between the two parametric space axes. 
														   ///< For example:
														   ///< \image html gradient.jpg

		float gradient_dir_v = 1.0f;					   ///< \see gradient_dir_u
	};

	/** Includes the state of the pointing device as polled by the message processing loop of the library. 
	
		This state can be polled using the getMouseState function. Typically, the mouse (and keyboard) events are
		polled from within the update callback:

		\code{.cpp}
		graphics::MouseState mouse;
		graphics::getMouseState(mouse);
		if (mouse.button_left_pressed)
		{
			// call custom code here to respond to the left mouse button press.
		}
	    \endcode
	*/
	struct MouseState
	{
		bool button_left_pressed;	///< The left button went from an "depressed" state to a "pressed" one during the last poll cycle.
		bool button_middle_pressed; ///< The middle button went from an "depressed" state to a "pressed" one during the last poll cycle.
		bool button_right_pressed;  ///< The right went from an "depressed" state to a "pressed" one during the last poll cycle.
		bool button_left_released;  ///< The left button went from an "pressed" state to a "released" one during the last poll cycle.
		bool button_middle_released;///< The middle button went from an "pressed" state to a "released" one during the last poll cycle.
		bool button_right_released; ///< The right button went from an "pressed" state to a "released" one during the last poll cycle.
		bool button_left_down;		///< The left button is currently held down.
		bool button_middle_down;	///< The middle button is currently held down.
		bool button_right_down;	    ///< The right button is currently held down.
		bool dragging;				///< The left button is currently held down and the cursor is moving.
		int cur_pos_x;				///< The current x position in pixel units of the pointing device.
		int cur_pos_y;				///< The current y position in pixel units of the pointing device.
		int prev_pos_x;				///< The x position in pixel units of the pointing device in the previous update cycle.
		int prev_pos_y;				///< The y position in pixel units of the pointing device in the previous update cycle.
	};


	/** \defgroup _WINDOW Window initialization and handling
	* @{
	*/
	
	/** Creates and shows a framed window with a title of specific dimensions.

		This should be the first function call to the SGG API. Internally, it performs library initialization and graphics 
		canvas allocation and displays the window.
		
		\param width is the window canvas width in pixel units.
		\param height is the window canvas height in pixel units.
		\param title is the title displayed on the window frame.
	*/
	void createWindow(int width, int height, std::string title);
	
	/** Sets the color to fill the background of the main window, including the area outside the drawing canvas extents.

		The function can be called at any time (after creating a window). Typical use:

	    \code{.cpp}
		graphics::Brush br;
		br.fill_color[0] = 0.2f;
		br.fill_color[1] = 0.3f;
		br.fill_color[2] = 0.6f;
		graphics::setWindowBackground(br);
		\endcode

		\param style is the graphics::Brush structure defining the background color. Only the fill_color field is used.
	*/
	void setWindowBackground(Brush style);

	/** Destroys the application window and clears all internal resources.

	    This is typically the last SGG API call, before closing the application.
	*/
	void destroyWindow();

	/** Starts the message pump loop of the application window.

		The function processes all window and user interface events and is alos responsible for 
		internally calling the draw callback defined by the user to refresh the visual content of the
		window. The user also defines and provides an optional but typically present application state 
		update function, where all input processing is performed along with time-dependent updates of the
		application logic.

	    The function should be called after all application initialization is done, including the 
		setup of any callback functions to invoke upon the draw, state update and resizing events.
		The function only returns when the application window signals a quit event or the ESC key
		is pressed.

		Typical usage:

		\code{.cpp}
		graphics::createWindow(960, 540, "Night of the Living Arkanoid");
		graphics::setDrawFunction(draw);  // draw is a user-defined function 
		                                  // that issues calls to the library's draw functions 
										  // to paint the window canvas. 
	    graphics::setUpdateFunction(update);  // update is a user-defined function to perform all 
											  // user input polling and application state updates.

		... // application initialization code.

		graphics::startMessageLoop();
		graphics::destroyWindow();
		\endcode
	*/
	void startMessageLoop();

	/** Defines the extents of the drawing canvas in the custom units used by the application.

	    The function explicitly sets the desired width and height of the drawing canvas in the measurement units
		used by the developer of the application, irrespectively of the actual window size and resolution. The 
		contents of the drawing canvas are scaled to fit the window according to the modes available via the 
		setCanvasScaleMode function. For example, the developer may define that the drawing canvas should be
		30 X 20 application units (e.g. cm). The drawing canvas will be then scaled to correspond to the 
		actual window client area, properly adapted to fill the window resolution. 

		Please note that the canvas aspect ratio needs not correspond with the window aspect ratio. The fitting
		is determined by the setCanvasScaleMode function, as mentioned above.

		\param w is the canvas width in custom units.
		\param h is the canvas height in custom units.
	*/
	void setCanvasSize(float w, float h);

	/** Defines how the canvas scales to adapt to the actual window size. 

		SGG supports 3 different modes of scaling the canvas to fit the window. The 3 modes are defined in the scale_mode_t enumeration.

		\param sm is the scaling mode and is one of the following:

		graphics::CANVAS_SCALE_WINDOW: The canvas size used for drawing corresponds to the current window size in pixel units. Custom canvas size 
		provide via the setCanvasSize is ignored. Upon window size change, the canvas is adapted to be equal to the window client area 
		resolution. This mode is useful if the graphics drawn must correspond to the specific image resolution of the window and should not
		be dynamically scaled.

		graphics::CANVAS_SCALE_STRETCH: The canvas is stretched to fill the entire window. When the aspect ratio of tyhe canvas as specified by the 
		user does not match the window aspect ratio, the drawn contents will appear distorted (stretched or compressed).

		 \image html resize2.jpg

		graphics::CANVAS_SCALE_FIT: The canvas is scaled to maximally fit the window but its aspect ratio is not affected. This means that if there 
		is an aspect ration mismatch between the canvas and the window, the canvas will leave equal gaps along the axis of mismatch 
		(centered). The window background outside the canvas will be painted with the user-defined background color. The drawn contents 
		of the canvas will be clipped at the borders of the canvas and will not cross over to the elastic guard space.
		
		\image html resize.jpg
	
	   	\see setWindowBackground
		\see setCanvasSize
	*/
	void setCanvasScaleMode(scale_mode_t sm);
	
	/** Puts the application window in full screen mode.

		This function must immediately follow the window creation, otherwise it may result in an unexpected behavior.

		\param fs should be set to either true for full screen or false for windowed mode.
	*/
	void setFullScreen(bool fs);
	
	/** Converts the horizontal window coordinate of a point to the corresponding canvas coordinate.

		This function is useful for translating window coordinates (e.g. from the MouseState reported position) to the canvas space.

		\param x is the window x coordinate to convert.
		\param clamped specifies whether the coordinates reported are clamped to the extents of the canvas.
		\return the horizontal coordinate in canvas units.

		\see windowToCanvasY
	*/
	float windowToCanvasX(float x, bool clamped = true);

	/** Converts the vertical window coordinate of a point to the corresponding canvas coordinate.

		This function is useful for translating window coordinates (e.g. from the MouseState reported position) to the canvas space.

		\param y is the window y coordinate to convert.
		\param clamped specifies whether the coordinates reported are clamped to the extents of the canvas.
		\return the vertical coordinate in canvas units.

		\see windowToCanvasX
	*/
	float windowToCanvasY(float y, bool clamped = true);

	/** Specifies a user-provided pointer to be stored within the SGG Engine.

	Typically, at the start of the application the user creates a class that holds the application's state. A pointer to this class can be passed to setUserData(). At any later point in the application's lifecycle, the stored pointer can be retrieved using getUserData(). This is very useful for retreiving application data during callbacks for update, draw and resize. 
	
	\param user_data is the pointer that the user wants to store within the SGG engine. This pointer can later be retrieved using getUserData(). 

	\see getUserData

	Potential usage:

	\code{.cpp}
	void resize(int new_w, int new_h)
	{
		...
		Game* the_game = (Game*)graphics::getUserData();
		...
	}

	void update(float ms)
	{
		...
		Game* the_game = (Game*)graphics::getUserData();
		...
	}

	void draw()
	{
		...
		Game* the_game = (Game*)graphics::getUserData();
		...
	}

	struct Game {
		int data_a;
		int data_b;
		...
		...
	};

	int main()
	{
		...
		Game* the_game = new Game();

		graphics::createWindow(960, 540, "Night of the Living Arkanoid");
		graphics::setUserData(the_game);
		...
		return 0;
	}
	\endcode
	*/
	void setUserData(const void* user_data);

	/** Returns the user-submitted pointer that was previously set with setUserData().

	If the memory pointed to by the provided pointer is freed, the user should also be careful and use setUserData(nullptr) to avoid accessing invalid memory later.  

	\return Returns the pointer that was previously submitted through setUserData(). The returned value is of type void*. The user is expected to know which the correct type is to cast it to. If no pointer was previously set, the function returns a nullptr.

	\see setUserData

	*/
	void* getUserData();

	/** @}*/

	/** \defgroup _CALLBACK Callback setup
	* @{
	*/

	/** Specifies a user-defined function to be called each time the window is to be redisplayed.

		The custom draw function is the only one that should contain all SGG draw functions to be 
		used for painting the contents of the canvas. Drawing occurs a) when a window size change is reported,
		b) at predetermied and equally spaced intervals, triggered by the event processing loop of the SGG engine.

		\param draw is the user-defined draw function passed. 

		Typical usage:

		\code{.cpp}
		void draw()
		{
			the_game.render(); // here, "the_game" is a global static variable (class instance) of a custom
							   // class that encapsulates the logic and state of a game.
							   // We call its render() method to perform all relevant drawing.
			
			                   // In addition, here we also add some extra drawing commands on top.
			graphics::Brush br;
			graphics::drawLine(10.0f, 10.0f, 400.0f, 10.0f, br);
		}

		
		void update(float ms)
		{
			... // update is a user-defined function to perform all
			    // user input polling and application state updates.
		}

		int main()
		{
			graphics::createWindow(960, 540, "Night of the Living Arkanoid");
			graphics::setDrawFunction(draw); 
			graphics::setUpdateFunction(update);  

			... // application initialization code.

			graphics::startMessageLoop();
			graphics::destroyWindow();
			return 0;
		}
		\endcode
	*/
	void setDrawFunction(std::function<void()> draw);

	/** Specifies a user-defined function to be called each time the state of the application needs to be updated.

	This callback function is invoked a) when an input event is triggered in the event processing loop of the engine,
	such as mouse state change or keyboard key press, b) at regularly timed intervals, so that time-dependent states
	in the application (such as animations, collisions, simulations) may advance their values.

	Typically, in the update callback function the developer places code for polling the input devices and using the 
	time increment (passed as an argument) to update the application mechanics. Keep also in mind that the time increment 
	passed in the callback is also globally available thoughout the code through the getDeltaTime() function.

	\param update is the user-defined function passed. The callback function must accept a single parameter, the 
	time passed in miliseconds from the last triggered update.

	Typical usage:

	\code{.cpp}
	void draw()
	{
		... // includes all the draw calls for repainting the canvas.
	}

	bool toggle_help = false;

	void update(float ms)
	{
		
		the_game.update(); // here, "the_game" is a global static variable (class instance) of a custom
						   // class that encapsulates the logic and state of a game.
						   // We call its update() method to perform all relevant state changes in internal fields.
						   // Please note here that the time increment ms is not passed to the update() method,
						   // as it internally calls the graphics::getDeltaTime() itself for this purpose. 

		// In addition, check whether the user pressed the F1 function key.
		if (graphics::getKeyState(graphics::SCANCODE_F1))
		{
			// If pressed, toggle a global variable that enables the display of a help message overlay. 
			toggle_help = !toggle_help;
		}
	}

	int main()
	{
		graphics::createWindow(960, 540, "Night of the Living Arkanoid");
		graphics::setDrawFunction(draw);
		graphics::setUpdateFunction(update);
		... // application initialization code.
		graphics::startMessageLoop();
		graphics::destroyWindow();
		return 0;
	}
	\endcode
	*/
	void setUpdateFunction(std::function<void(float)> update );

	/** Specifies a user-defined function to be called each time the window is resized.

	Typically, there is no need to call this function in most situations, as the dimensions of the window and the drawing 
	canvas are independent. Still, in some case, one may want to change what is displayed according to the available 
	screen area, or adapt the aspect ratio of the canvas to match that of the host window. 

	\param resize is the user-defined function passed. The callback function must accept two int parameters, the
	new width and height of the window. 

	Potential usage:

	\code{.cpp}
	void resize(int new_w, int new_h)
	{
		the_game.adjustCanvas(new_w, new_h); // here, "the_game" is a global static variable (class instance) of a custom
						                     // class that encapsulates the logic and state of a game.
						                     // We call its adjustCanvas() method here to rescale the canvas so that it 
											 // matches the aspect ratio of the window. 
		
	}

	int main()
	{
		...
		graphics::createWindow(960, 540, "Night of the Living Arkanoid");
		graphics::setResizeFunction(resize);
		...
		return 0;
	}
	\endcode
	*/
	void setResizeFunction(std::function<void(int, int)> resize);
	/** @}*/

	/** \defgroup _INPUT Input handling
	* @{
	*/
	/** Polls the engine for the current state of the mouse.

	    Stores in the user-provided MouseState record ms, the current status of all mouse events. For more information on what 
		data can be obtained, see the MouseState structure. Typically, this function is used from within the update callback
		of the application.

		\param ms is the user-provided record to update with the current status of the mouse cursor button and higher-level information
		such as mouse "dragging" and button "pressed / released".

		\see MouseState
		\see setUpdateFunction
	*/
	void getMouseState(MouseState & ms);


	/** Polls the engine for the current state of a specific key.
		
		Since buttons can be queried individually, multiple pressed keys can be detected, by querying each one of them. The 
		function shall return true if a button has been reported as pressed by a window event between the previous call to
		the update function and the current call to getKeyState().

		\param key is the scancode ID of the key to query. The available scancodes are listed in the [scancodes.h](@ref scancodes.h)  file.

		\return true if the key is pressed, false otherwise.

		\see setUpdateFunction
	*/
	bool getKeyState(scancode_t key);
	/** @}*/

	/** \defgroup _TIME Time reporting
	* @{
	*/

	/** Returns the time passed from the previous engine state update.

		The function reports the time passed in miliseconds between the current internal state update and the previous one.
		If a user-defined update callback is provided, the time also corresponds to the interval between successive callback
		invocations.

		\return the time elapsed from the previous engine update (coinsides also with the "frame" update interval) in miliseconds.
	*/
	float getDeltaTime();

	/** Reports the absolute application execution time.

	    The time starts counting from the call to create the window. 

		\return the elapsed time passed from the launch of the application window in miliseconds. 
	*/
	float getGlobalTime();
	/** @}*/

	/** \defgroup _GRAPHICS Graphics output
	* @{
	*/

	/** Draws a rectangle.

		Draws a rectangle of size width X height, centered at (center_x, center_y).
		The shape outline and fill attributes are provided in the brush parameter. 

		\param center_x is the x coordinate of the rectangle center in canvas units.
		\param center_y is the y coordinate of the rectangle center in canvas units.
		\param width is the horizontal size of the rectangle center in canvas units.
		\param height is the vertical size of the rectangle center in canvas units.
		\param brush specifies the drawing attributes to use for the outline and fill of the shape.

		\see Brush
	*/
	void drawRect(float center_x, float center_y, float width, float height, const Brush & brush);

	/** Draws a line segment.

		Draws a linear segment between two points on the canvas.
		The outline attributes are specified in the brush parameter.

		\param x1 is the x coordinate of the first point in canvas units.
		\param y1 is the y coordinate of the first point in canvas units.
		\param x2 is the x coordinate of the second point in canvas units.
		\param y2 is the y coordinate of the second point in canvas units.
		\param brush specifies the drawing attributes to use for the outline and fill of the shape.

		\see Brush
	*/
	void drawLine(float x1, float y1, float x2, float y2, const Brush & brush);

	/** Draws a disk.

		Draws a disk (or circle, if fill opacity is set to 0) of certain radius and centered at (cx, cy).
		The outline and fill attributes are specified in the brush parameter.

		\param cx is the x coordinate of the center of the disk in canvas units.
		\param cy is the y coordinate of the center of the disk in canvas units.
		\param radius is the radius of the disk in canvas units.
		\param brush specifies the drawing attributes to use for the outline and fill of the shape.

		\see Brush
	*/
	void drawDisk(float cx, float cy, float radius, const Brush & brush);

	/** Draws a sector of a disk.

		Draws a sector of a disk between an inner and outer radius and two angles. 
		The outline and fill attributes are specified in the brush parameter.

		\code{.cpp}
		// In the draw callback function:
		graphics::Brush br;
		br.fill_color[0] = 1.0f;
		br.fill_color[1] = 0.1f;
		br.fill_color[2] = 0.0f;
		br.fill_secondary_color[0] = 0.0f;
		br.fill_secondary_color[1] = 0.2f;
		br.fill_secondary_color[2] = 1.f;
		br.gradient = true;
		br.gradient_dir_u = 1.0f;
		br.gradient_dir_v = 0.0f;

		graphics::drawSector(20, 20, 5, 10, 0, 90, br);
		\endcode

		\image html sector.jpg

		\param cx is the x coordinate of the center of the disk sector in canvas units.
		\param cy is the y coordinate of the center of the disk sector in canvas units.
		\param radius1 is the inner radius of the disk sector in canvas units.
		\param radius2 is the outer radius of the disk sector in canvas units.
	*/
	void drawSector(float cx, float cy, float radius1, float radius2, float start_angle, float end_angle, const Brush & brush);

	/** Sets the current font for text rendering.

		Notifies the SGG engine to prepare and make current the font typeface in the filename supplied as argument. If the 
		font is not already loaded into SGG, it loads the SGG and builds all necessary information for it. If the font name
		supplied does not correspond to a valid path to a TrueType typeface, the operation is silently aborted and no font 
		is loaded or made active for text rendering. After loading the font, it is made current and all subsequent text drawing
		calls use this font for text rendering. If the font is already loaded into SGG, it is just made active for rendering. This 
		operation does not incur any measurable overhead, as all resource allocation and convesion processes occur once, during the 
		first call to load a specific font. 

		\param fontname is a valid path to the filename of the TrueType font (.ttf extension) to use.

		\return true if the operation was successful, false otherwise.

		\see drawText
	*/
	bool setFont(std::string fontname);

	/** Draws a string of text at a user-provided location on the canvas, using the current font.

		The function displays the glyphs for the text provided, in a left-aligned manner. The coordinates
		passed to the function correspond to the lower left corner of the text box and the height of the 
		glyphs is indicated by the size parameter. If there is no active font (or the setFont function failed),
		no text is drawn. 

		\param pos_x is the horizontal coordinate of the lower left corner of the text in canvas units.
		
		\param pos_y is the vertical coordinate of the lower left corner of the text in canvas units.
		
		\param size is the glyph height of the text to be drawn.
		
		\param text is the string of text to display.
		
		\param brush contains the drawing attributes. Only the fill parameters of the provided brush are
		used for drawing, including any gradient.

	    \see setFont
	*/
	void drawText(float pos_x, float pos_y, float size, const std::string & text, const Brush & brush);
	
	/** Sets the current orientation that all subsequent draw calls will use for the displayed shapes.

	    SGG maintains a current orientation (default value 0.0f - no rotation) according to which it 
		displays the shape of draw calls. Once set, the orientation is respected by all subsequent
		draw calls, until set to a new value or reset via a call to resetPose(). The rotation of the 
		drawn shape in accordance with the orientation value occurs using a pivot point specific to
		the drawn shape. For rectangles, disks and sections, the pivot point is their center. For 
		text, it is the lower left corner of the text bounds. Line segments are not rotated, since
		it is simpler to redefine their end-points.

		\param angle is the angle of rotation for the shapes to draw in degrees. Positive angles correspond
		to counter-clockwise rotations.
	*/
	void setOrientation(float angle);

	/** Sets the current scale that all subsequent draw calls will use for the displayed shapes.

		SGG maintains a current scale (default value 1.0f - no scaling) according to which it
		displays the shape of draw calls. Once set, the scale is respected by all subsequent
		draw calls, until set to a new value or reset via a call to resetPose(). 

		The scaling is defined by two scaling factors, one for each dimension of the shape. Setting
		an equal value for both parameters corresponds to a uniform scale (no distortion). 
		Scaling factors greater than 1 mean shape magnification, while values less than 1 result in shrinking. 
		Never set the scaling factor to 0.
		
		The change of scale of the
		drawn shape along its X and Y dimension is performed prior to the rotation, using a pivot point specific to
		the drawn shape. For rectangles, disks and sections, the pivot point is their center. For
		text, it is the lower left corner of the text bounds. Line segments are not scaled, since
		it is simpler to redefine their end-points.

		\param sx is the scale factor along the local X axis of the shape. 1 means no scaling.

		\param sy is the scale factor along the local Y axis of the shape. 1 means no scaling.
	*/
	void setScale(float sx, float sy);

	/** Restores both the orientation and scaling to their default values for subsequent draw calls.
	*/
	void resetPose();
	/** @}*/

	/** \defgroup _AUDIO Audio output
	* @{
	*/

	/** Plays a sound sample.

	    The function plays a sound file provided in mp3, ogg or wav format at specified volume level, with the 
		option to indefinately loop it. If the provided sound file is successfully loaded into memory, it is set to play 
		immediately. The sound data are kept in memory, therefore future attempts to play the sample, do not 
		reload the file. If the sound file is not found, no sound is played.

		\param soundfile is the path to the file of the sound sample. WAV, MP3 or OGG file formats are supported.
		
		\param volume is the volume of the played sound sample with values in the range [0,1]

		\param looping sets the sound to indefinately loop. Default value is false;
	*/
	void playSound(std::string soundfile, float volume, bool looping = false);

	/** Starts playing an audio file as a stream.

	    The function plays a sound file provided in mp3, ogg or wav format at specified volume level, with the 
		option to indefinately loop it. The file is loaded in chunks, in order to support large audio files
		and therefore, the function is suitable for reproducing large pieces of music. 
		If the provided sound file is successfully loaded, it is set to play 
		immediately. If the sound file is not found, no sound is played. There is also the option to set a 
		fade in/out time for a smooth transition to a subsequent call to the function using a different sound.
		The same fade effect applies to the beggining of the play cycle. Only one audio file can be
		played as "music" at any one time. A subsequent call to playMusic, simply fades in a new audio stream
		and eventually stops the currently played one.

		\params soundfile is the music file to load in MP3, OGG or WAV format. 

		\param volume is the volume of the played sound sample with values in the range [0,1]

		\param looping sets the sound to indefinately loop. Default value is true;

		\param fade_time is the fade-in/-out time for the audio file in miliseconds. Default value is 0.
		
	*/
	void playMusic(std::string soundfile, float volume, bool looping = true, int fade_time = 0);

	/** Stops any music that is being played.

		This operation only applies to audio files played with the playMusic function. The function 
		also takes as optional argument a fade-out time. 

		\params fade_time is the fade-out time for the currently playing sound.
	*/
	void stopMusic(int fade_time = 0);
	/** @}*/
	
}