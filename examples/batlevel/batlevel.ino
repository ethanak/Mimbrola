#include <stdio.h>
#include <ctype.h>
#include <AudioGeneratorMbrola.h>
#include <AudioOutputI2SNoDAC.h>
#include "lang_us.h";

// for internal DAC uncomment line below
// and uncomment definition of USE_AUTOCLICK in config.h

//#define INTERNAL_DAC 1


char buf[1024];
int bufpos;

AudioGeneratorMbrola *ami;
AudioOutputI2S *out;

float myPitch = 1.0, mySpeed = 1.0, myVolume = 1.0;
int myContrast = 0;

int sercmd(const char *buf)
{
    while (*buf && isspace(*buf)) buf++;
    if (*buf++ != '\\') return 0;
    while (*buf && isspace(*buf)) buf++;
    char cmd = *buf++;
    if (!cmd) return 0;
    if (cmd == '?') {
        printf("Pitch:    %.2f\n", myPitch);
        printf("Speed:    %.2f\n", mySpeed);
        printf("Volume:   %.2f\n", myVolume);
        printf("Contrast: %d\n", myContrast);
        return 1;
    }
    cmd = tolower(cmd);
    if (strchr("spvc", cmd)) {
        float f = strtod(buf, NULL);
        if (cmd == 's') {
            mySpeed = f;
            if (ami) ami->setSpeed(mySpeed = f);
        } else if (cmd == 'v') {
            myVolume = f;
            if (ami) ami->setVolume(myVolume);
        } else if (cmd == 'c') {
            myContrast = (int)((f < 1.0) ? (100 * f) : f);
            if (ami) ami->setContrast(myContrast);
        } else {
            myPitch = f;
            if (ami) ami->setPitch(myPitch = f);
        }
        return 1;
    }
    return 0;
}

int getser(void)
{

    while (Serial.available()) {
        int z = Serial.read();
        if (z == 13 || z == 10) {
            if (!bufpos) continue;
            buf[bufpos] = 0;
            bufpos = 0;
            return 1;
        }
        if (bufpos < 1023) buf[bufpos++] = z;
    }
    return 0;
}

void stop(void)
{
    if (ami) {
        ami->stop();
        delete ami;
        delete out;
        ami = NULL;
        out = NULL;
    }
}

void speakloop(void)
{
    if (ami && ami->isRunning()) {
        if (!ami->loop()) {
            stop();
        }
    }
}

void start(void)
{
    stop();
    ami = new AudioGeneratorMbrola();
    ami->setContrast(0.2);
    ami->setPitch(myPitch);
    ami->setSpeed(mySpeed);
    ami->setVolume(myVolume);
#ifdef INTERNAL_DAC
    out = new AudioOutputI2S(0,1);
#else
    out = new AudioOutputI2S();
    // assign pin numbers from your board
    out->SetPinout(12, 14, 13);
#endif
    out->SetGain(1.0);
    ami->setContrast(myContrast);

}

bool saymulti(const char **txt)
{
    start();
    if (!ami->begin(txt, out)) {
        delete ami;
        delete out;
        ami = NULL;
        return false;
    }
    return true;
}

bool say(const char *txt)
{
    start();
    if (!ami->begin(txt, out)) {
        delete ami;
        delete out;
        ami = NULL;
        return false;
    }
    return true;
}

const char *table[4];

void say_battery(float level)
{
    int n, t;
    static const float volt2level[] = {
        3.2, 3.5, 3.72, 3.75,
        3.78, 3.82, 3.88, 3.93,
        4.0, 4.1
    };

    for (n = 0; n < 10; n++) if (level < volt2level[n]) break;
    table[0] = battery;
    table[1] = blevels[n];
    t = 2;
    if (n > 0 && n < 10) table[t++] = percent;
    table[t] = NULL;
    saymulti(table);

}

void setup()
{
    Serial.begin(115200);
    say(iamready);
}

void loop()
{
    speakloop();
    if (getser()) {
        if (!sercmd(buf)) say_battery(strtod(buf, NULL));
    }
}
