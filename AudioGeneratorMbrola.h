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

#ifndef _AUDIOGENERATORMBROLA_H
#define _AUDIOGENERATORMBROLA_H

#include <AudioGenerator.h>
#include "esprola.h"

struct mbrola_input_userData {
    const char *text;
    const char *pos;
    };
    
class AudioGeneratorMbrola : public AudioGenerator
{
  public:
    AudioGeneratorMbrola() : bufferPos(0),bufferLen(0), contrast(0), mbrola(NULL),pitch(1.0),tempo(1.0) {};
    ~AudioGeneratorMbrola();
    virtual bool begin(const char *text, AudioOutput *output);
    virtual bool loop() override;
    virtual bool stop() override;
    virtual bool isRunning() override;
    Mbrola *getMbrola();
    void setContrast(int n);
    int getContrast(void) {return contrast;};
    void setPitch(float pitch);
    void setSpeed(float speed);
    float getPitch();
    float getSpeed();
    const char *getVoice(void) {return get_voice_Mbrola();};

  private:
    void applyContrast(void);
    int16_t buffer[1024];
    int bufferPos, bufferLen;
    uint8_t contrast;

  protected:
    Mbrola *mbrola;
    struct mbrola_input_userData udata;
    int readAntiShock(int16_t *buffer, int count);
    bool beginAudioOutput(AudioOutput *output);
    float pitch;
    float tempo;
    int16_t antishock;
    int8_t as_phase;
    
};

#endif

