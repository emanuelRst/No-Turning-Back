#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool LoadSound(const std::string& filename, ALuint& buffer);
    void PlaySound(ALuint buffer);
    void PlayAmbient(ALuint buffer);
    void StopAmbient();

private:
    ALCdevice* device;
    ALCcontext* context;
    std::vector<ALuint> sources;
    ALuint ambientSource = 0;
};

#endif
