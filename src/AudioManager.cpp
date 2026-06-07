#include "AudioManager.h"
#include <iostream>
#include <vector>

AudioManager::AudioManager() {
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open audio device" << std::endl;
        return;
    }
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    // Pre-generar fuentes para reutilización
    sources.resize(8);
    alGenSources(8, sources.data());
}

AudioManager::~AudioManager() {
    alDeleteSources(sources.size(), sources.data());
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

bool AudioManager::LoadSound(const std::string& filename, ALuint& buffer) {
    SF_INFO sfinfo;
    SNDFILE* sndfile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Failed to open sound file: " << filename << std::endl;
        return false;
    }

    ALenum format;
    if (sfinfo.channels == 1) format = AL_FORMAT_MONO16;
    else if (sfinfo.channels == 2) format = AL_FORMAT_STEREO16;
    else {
        std::cerr << "Unsupported channel count: " << sfinfo.channels << std::endl;
        sf_close(sndfile);
        return false;
    }

    std::vector<short> samples(sfinfo.frames * sfinfo.channels);
    sf_read_short(sndfile, samples.data(), samples.size());
    sf_close(sndfile);

    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, samples.data(), samples.size() * sizeof(short), sfinfo.samplerate);

    return true;
}

void AudioManager::PlaySound(ALuint buffer) {
    // Buscar una fuente que no esté reproduciendo
    ALuint source = 0;
    for (ALuint s : sources) {
        ALint state;
        alGetSourcei(s, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            source = s;
            break;
        }
    }

    if (source != 0) {
        alSourcei(source, AL_BUFFER, buffer);
        alSourcePlay(source);
    } else {
        std::cerr << "No available audio sources" << std::endl;
    }
}
