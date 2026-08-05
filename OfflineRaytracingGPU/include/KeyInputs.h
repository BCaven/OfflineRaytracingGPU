#pragma once

#include <atomic>
#include <map>
#include <string>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL.h>


class KeyInputs
{
private:
	inline static KeyInputs* instance = nullptr;
	KeyInputs();
	~KeyInputs();
	KeyInputs(const KeyInputs&) = delete;
	KeyInputs& operator=(const KeyInputs&) = delete;

	// TODO: constructing this
	std::map<SDL_Keycode, std::atomic<bool>&> keybindings;

	const bool* keyboardState;

public:

	static KeyInputs& inputHandler()
	{
		if (!instance)
		{
			instance = new KeyInputs();
		}
		return *instance;
	}

	static void DeleteInstance()
	{
		delete instance;
		instance = nullptr;
	}

	// in a funny sense, it might be fine to make these not atomic since we only have one writer

	static inline float MOUSE_SENSITIVITY = 1;

	static inline std::atomic<bool> FORWARD = false;
	static inline std::atomic<bool> BACKWARD = false;
	static inline std::atomic<bool> LEFT = false;
	static inline std::atomic<bool> RIGHT = false;
	static inline std::atomic<bool> UP = false;
	static inline std::atomic<bool> DOWN = false;

	static inline std::atomic<float> MOUSE_X = 0.0;
	static inline std::atomic<float> MOUSE_Y = 0.0;
	static inline std::atomic<float> MOUSE_DELTA_X = 0.0;
	static inline std::atomic<float> MOUSE_DELTA_Y = 0.0;

	static inline std::atomic<bool> MOUSE_LEFT_PRESSED = false;
	static inline std::atomic<bool> MOUSE_RIGHT_PRESSED = false;
	static inline std::atomic<float> MOUSE_WHEEL = 0.0;
	static inline std::atomic<bool> WINDOW_RESIZED = false;
	static inline std::atomic<bool> QUIT = false;


	void handleKeyInputs();
	void registerKeybindings(std::string filePath);
	void flushBindings();
};