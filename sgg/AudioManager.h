#pragma once

#include <unordered_map>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

class AudioManager
{
	std::unordered_map<std::string, Mix_Chunk*> sounds;
	std::unordered_map<std::string, Mix_Music*> scores;
	bool initialized = true;
public:
	void playSound(std::string soundfile, float volume, bool looping = false);
	void playMusic(std::string soundfile, float volume, bool looping = true, int fade_time = 0);
	void stopMusic(int fade_time = 0);
	AudioManager();
	~AudioManager();
	
};