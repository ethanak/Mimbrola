#ifndef ESPROLA_H
#define ESPROLA_H 1

#include <stdint.h>
#ifndef _MBROLA_H
typedef struct {
} Mbrola;
#endif
#ifdef __cplusplus
extern "C" {
#endif

extern const char *get_voice_Mbrola(void);

Mbrola *init_Mbrola(int (*grabLine)(char *, int, void *),
                    int(*output_function)(int16_t *, int));
void close_Mbrola(Mbrola * mb);
int read_Mbrola(Mbrola * mb, int16_t * buffer_out, int nb_wanted);
void set_input_userData_Mbrola(Mbrola * mb, void *userData);
uint8_t reset_Mbrola(Mbrola * mb);

void set_voicefreq_Mbrola(Mbrola * mb, uint16_t OutFreq);
uint16_t get_voicefreq_Mbrola(Mbrola * mb);

void set_smoothing_Mbrola(Mbrola * mb, uint8_t smoothing);
uint8_t get_smoothing_Mbrola(Mbrola * mb);

void set_no_error_Mbrola(Mbrola * mb, uint8_t no_error);
uint8_t get_no_error_Mbrola(Mbrola * mb);

void set_volume_ratio_Mbrola(Mbrola * mb, float volume_ratio);
float get_volume_ratio_Mbrola(Mbrola * mb);

void set_time_ratio_Mbrola(Mbrola * mb, float ratio);
void set_freq_ratio_Mbrola(Mbrola * mb, float ratio);
float get_time_ratio_Mbrola(Mbrola * mb);
float get_freq_ratio_Mbrola(Mbrola * mb);
#ifdef __cplusplus
}
#endif
#endif
