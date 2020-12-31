#include <sgg/AudioManager.h>

void AudioManager::playSound(std::string soundfile, float volume, bool looping)
{
	auto siter = sounds.find(soundfile);
	if (siter == sounds.end())
	{
		Mix_Chunk * as = Mix_LoadWAV(soundfile.c_str());
		if (as)
		{
			sounds.insert(std::pair<std::string, Mix_Chunk*>(soundfile, as));
			Mix_VolumeChunk(as, (unsigned char)(MIX_MAX_VOLUME*volume));
			Mix_PlayChannel(-1, as, looping ? -1 : 0);
		}
	}
	else
	{
		Mix_VolumeChunk(siter->second, (unsigned char)(MIX_MAX_VOLUME*volume));
		Mix_PlayChannel(-1, siter->second, looping ? -1 : 0);
	}
}

void AudioManager::playMusic(std::string soundfile, float volume, bool looping, int fade_time)
{
	auto siter = scores.find(soundfile);
	if (siter == scores.end())
	{
		Mix_Music * ms = Mix_LoadMUS(soundfile.c_str());
		if (ms)
		{
			scores.insert(std::pair<std::string, Mix_Music*>(soundfile, ms));
			Mix_VolumeMusic((unsigned char)(MIX_MAX_VOLUME*volume));
			Mix_FadeInMusic(ms, looping ? -1 : 1, fade_time);
		}
	}
	else
	{
		Mix_VolumeMusic((unsigned char)(MIX_MAX_VOLUME*volume));
		Mix_FadeInMusic(siter->second, looping ? -1 : 1, fade_time);
	}
}

void AudioManager::stopMusic(int fade_time)
{
	Mix_FadeOutMusic(fade_time);
	
}

AudioManager::AudioManager()
{
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
		return;
	initialized = true;
}

AudioManager::~AudioManager()
{
	
	for (auto sound : sounds)
		Mix_FreeChunk(sound.second);
	for (auto score : scores)
		Mix_FreeMusic(score.second);

	Mix_CloseAudio();
}
