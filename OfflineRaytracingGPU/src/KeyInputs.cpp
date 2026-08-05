#include "KeyInputs.h"

void KeyInputs::handleKeyInputs()
{
	flushBindings();
	float x, y;
	auto mouse_buttons = SDL_GetMouseState(&x, &y);
	MOUSE_X = x;
	MOUSE_Y = y;

	float dx = 0, dy = 0;
	auto mouse_delta_buttons = SDL_GetRelativeMouseState(&dx, &dy);
	MOUSE_DELTA_X = dx;
	MOUSE_DELTA_Y = dy;


	// TODO: have a better way to do this
	FORWARD = keyboardState[SDL_SCANCODE_W];
	BACKWARD = keyboardState[SDL_SCANCODE_S];
	LEFT = keyboardState[SDL_SCANCODE_A];
	RIGHT = keyboardState[SDL_SCANCODE_D];
	UP = keyboardState[SDL_SCANCODE_Q];
	DOWN = keyboardState[SDL_SCANCODE_E];

	QUIT = keyboardState[SDL_SCANCODE_ESCAPE];

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		switch (e.type)
		{
		case SDL_EVENT_MOUSE_WHEEL:
			MOUSE_WHEEL = (float)e.wheel.y;
			break;
		case SDL_EVENT_QUIT:
			QUIT = true;
			break;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			QUIT = true;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			WINDOW_RESIZED = true;
			break;
		}
	}
}

void KeyInputs::registerKeybindings(std::string filePath)
{
}

void KeyInputs::flushBindings()
{
	FORWARD = false;
	BACKWARD = false;
	LEFT = false;
	RIGHT = false;
	MOUSE_LEFT_PRESSED = false;
	MOUSE_RIGHT_PRESSED = false;
	WINDOW_RESIZED = false;
	QUIT = false;
	MOUSE_WHEEL = 0;
}

KeyInputs::KeyInputs()
{
	FORWARD = false;
	BACKWARD = false;
	LEFT = false;
	RIGHT = false;
	QUIT = false;
	MOUSE_LEFT_PRESSED = false;
	MOUSE_X = 0;
	MOUSE_Y = 0;
	keyboardState = SDL_GetKeyboardState(nullptr);
}



KeyInputs::~KeyInputs()
{

}