/*
 * Mbrola adaptation for microcontrollers
 * Copyright (c) 2021 Bohdan R. Rau
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "config.h"
#include "esprola.h"
#include "AudioGeneratorMbrola.h"

int zero;

extern "C" {
    static int grabLine(char *, int, void *);
};                              // semicolon for indent

static int grabLine(char *buf, int size, void *userData)
{
    struct mbrola_input_userData *ud = (struct mbrola_input_userData *)userData;
    int i;
    if (!*ud->pos)
        return 0;
    for (i = 0; *ud->pos && *ud->pos != '\n'; i++, ud->pos++) {
        if (i < size - 1)
            *buf++ = *ud->pos;
    }
    if (*ud->pos == '\n')
        ud->pos++;
    *buf = 0;
    return 1;
}

AudioGeneratorMbrola::~AudioGeneratorMbrola()
{
    if (mbrola)
        close_Mbrola(mbrola);
}

Mbrola *AudioGeneratorMbrola::getMbrola()
{
    return mbrola;
}

void AudioGeneratorMbrola::applyContrast(void)
{
    float d, contrast_level = contrast / 100.0;
    int i;
    for (i = 0; i < bufferLen; i++) {
        d = M_PI_2 * buffer[i] / 32768.0;
        buffer[i] = sin(d + contrast_level * sin(d * 4)) * 32767;
    }
}

void AudioGeneratorMbrola::setContrast(int n)
{
    contrast = (n < 0) ? 0 : (n > 100) ? 100 : n;
}

int AudioGeneratorMbrola::readAntiShock(int16_t *buffer, int count)
{
    int samples=0;
        
    if (as_phase == 1) {
        return 0;
        while (count-- > 0 && antishock <= -10) {
            *buffer++ = (antishock < -21800) ? (3*antishock + 32760) : 0;;
            antishock += 10;
            samples++;
        }
        return samples;
    }

    if (as_phase == 2) {
        while (count-- > 0 && antishock >=-32700) {
            *buffer++ = (antishock > -10900) ? (3 * antishock) : -32700;
            antishock -= 20;
            samples++;
        }
        return samples;
    }
    return 0;
}
    
bool AudioGeneratorMbrola::loop(void)
{
    while (running) {
        if (bufferPos >= bufferLen) {
            int n=0;
#ifdef USE_ANTISHOCK

            if (as_phase == 1) { // attack
                n=readAntiShock(buffer, 1024);
                if (!n) as_phase = 0;
            }
            if (as_phase == 0) {
                n = read_Mbrola(mbrola, buffer, 1024);
                if (n<=0) {
                    as_phase = 2;
                    antishock=0;
                }
            }
            if (as_phase == 2) {
                n=readAntiShock(buffer, 1024);
            }
#else
            n = read_Mbrola(mbrola, buffer, 1024);
#endif
            if (n <= 0) {
                stop();
                break;
            }
            bufferPos = 0;
            bufferLen = n;
            if (contrast)
                applyContrast();
        }
        lastSample[AudioOutput::LEFTCHANNEL] =
            lastSample[AudioOutput::RIGHTCHANNEL] = buffer[bufferPos];
        if (!output->ConsumeSample(lastSample))
            break;              // Can't send, but no error detected
        bufferPos++;
    }
    output->loop();
    return running;
}

bool AudioGeneratorMbrola::beginAudioOutput(AudioOutput * output)
{
    as_phase = 1;
    antishock = -32700;
    if (!output->SetRate(get_voicefreq_Mbrola(mbrola))) {
        printf("AudioGeneratorMbrola::begin: failed to SetRate %d in output\n",
               get_voicefreq_Mbrola(mbrola));
        return false;
    }
    if (!output->SetBitsPerSample(16)) {
        printf
            ("AudioGeneratorMbrola::begin: failed to SetBitsPerSample in output\n");
        return false;
    }
    if (!output->SetChannels(1)) {
        printf
            ("AudioGeneratorMbrola::begin: failed to SetChannels in output\n");
        return false;
    }
    if (!output->begin()) {
        printf
            ("AudioGeneratorMbrola::begin: output's begin did not return true\n");
        return false;
    }
    this->output = output;
    running = true;
    return true;

}

bool AudioGeneratorMbrola::begin(const char *text, AudioOutput * output)
{
    if (!mbrola) {
        mbrola = init_Mbrola(grabLine, NULL);
        set_freq_ratio_Mbrola(mbrola, pitch);
        set_time_ratio_Mbrola(mbrola, tempo);
        
        
    }
    udata.text = text;
    udata.pos = text;
    set_input_userData_Mbrola(mbrola, (void *)&udata);
    return beginAudioOutput(output);
}

bool AudioGeneratorMbrola::stop()
{
    if (!running)
        return true;
    running = false;
    output->stop();
    if (mbrola)
        close_Mbrola(mbrola);
    mbrola = NULL;
    return true;
}

bool AudioGeneratorMbrola::isRunning()
{
    return running;
}

void AudioGeneratorMbrola::setSpeed(float speed)
{
    if (speed < 0.5) speed = 0.5; else if (speed > 2.0) speed=2.0;
    tempo = 1.0/speed;
    if (mbrola) set_time_ratio_Mbrola(mbrola, tempo);
}

float AudioGeneratorMbrola::getSpeed(void)
{
    if (mbrola) return 1.0 / get_time_ratio_Mbrola(mbrola);
    return 1.0 / tempo;
}

void AudioGeneratorMbrola::setPitch(float _pitch)
{
    if (_pitch < 0.5) pitch = 0.5; else if (_pitch > 2.0) pitch=2.0; else pitch=_pitch;
    if (mbrola) set_freq_ratio_Mbrola(mbrola, pitch);
}

float AudioGeneratorMbrola::getPitch(void)
{
    if (mbrola) return get_freq_ratio_Mbrola(mbrola);
    return pitch;
}

    
