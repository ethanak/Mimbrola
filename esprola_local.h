/*
 * Mbrola adaptation for microcontrollers
 * Copyright (c) 2021 Bohdan R. Rau
 *
 * Contains parts of original Mbrola code:
 * Copyright (c) 1995-2018 Faculte Polytechnique de Mons (TCTS lab)
 *
 * For further details see https://github.com/numediart/MBROLA 
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

#ifndef _MBROLA_H
#define _MBROLA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
#include "config.h"
#include "common.h"
#include data_header(_header.h)
*/
//#define uint8_t int
#define False 0
#define True 1
#define uint8  uint8_t
#define int8   int8_t
#define int16  int16_t
#define uint16 uint16_t
#define int32  uint32_t

#define MBR_free(X)		{free(X);X=NULL;}
#define MBR_malloc(X)           calloc((X), 1)
#define MBR_strdup(X)           strdup(X)

#define VOICING_MASK 2 /* Voiced/Unvoiced mask  */
#define TRANSIT_MASK 1 /* Stationary/Transitory mask */
#define NV_REG 0          /* unvoiced stable state */
#define NV_TRA TRANSIT_MASK    /* unvoiced transient    */
#define V_REG VOICING_MASK    /* voiced stable state   */
#define V_TRA (VOICING_MASK | TRANSIT_MASK)  /* voiced transient      */

#define COMMENT ";"


/* The name as a string */
typedef char* PhonemeName;

/* Phoneme name encoded according to auxiliary table */
typedef uint16 PhonemeCode;
#define MAX_PHONEME_NUMBER 65000

typedef enum {
        PHO_OK,
        PHO_EOF,
        PHO_FLUSH,
        PHO_ERROR
} StatePhone;


/* from phone.h */

#define MAXPITCHPAT 12
#define MAXPHONES 16

/*
 * STRUCTURES representing phones and diphone sequences to synthesize
 */

/* Pitch pattern point attached to a Phoneme */
typedef struct
{
	float pos;		        /* relative position within phone in milliseconds */
	float freq;		        /* frequency (Hz)*/
} PitchPatternPoint;

#define pos_Pitch(X) X->pos
#define freq_Pitch(X) X->freq

/* A Phoneme and its pitch points */
typedef struct
{
	char name[4];       	              /* Name of the phone       */
	float length;	                 /* phoneme length in ms    */
	int8_t  NPitchPatternPoints;        /* Nbr of pattern points   */
    uint8_t used;
	PitchPatternPoint PitchPattern[MAXPITCHPAT+2];
	/* PitchPattern[0] gives F0 at 0% of the duration of a phone,
	   and the last pattern point (PitchPattern[NPitchPatternPoints-1])
	   gives F0 at 100% ( reserve 2 slots for 0% and 100% during interpolation )	 */
} Phone;

/* Convenient macro to access Phone structure */
#define tail_PitchPattern(X) (&(X->PitchPattern[X->NPitchPatternPoints-1]))
#define head_PitchPattern(X) (&(X->PitchPattern[0]))
#define val_PitchPattern(X,i) (&(X->PitchPattern[i]))
#define length_Phone(X) (X->length)
#define name_Phone(X) (X->name)
#define NPitchPatternPoints(X) (X->NPitchPatternPoints)
#define pp_available(X) (X->pp_available)
#define PitchPattern(X) (X->PitchPattern)

/*
 * number of unused phones in static table
 */
 


/*
 * from diphone_info.h
 */
 
/*
 * Structure of the diphone database (as stored in memory)
 */
typedef struct
{
	/* Name of the diphone     */
	PhonemeCode left;
	PhonemeCode right;	

	int32 pos_wave;	/* position in SPEECH_FILE */
	int16 halfseg;	/* position of center of diphone */
	int32 pos_pm;		/* index in PITCHMARK_FILE */
	uint8 nb_frame;    	/* Number of pitch markers */
} DiphoneInfo;

/* Convenience macros */
#define left(diphoneinfo) (diphoneinfo).left
#define right(diphoneinfo) (diphoneinfo).right
#define pos_wave(diphoneinfo) (diphoneinfo).pos_wave
#define halfseg(diphoneinfo) (diphoneinfo).halfseg
#define pos_pm(diphoneinfo) (diphoneinfo).pos_pm
#define nb_frame(diphoneinfo) (diphoneinfo).nb_frame

/* from diphone.h */

/*
 * PITCH MARKED DIPHONE DATABASE, CONSTANTS
 */
#define NBRE_PM_MAX	2000  /* Max nbr of frames in a synth. segment*/

/*
 * STRUCTURES representing diphone sequences to synthesize
 */

/* 
 * A DiphoneSynthesis is a Diphone equiped with information necessary to 
 * synthesize it
 */
typedef struct
{
	/* A Diphone is made of 2 phonemes */
	Phone *LeftPhone; /* First phoneme  */
	Phone *RightPhone;/* Second phoneme */

	int   Length1; /* Length of first half-phoneme in samples */
	int   Length2; /* Length of second half-phoneme in samples */

	DiphoneInfo* Descriptor;	 /* Descriptor in the diphone database      */
	const uint8 *p_pmrk;		/* Point to the beginning of the pm 1..N interval */
	uint8 p_pmrk_offset;  /* offset in the 4 bit compressed structure */

	int16 smoothw[2* MBRPeriod];    /* Difference vector between 2 ola frames (2 mbr_period) */
	uint8_t smooth;		   /* True if Smoothw has a value */

	int16 buffer[max_samples];     /* To read or uncompress audio data */
   
	uint8 real_frame[max_frame]; /*  for skiping V - NV transition */
	uint8 tot_frame;   /* physical number of frames of the diphone */
	int nb_pm;			/* Number of pitch markers to synthesize */
} DiphoneSynthesis;

/*
 * Convenient macros to access Diphone_synth structures
 */
#define halfseg_diphone(X) X->Descriptor->halfseg
#define nb_frame_diphone(X) X->Descriptor->nb_frame
#define pos_wave_diphone(X) X->Descriptor->pos_wave

#define left_diphone(MB,X)  auxiliary_tab_val( diphone_table(diph_dba(MB)) , X->Descriptor->left)
#define right_diphone(MB,X) auxiliary_tab_val( diphone_table(diph_dba(MB)) , X->Descriptor->right)

#define Descriptor(X) (X->Descriptor)
#define Length1(X) X->Length1
#define Length2(X) X->Length2
#define LeftPhone(X) (X->LeftPhone)
#define RightPhone(X) (X->RightPhone)

#define nb_pm(X) X->nb_pm
#define smoothw(X) X->smoothw
#define smooth(X) X->smooth
#define buffer(X) X->buffer
#define buffer_alloced(X) X->buffer_alloced
#define real_frame(X) X->real_frame
#define physical_frame_type(X) X->physical_frame_type
#define tot_frame(X) X->tot_frame

/* At the moment it's a macro */
#define pmrk_DiphoneSynthesis(DP,INDEX) ((DP->p_pmrk[ ( (INDEX-1)+DP->p_pmrk_offset)/ 4 ] >> (  2*( ((INDEX-1) + DP->p_pmrk_offset)%4))) & 0x3)


void reset_DiphoneSynthesis(DiphoneSynthesis* ds);
/*
 * Forget the diphone in progress
 */

void close_DiphoneSynthesis(DiphoneSynthesis* ds);
/* Release memory and phone */

int GetPitchPeriod(DiphoneSynthesis *dp, int cur_sample,int Freq);
/*
 * Returns the pitch period (in samples) at position cur_sample 
 * of dp by linear interpolation between pitch pattern points.
 */



/* from phonbuff.h */

#define MAXNPHONESINONESHOT 16    /* Max nbr of phonemes without F0 pattern*/

/* A phonetic command buffer and its pitch points */
typedef struct
{
	Phone* Buff[MAXNPHONESINONESHOT];/* Phonetic command buffer  */
	int NPhones;     /* Nbr of phones in the phonetic command buffer  */
	int CurPhone;   /* Index of current phone in the command buffer  */
	StatePhone state_pho;   /* State of the last phoneme serie: EOF FLUSH OK */
	uint8_t Closed;	   /* True if the sequence is closed by a pitch point */
    uint8_t eof;
	float default_pitch; /* first pitch point of the sequence */
	float TimeRatio;  /* Ratio for the durations of the phones */
	float FreqRatio;  /* Ratio for the pitch applied to the phones */
    int (* grabLine) (char *, int, void *);
    void *inputUserData;
} PhoneBuff;

/* Convenient macro to access Phonetable */
#define input(X) (X->input)
#define CurPho(X) (X->Buff[X->CurPhone])
#define NPhones(X) X->NPhones
#define CurPhone(X) X->CurPhone
#define Buff(X) X->Buff
#define val_PhoneBuff(pt,i) (pt->Buff[i])
#define state_pho(pt) (pt->state_pho)
#define Closed(pt) (pt->Closed)
#define default_phon(pt) sil_phone
#define default_pitch(pt) (pt->default_pitch)

#define TimeRatio(pt) (pt->TimeRatio)
#define FreqRatio(pt) (pt->FreqRatio)

/* 
 * Last phone of the list
 */
#define tail_PhoneBuff(pt) (val_PhoneBuff(pt,NPhones(pt)))

#define tail_PhoneBuffMB(mb) (mb->phoneBuff.Buff[mb->phoneBuff.NPhones])
#define head_PhoneBuffMB(mb) (mb->phoneBuff.Buff[0])

/* 
 * First phone of the list
 */
#define head_PhoneBuff(pt) (val_PhoneBuff(pt,0))

/*
 * Reads a phone from the phonetic command buffer and prepares the next
 * diphone to synthesize ( prev_diph )
 * Return value may be: PHO_EOF,PHO_FLUSH,PHO_OK, PHO_ERROR
 *
 * NB : Uses only phones from 1 to ... NPhones-1 in the buffer.
 * Phone 0 = memory from previous buffer.
 */


/*
 * from original mbrola.h
 */

typedef struct 
{
	/*
	 * prev_diph points to the previous diphone synthesis structure
	 * and cur_diph points to the current one. The reason is that to
	 * synthesize the previous diphone we need information on the next
	 * one. While progressing to the next diphone, prev_diph memory is
	 * resetted the pointers are swapped between cur and prev diphones
	 */
	DiphoneSynthesis *prev_diph, *cur_diph;

	/* Last_time_crumb balances slow time drifting in match_proso. time_crumb is
	 * the difference in samples between the length really synthesized and 
	 * theoretical one 
	 */
	int last_time_crumb;	
  
	float FirstPitch;     /* default first F0 Value (fetched in the database) */
	int32 audio_length;  /* File size, used for file formats other than RAW */

	int frame_number[NBRE_PM_MAX]; /* for match_prosody */
	int frame_pos[NBRE_PM_MAX];    /* frame position for match_prosody */

	int nb_begin;
	int nb_end; /* number of voiced frames at the begin and end the segment */

	uint8_t saturation;    /* Saturation in ola_integer */
	float ola_win[2*MBRPeriod];     /* OLA buffer                  */
	int16 ola_integer[2*MBRPeriod]; /* OLA buffer for file output  */

	float weight[2*MBRPeriod];      /* Hanning weighting window */
	float volume_ratio; 	       /* 1.0 is default */
  
	/* 
	 * The following variables are part of the structure for library mode
	 * but could be local for standalone mode
	 */
	uint8_t odd; /* flip-flop for reversing 1 out of 2 unvoiced OLA frame */
	int frame_counter;    /* frame being OLAdded  */
	int buffer_shift;		/* Shift between 2 Ola = available for output */
	int zero_padding;	   /* 0's between 2 Ola = available for output */
  
	uint8_t smoothing;	      /* True if the smoothing algorithm is on */
	uint8_t no_error;        /* True to ignore missing diphones */

	uint16 VoiceFreq;		   /* Freq of the audio output (vocal tract length) */
	float  VoiceRatio;    /* Freq ratio of the audio output */

	uint8_t first_call;	/* True if it's the first call to Read_MBR */
	int eaten;	     /* Samples allready consumed in ola_integer */

    DiphoneSynthesis dbs[2];
    PhoneBuff phoneBuff;
    Phone _localPhoneTable[MAXPHONES];
    uint32_t flash_address;
    int (* output_function)(int16_t *samples, int samplesNum);

} Mbrola;

/* Convenience macros */
#define diph_dba(mb)  mb->diph_dba
#define parser(mb)  mb->parser
#define prev_diph(mb)  mb->prev_diph
#define cur_diph(mb)  mb->cur_diph
#define last_time_crumb(mb)  mb->last_time_crumb
#define FirstPitch(mb)  mb->FirstPitch
#define audio_length(mb)  mb->audio_length
#define frame_number(mb)  mb->frame_number
#define frame_pos(mb)  mb->frame_pos
#define nb_begin(mb)  mb->nb_begin
#define nb_end(mb)  mb->nb_end
#define saturation(mb)  mb->saturation
#define ola_win(mb)  mb->ola_win
#define ola_integer(mb)  mb->ola_integer
#define weight(mb)  mb->weight
#define volume_ratio(mb)  mb->volume_ratio
#define odd(mb)  mb->odd
#define frame_counter(mb)  mb->frame_counter
#define buffer_shift(mb)  mb->buffer_shift
#define zero_padding(mb)  mb->zero_padding
#define smoothing(mb)  mb->smoothing
#define no_error(mb)  mb->no_error
#define VoiceRatio(pt) (pt->VoiceRatio)
#define VoiceFreq(pt) (pt->VoiceFreq)
#define first_call(pt) (pt->first_call)
#define eaten(pt) (pt->eaten)

extern	DiphoneSynthesis* init_DiphoneSynthesis(Mbrola *mb,int pb);
/* Alloc memory, working and audio buffers for synthesis */

void set_voicefreq_Mbrola(Mbrola* mb, uint16 OutFreq);
/* Change the Output Freq and VoiceRatio to change the vocal tract   */

uint16_t get_voicefreq_Mbrola(Mbrola* mb);
/* Get output Frequency */

void set_smoothing_Mbrola(Mbrola* mb, uint8_t smoothing);
/* Spectral smoothing or not */

uint8_t get_smoothing_Mbrola(Mbrola* mb);
/* Spectral smoothing or not */

void set_no_error_Mbrola(Mbrola* mb, uint8_t no_error);
/* Tolerance to missing diphones */

uint8_t get_no_error_Mbrola(Mbrola* mb);
/* Spectral smoothing or not */

void set_volume_ratio_Mbrola(Mbrola* mb, float volume_ratio);
/* Overall volume */

float get_volume_ratio_Mbrola(Mbrola* mb);
/* Overall volume */

#if 0
void set_parser_Mbrola(Mbrola* mb, Parser* parser);
/* drop the current parser for a new one */
#endif

Mbrola* init_Mbrola(int (* grabLine)(char *, int, void *), int (output_function)(int16_t *, int ));
/* 
 * Connect the database to the synthesis engine, then initialize internal 
 * variables. Connect the phonemic command stream later with set_parser_Mbrola
 */

void close_Mbrola(Mbrola* mb);
/* close related features and free the memory ! */

uint8_t reset_Mbrola(Mbrola* mb);
/* 
 * Gives initial values to current_diphone (not synthesized anyway)
 * -> it will give a first value for prev_diph when we make the first
 *    NextDiphone call so that cur_diph= _-FirstPhon with lenght1=0
 *    and prev_diph= _-_ with length2=0
 *
 * return False in case of error
 */

StatePhone NextDiphone(Mbrola* mb);
/*
 * Reads a phone from the phonetic command buffer and prepares the next
 * diphone to synthesize ( prev_diph )
 * Return value may be: PHO_EOF, PHO_FLUSH, PHO_OK, PHO_ERROR
 */

uint8_t MatchProsody(Mbrola* mb);
/*
 * Selects Duplication or Elimination for each analysis OLA frames of
 * the diphone we must synthesize (prev_diph). Selected frames must fit
 * with wanted pitch pattern and phonemes duration of prev_diph
 *
 * Return False in case of error
 */

void Concat(Mbrola* mb);
/*
 * This is a unique feature of MBROLA.
 * Smoothes diphones around their concatenation point by making the left
 * part fade into the right one and conversely. This is possible because
 * MBROLA frames have the same length everywhere plus phase trick.
 *
 * output : nb_begin, nb_end -> number of stable voiced frames to be used
 * for interpolation at the end of Leftphone(prev_diph) and the beginning
 * of RightPhone(prev_diph).
 */

void OverLapAdd(Mbrola* mb, int frame);
/*
 *  OLA routine
 */


/* LIBRARY mode: synthesis driven by the output */

int read_Mbrola(Mbrola* mb, int16_t *buffer_out, int nb_wanted);
/*
 * Reads nb_wanted samples in an audio buffer
 * Returns the effective number of samples read
 */
StatePhone Synthesis(Mbrola* mb);
uint8_t getdiphone_Database(Mbrola *mb, DiphoneSynthesis *diph);
Phone* init_Phone(Mbrola *mb,const char *name, float length);
void reset_Phone(Phone *ph);
void close_Phone(Phone *ph);
void appendf0_Phone(Phone *ph, float pos, float f0);
void applyRatio_Phone(Phone* ph, float ratio);
int freePhones(Mbrola *mb);

void init_PhoneBuff(Mbrola *mb, float default_pitch, float time_ratio, float freq_ratio, int (* grabLine)(char *, int, void*));
void reset_PhoneBuff(Mbrola *mb);
/* Before a synthesis sequence initialize the loop with a silence */

StatePhone next_PhoneBuff(Mbrola *mb,Phone** ph);

void flashInit(void);

static void fatal_message() {};
static void error_message() {};
static void warning_message() {};
static void debug_message() {};

#define debug_message1 debug_message
#define debug_message2 debug_message
#define debug_message3 debug_message
#define debug_message4 debug_message
#define debug_message5 debug_message


#define ERROR_MEMORYOUT					-1
#define ERROR_UNKNOWNCOMMAND			-2
#define ERROR_SYNTAXERROR				-3
#define ERROR_COMMANDLINE          -4
#define ERROR_OUTFILE              -5
#define ERROR_RENAMING             -6
#define ERROR_NEXTDIPHONE          -7
 
#define ERROR_PRGWRONGVERSION		-10
 
#define ERROR_TOOMANYPITCH			-20
#define ERROR_TOOMANYPHOWOPITCH		-21
#define ERROR_PITCHTOOHIGH			-22
 
#define ERROR_PHOLENGTH				-30
#define ERROR_PHOREADING				-31
 
#define ERROR_DBNOTFOUND				-40
#define ERROR_DBWRONGVERSION			-41
#define ERROR_DBWRONGARCHITECTURE	-42
#define ERROR_DBNOSILENCE				-43
#define ERROR_INFOSTRING           -44
 
#define ERROR_BINNUMBERFORMAT			-60
#define ERROR_PERIODTOOLONG				-61
#define ERROR_SMOOTHING					-62
#define ERROR_UNKNOWNSEGMENT			-63
#define ERROR_CANTDUPLICATESEGMENT		-64
#define ERROR_TOOMANYPHONEMES                   -65

#define ERROR_BOOK		-70
#define ERROR_CODE		-71

#define WARNING_UPGRADE -80
#define WARNING_SATURATION -81

static int lasterr_code=-1;
#endif
