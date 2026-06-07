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

private:
    ALCdevice* device;
    ALCcontext* context;
    std::vector<ALuint> sources;
};

#endif
