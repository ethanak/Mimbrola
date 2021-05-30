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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include "config.h"
#include data_header(_header.h)
#include "esprola_local.h"
#include data_header(_data.h)
#include "esprola.h"


#ifndef HAVE_WAVEBLOB    
#include <esp_partition.h>
#endif

/* phone.h */

//static Phone _localPhoneTable[MAXPHONES];

const char *get_voice_Mbrola(void)
{
    return BLOB_ID+1;
}
void init_PhoneTable(Mbrola *mb)
{
        memset(mb->_localPhoneTable, 0, sizeof(Phone) * MAXPHONES);
}

void free_PhoneTable(Mbrola *mb)
{
}

Phone * init_Phone(Mbrola *mb,const char *name, float length)
{
    Phone* self;
    int i;
    for (i=0;i < 32; i++) if (!mb->_localPhoneTable[i].used) break;
    if (i >= 32) return NULL;
    self = &mb->_localPhoneTable[i];
    self->used = 1;
	strcpy(name_Phone(self),name);
	length_Phone(self)=length;
	reset_Phone(self);
    return self;
}

int freePhones(Mbrola *mb)
{
    int i,cnt;
    for (i = cnt = 0; i<MAXPHONES; i++) if (!mb->_localPhoneTable[i].used) cnt++;
    return cnt;
}

void  reset_Phone(Phone *ph)
/* Reset the pitch pattern list of a phoneme */
{
	ph->NPitchPatternPoints=0;
}

void  close_Phone(Phone *ph)
{
    ph->used = 0;
}

void appendf0_Phone(Phone *ph, float pos, float f0)
/*
 * Append a pitch point to a phoneme ( position in % and f0 in Hertz )
 * pitch pattern replaces last if there is no place in table
 */
{
	int index_f0= NPitchPatternPoints(ph);
    if (index_f0 >= MAXPHONES) index_f0--; // lost previous pitch ?
	pos_Pitch( val_PitchPattern(ph,index_f0))= pos/100.0f * length_Phone(ph);
	freq_Pitch( val_PitchPattern(ph,index_f0))= f0;
    NPitchPatternPoints(ph) = index_f0+1;
}

void applyRatio_Phone(Phone* ph, float ratio)
/* length and freq modified if the vocal tract length is not 1.0 */
{
	int i;
	/* need to change anything ? */
	if (ratio==1.0)
		return;

	length_Phone(ph)*= ratio;
	for(i=0; i< ph->NPitchPatternPoints; i++)
    {
		ph->PitchPattern[i].pos *= ratio;
		ph->PitchPattern[i].freq /= ratio;
    }
}

/* phonbuff.c */

#include <errno.h>
#include <ctype.h>

void initdummy_PhoneBuff(Mbrola *mb)
{
	Phone* my_phone;
    PhoneBuff* pt = &mb->phoneBuff;
	/* silence with dummy length, and  1 pitch point at 0% equal 
	 * to FirstPitch for interpolation
	 */
	my_phone=init_Phone(mb, default_phon(pt), 0.0f);
	appendf0_Phone(my_phone, 0.0f, default_pitch(pt));
	pt->Buff[0]=my_phone;

	CurPhone(pt)=0;    /* Forces FillCommandBuffer to read new data and  */
	NPhones(pt)=0;      /* consider PhoneBuff[0] as the previous phone   */
	state_pho(pt)=PHO_OK;  /* Reset ReadPho internal flag */
	Closed(pt)=False;
    pt->eof = 0;
    
}
void free_residue_PhoneBuff(Mbrola *mb)
/* mark phones as unused */
{
	int i;
  
	/* In charge of freeing what have not been passed to the synthesizer */
	for(i=mb->phoneBuff.CurPhone; i<mb->phoneBuff.NPhones+1; i++)
		close_Phone(mb->phoneBuff.Buff[i]);	
}

void reset_PhoneBuff(Mbrola *mb)
/* Before a synthesis sequence initialize the loop with a silence */
{
	/* Remove the reliquate */
	free_residue_PhoneBuff(mb);
	initdummy_PhoneBuff(mb);
	//input(pt)->reset_Input(input(pt));
}

void init_PhoneBuff(Mbrola *mb, float default_pitch, float time_ratio, float freq_ratio, int (* grabLine)(char *, int, void*))
/*
 * Phonetab Constructor
 */
{
    PhoneBuff *self;
    init_PhoneTable(mb);
    mb->phoneBuff.default_pitch=default_pitch;
	mb->phoneBuff.TimeRatio=time_ratio;
	mb->phoneBuff.FreqRatio=freq_ratio;
    mb->phoneBuff.grabLine = grabLine;
	initdummy_PhoneBuff(mb);
}

void shift_PhoneBuff(PhoneBuff *pt)
/* 
 * Reset the phonetable to an empty value
 */
{
	/* PhoneBuff[0] filled with last phone of previous fill */
	head_PhoneBuff(pt)=tail_PhoneBuff(pt);
  
	/* The first pitch point will have index 1  */
	/* Leaves the first position free for 0% pitch point */
	CurPhone(pt)=0;
	NPhones(pt)=0;
	Closed(pt)=False;
}

void append_PhoneBuff(Mbrola *mb,const char *name,float length)
/*
 * Append a new phone at the end of the table
 * if too many phones without pitch information, add a pitch point with
 * default pitch
 */
{
	Phone *my_phone;
  
	mb->phoneBuff.NPhones++;
	my_phone= init_Phone(mb,name,length);
	tail_PhoneBuffMB(mb)= my_phone;

	/* Dummy point for later 0% value */
	appendf0_Phone(tail_PhoneBuffMB(mb), 0.0f, 0.0f);

	/* Test overflow ... I consider this is not fatal any more, just add a compulsory pitch point */
	if (mb->phoneBuff.NPhones==MAXNPHONESINONESHOT-2)
    {
		
		/* Energic measures to force a pitch point */
		appendf0_Phone(my_phone, 0.0, mb->phoneBuff.default_pitch);
    }
}

void interpolatef0_PhoneBuff(Mbrola *mb)
/*
 * Interpolate 0% and 100% value for each phone of the table
 */
{
	int i;
	float CurPos;
	float a, b;            /* Interpolation parameters */
	float InterpLength=0.0f;
  
	/* Sum the lengths without the borders */
	for(i=1; i<mb->phoneBuff.NPhones; i++)
		InterpLength+= length_Phone( mb->phoneBuff.Buff[i]);
  
	CurPos= length_Phone(head_PhoneBuffMB(mb)) - 
		pos_Pitch( tail_PitchPattern(head_PhoneBuffMB(mb)));
  
	/* Add the right and the left border for the interpolation */
	InterpLength+= pos_Pitch(val_PitchPattern(tail_PhoneBuffMB(mb),1)) + CurPos;
  
	/* From broad to narrow stylization : 
	 *     linear interpolation frequency=ax+b 
	 */
	if (InterpLength!=0)
    {
		/* Last pitch point of the first phoneme */
		b= freq_Pitch(tail_PitchPattern(head_PhoneBuffMB(mb)));

		/* First pitch point of the last phoneme */
		a= ( freq_Pitch(val_PitchPattern(tail_PhoneBuffMB(mb),1)) - b) / InterpLength ;

		/* Interpolate the core */
		for (i=1; i<mb->phoneBuff.NPhones; i++)
		{
			/* reset the number of points */
			reset_Phone(mb->phoneBuff.Buff[i]);
			 
			/* Put pitch point at 0% */
			appendf0_Phone(mb->phoneBuff.Buff[i], 0.0f, a*CurPos+b);
			 
			/* Put a pitch point at 100% */
			CurPos+= length_Phone(mb->phoneBuff.Buff[i]);
			appendf0_Phone(mb->phoneBuff.Buff[i], 100.0f, a*CurPos+b);
		}
      
		/* Add 0% pitch on the last phoneme (minimal phone has 2 pitch points) */
		mb->phoneBuff.Buff[mb->phoneBuff.NPhones]->PitchPattern[0].pos=  0.0f;
		mb->phoneBuff.Buff[mb->phoneBuff.NPhones]->PitchPattern[0].freq= a*CurPos + b;
      
		/* Add 100% pitch point to the 1st phoneme */
		appendf0_Phone(head_PhoneBuffMB(mb),
					   100.0,
					   freq_Pitch(head_PitchPattern(mb->phoneBuff.Buff[1])));
    }
  
	/* Interpolated, until next sequence ! */
	mb->phoneBuff.Closed=True;
}

#define UNSPACE(c) while ((int)*(c) && isspace((int)*(c))) c++
#define TOSPACE(c) while ((int)*(c) && !isspace((int)*(c))) c++



static StatePhone ReadLine(PhoneBuff *pt,char *buf, char **line, int size)
{
    char *c,cmd;float f;
    for (;;) {
        if (!pt->grabLine(buf, size,pt->inputUserData)) return PHO_EOF;
        //printf("Grab %s\n",buf);
        c=buf;
        UNSPACE(c);
        if (!*c) continue;
        if (*c == '#') return PHO_FLUSH;
        if (*c !=';') break;
        if (c[1] != ';') continue;
        c+=2;
        if (*c != 'T' && *c != 'F') continue;
        cmd=*c++;
        UNSPACE(c);
        if (*c++ != '=') continue;
        UNSPACE(c);
        f=strtof(c, &c);
        if (f < 0.5 || f > 2.0) continue;
        if (cmd == 'T') pt->TimeRatio = f;
        else pt->FreqRatio = f;
    }
    *line=c;
    return PHO_OK;
}

StatePhone FillCommandBuffer(Mbrola *mb)
{
    char linebuf[256],*line;
    StatePhone state_line;	 /* Return value */
    char *name;
    float length;
    float pos, val;
    if (mb->phoneBuff.eof) return PHO_EOF;
    errno=0;
    
    do {
        state_line = ReadLine(&mb->phoneBuff, linebuf,&line,256);
        
        if (state_line == PHO_FLUSH || state_line == PHO_EOF) {
            /* If we have to flush, add 3 trailing silences */
            /* the 1st one will help reveal the last phoneme ! */
            append_PhoneBuff(mb, sil_phone,0);

            /* The 2nd will be issued */
            append_PhoneBuff(mb, sil_phone,0);

            /* 
             * The 3rd one won't even issued with nextphone_Parser, but
             * compulsory to provide F0 interpolation value
             */
            append_PhoneBuff(mb, sil_phone,0);
            appendf0_Phone(tail_PhoneBuffMB(mb),0.0f,mb->phoneBuff.default_pitch*mb->phoneBuff.FreqRatio);
            if (state_line == PHO_EOF) mb->phoneBuff.eof=1;
            break;
        }
        else if (state_line==PHO_EOF) return PHO_EOF;

        /* PHO_OK */
        name=line;TOSPACE(line);
        if (*line) *line++=0;
        if (strlen(name)>3) return PHO_ERROR;
        length = strtof(line, &line); if (errno || length < 0) return PHO_ERROR;

        if (!strcmp(name, sil_phone)) {
            /* it's a silence add an anti spreading 0ms silence */
			append_PhoneBuff(mb, sil_phone,0); 
		}
		
		/* A New phoneme */
		append_PhoneBuff(mb, name, length*mb->phoneBuff.TimeRatio);

        /*
		 * Read pairs of POSITION PITCH_VALUE till the end of a_line
		 * Eventually the pairs can be surrounded with ()
         */
		

        for (;;) {
            UNSPACE(line);
            if (!*line) break;
            if (*line == '(' ) line++;
            pos=strtod(line, &line); if (errno) return PHO_ERROR;
            val=strtod(line, &line); if (errno) return PHO_ERROR;
            UNSPACE(line); if (*line == ')' ) line++;
            appendf0_Phone(tail_PhoneBuffMB(mb),pos,val *mb->phoneBuff.FreqRatio);
        }
    } while ( NPitchPatternPoints(tail_PhoneBuffMB(mb)) == 1 );
    interpolatef0_PhoneBuff(mb);
    return PHO_OK;
}

StatePhone next_PhoneBuff(Mbrola *mb, Phone** ph)
/*
 * Reads a phone from the phonetic command buffer and prepares the next
 * diphone to synthesize ( prev_diph )
 * Return value may be: PHO_EOF,PHO_FLUSH,PHO_OK
 *
 * NB : Uses only phones from 1 to ... NPhones-1 in the buffer.
 * Phone 0 = memory from previous buffer.
 */
{

	if (mb->phoneBuff.CurPhone==mb->phoneBuff.NPhones
		|| !mb->phoneBuff.Closed)
    { /* Must read buffer ahead */
      
      /* Shift the phonetable if new data needed */
		if (mb->phoneBuff.CurPhone==mb->phoneBuff.NPhones)
			shift_PhoneBuff(&mb->phoneBuff);
		
		/*
		 * Test if the previous call to FillCommandBuffer had resulted in
		 * FLUSH or EOF.   EOF is possible only with the non-library version
		 */
		if ( (mb->phoneBuff.state_pho==PHO_FLUSH) ||
			 (mb->phoneBuff.state_pho==PHO_EOF))
		{
			StatePhone temp=mb->phoneBuff.state_pho;
			mb->phoneBuff.state_pho=PHO_OK; /* reset */
			return(temp);
		}
		/* Read new phoneme serie */
		mb->phoneBuff.state_pho=FillCommandBuffer(mb);
		/* Premature exit */
		if (mb->phoneBuff.state_pho==PHO_ERROR)
			return PHO_ERROR;
		
		/* The pitch sequence is not complete */
		if (!mb->phoneBuff.Closed)
		{
			mb->phoneBuff.state_pho=PHO_OK; /* reset for further readings */
			return(PHO_EOF);
		}
    }

    *ph = mb->phoneBuff.Buff[mb->phoneBuff.CurPhone];
    mb->phoneBuff.Buff[mb->phoneBuff.CurPhone++] = NULL;
	return(PHO_OK);
}
/* diphone.c */

DiphoneSynthesis* init_DiphoneSynthesis(Mbrola *mb, int nb)
/*  Alloc memory, working and audio buffers for synthesis */
{
	DiphoneSynthesis* self;
  
	self= &mb->dbs[nb];
    memset(self, 0, sizeof (*self));
    
	return(self);
}

void reset_DiphoneSynthesis(DiphoneSynthesis* ds)
/*
 * Forget the diphone in progress
 */
{
	ds->Descriptor=NULL;
  
	if (LeftPhone(ds))
    {
		close_Phone( LeftPhone(ds) );
		LeftPhone(ds)=NULL;
    }
  
	if (RightPhone(ds))
    {
		close_Phone( RightPhone(ds) );
		RightPhone(ds)= NULL;
    }
}

void close_DiphoneSynthesis(DiphoneSynthesis* ds)
/* Release memory and phone */
{
	reset_DiphoneSynthesis(ds);
}

int GetPitchPeriod(DiphoneSynthesis *dp, int cur_sample,int Freq)
/*
 * Returns the pitch period (in samples) at position cur_sample 
 * of dp by linear interpolation between pitch pattern points.
 */
{
	int i;
	Phone *work_phone;
	float phon_time;		/* Time from the begining of the phoneme */
	float curtime= cur_sample*1000.0f/(float)Freq; /* Time in ms */
	float return_val;
  
  
	/* Left part of the diphone */
	if (cur_sample < Length1(dp))
    {
		work_phone=LeftPhone(dp);
		phon_time= curtime + length_Phone(LeftPhone(dp)) - 
			Length1(dp)/(Freq/1000); 
      
		/* Too far */
		if (phon_time >= length_Phone(LeftPhone(dp)))
		{
			Phone *phon=LeftPhone(dp);
			return (int) ((float)Freq/freq_Pitch(tail_PitchPattern(phon)));
		}
    }
	else		/* Right part of the diphone */
    {
		work_phone=RightPhone(dp);
		phon_time= curtime - Length1(dp)/(Freq/1000) ;
    }
  
	if (phon_time<0.0f) 
		phon_time=0.0f;
  
	i=0;
	while ( (i < work_phone->NPitchPatternPoints) &&
			(phon_time >= work_phone->PitchPattern[i].pos))
		i++;
  
	if (i>=work_phone->NPitchPatternPoints)
		return_val= freq_Pitch(tail_PitchPattern(work_phone));
	else	/* Linear interpolation of pitch value */
		return_val= work_phone->PitchPattern[i-1].freq
			+ (work_phone->PitchPattern[i].freq - work_phone->PitchPattern[i-1].freq)
			/(work_phone->PitchPattern[i].pos - work_phone->PitchPattern[i-1].pos)
			* (phon_time - work_phone->PitchPattern[i-1].pos);

	return( (int)((float)Freq / return_val) );
}

/* mbrola.c */

#include <math.h>

void init_Hanning(float* table,int size,float ratio)
/* 
 * Initialize the Hanning weighting window  
 * Ratio is used for volume control (Volume is embedded in hanning to
 * spare a multiplication later ...)
 */
{
	int i;
  
	for (i=0; i<size; i++) 
		table[i]= (float) (ratio * 0.5 * (1.0 - cos((double)i*2*3.14159265358979323846/(float)size)));
}

void set_voicefreq_Mbrola(Mbrola* mb, uint16 OutFreq)
/* Change the Output Freq and VoiceRatio to change the vocal tract 	*/
{
	VoiceFreq(mb)=OutFreq;
	VoiceRatio(mb)=(float) VoiceFreq(mb) / MBRFreq;
}

uint16 get_voicefreq_Mbrola(Mbrola* mb)
/* Get output Frequency */
{ return( VoiceFreq(mb)); }

void set_smoothing_Mbrola(Mbrola* mb, uint8_t smoothing)
/* Spectral smoothing or not */
{ smoothing(mb)= smoothing; }

uint8_t get_smoothing_Mbrola(Mbrola* mb)
/* Spectral smoothing or not */
{ return smoothing(mb) ; }

void set_no_error_Mbrola(Mbrola* mb, uint8_t no_error)
/* Tolerance to missing diphones */
{ no_error(mb)=no_error; }

uint8_t get_no_error_Mbrola(Mbrola* mb)
/* Spectral smoothing or not */
{ return no_error(mb); }

void set_volume_ratio_Mbrola(Mbrola* mb, float volume_ratio)
/* Overall volume */
{ 
	init_Hanning( weight(mb), 2*MBRPeriod, volume_ratio);
	volume_ratio(mb)= volume_ratio;
}

float get_volume_ratio_Mbrola(Mbrola* mb)
/* Overall volume */
{ return volume_ratio(mb); }

void set_time_ratio_Mbrola(Mbrola *mb, float ratio)
{
    mb->phoneBuff.TimeRatio=ratio;
}

void set_freq_ratio_Mbrola(Mbrola *mb, float ratio)
{
    mb->phoneBuff.FreqRatio=ratio;
}

float get_time_ratio_Mbrola(Mbrola *mb)
{
    return mb->phoneBuff.TimeRatio;
}

float get_freq_ratio_Mbrola(Mbrola *mb)
{
    return mb->phoneBuff.FreqRatio;
}

void set_input_userData_Mbrola(Mbrola *mb, void *userData)
{
    mb->phoneBuff.inputUserData = userData;
}

static uint32_t init_flash(void)
{
#ifndef HAVE_WAVEBLOB    
    esp_partition_iterator_t pat;
    static uint32_t blob_start = 0;
    if (blob_start) return blob_start;
    pat = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (pat) {
        const esp_partition_t *t= esp_partition_get(pat);
        //printf("%x %x %08x %08x %s\n",t->type, t->subtype, t->address, t->size, t->label);
        if (!strcmp(t->label, "mbrola")) {
            blob_start = t->address;
            break;
        }
        pat = esp_partition_next(pat);
    }
    esp_partition_iterator_release(pat);
    if (!blob_start) {
        printf("Mbrola data partition not found\n");
        return 0;
    }
    char buf[6];
    int et = spi_flash_read(blob_start,buf,4);
    buf[4] = 0;
    if (et) {
        printf("Flash read error\n");
        return 0;
    }
    if (strcmp(buf,BLOB_ID)) {
        printf("Not a MBROLA '%s' blob\n", BLOB_ID);
        return 0;
    }
    return blob_start;
#else
    return 1;
#endif
}

Mbrola* init_Mbrola(int (* grabLine)(char *, int, void *), int (output_function)(int16_t *, int ))
{
	Mbrola* mb;

    // flash init
    uint32_t faddr = init_flash();
    if (!faddr) return NULL;
    mb= (Mbrola*) MBR_malloc(sizeof(Mbrola));

    mb->flash_address = faddr;
  
	/* Default settings ! */
	set_voicefreq_Mbrola(mb, MBRFreq); /* VoiceRatio=1.0 */
	set_volume_ratio_Mbrola(mb, 1.0f);
	set_smoothing_Mbrola(mb,True);
	set_no_error_Mbrola(mb,False);

	saturation(mb) = False;
	audio_length(mb) =0;
	last_time_crumb(mb) =0;
	first_call(mb)=True;

	/* prev_diph points to the previous diphone synthesis structure
	 * and cur_diph points to the current one. While progressing to 
	 * the next diphone, prev_diph memory is resetted the pointers
	 * are swapped between cur and prev diphones
	 */
  
	prev_diph(mb)= init_DiphoneSynthesis(mb, 0);

	cur_diph(mb)=  init_DiphoneSynthesis(mb, 1);

	nb_end(mb)=1000; /* set to high value for first pass in Concat() */

    init_PhoneBuff(mb,(float)MBRFreq / (float)MBRPeriod, 1.0, 1.0, grabLine);
	return(mb);
}


void close_Mbrola(Mbrola* mb)
/* Free the memory ! */
{

	MBR_free(mb);
}

uint8_t reset_Mbrola(Mbrola* mb)
/* 
 * Gives initial values to current_diphone (not synthesized anyway)
 * -> it will give a first value for prev_diph when we make the first
 *    NextDiphone call so that cur_diph= _-FirstPhon with lenght1=0
 *    and prev_diph= _-_ with length2=0
 *
 * return False in case of error
 */
{
	int i;

	/* Free residual phonemes
	 * Right phoneme of prev_diph, and Left phoneme of cur_diph are shared.
	 * So to avoid double desallocation, scratch one 
	 */
	reset_DiphoneSynthesis(cur_diph(mb));
	RightPhone(prev_diph(mb))=NULL;
	reset_DiphoneSynthesis(prev_diph(mb));
  
	/* Set dummy cur_diphone with empty value */
	LeftPhone(cur_diph(mb)) = init_Phone(mb, sil_phone, 0.0); 
	RightPhone(cur_diph(mb)) = init_Phone(mb, sil_phone, 0.0); 
    if (!getdiphone_Database(mb, cur_diph(mb) ))
    {
		printf("_-_ PANIC with %s!\n", sil_phone);
		return False;
    }
  
	/* Indicate we don't generate anything with the first _-_ diphone  */
	nb_pm(cur_diph(mb))=1;
  
	/* The ola window is null from the start */
	for (i=0; i< 2*MBRPeriod; i++)
		ola_win(mb)[i]=0.0f;



  
	/* Indicate that the first call to read_MBR must trigger an initialization */
	first_call(mb)=True;
	return True;
}

uint8_t MatchProsody(Mbrola* mb)
/*
 * Selects Duplication or Elimination for each analysis OLA frames of
 * the diphone we must synthesize (prev_diph). Selected frames must fit
 * with wanted pitch pattern and phonemes duration of prev_diph
 *
 * Return False in case of error
 */
{
	int nb_frame;             /* Nbr of pitch markers in the database segment*/
	float theta1;             /* Time warping coefficients               */
	float theta2=0.0f;
	float beta=0.0f;
	int halfseg;              /* Index (in samples) of center of diphone     */
	int cur_sample;           /* sample offset in synthesis window           */
	int t;                    /* time in analysis window                     */
	int limit;                /* sample where to stop                        */
	int i;
	int k;                    /* dummy variable !!!                          */
	int len_anal;             /* Number of sample in last segment analysed   */
	int start;
	int old_len1;             /* Length1 in samples before adjustment */
  
  
	/* Modify the length of the 2nd part of a phone if the end of its 1st 
	   part does not correspond to its theoretical value */
	start=(int)( length_Phone(LeftPhone(prev_diph(mb))) - 
				  Length1(prev_diph(mb)) / (MBRFreq*1000.0f));

	old_len1=Length1(prev_diph(mb));
    
	if (old_len1>0)
    {
		Length1(prev_diph(mb))+= last_time_crumb(mb) ;

		/* redraw pitch curve with new length */
		for (i=0; i< NPitchPatternPoints(LeftPhone(prev_diph(mb))) ;i++)
			if (PitchPattern(LeftPhone(prev_diph(mb)))[i].pos > start)
			{
				PitchPattern(LeftPhone(prev_diph(mb)))[i].pos= start+
					(PitchPattern(LeftPhone(prev_diph(mb)))[i].pos - start) * 
					Length1(prev_diph(mb))/old_len1;
			}
    }

	nb_frame = nb_frame_diphone(prev_diph(mb));
	halfseg = halfseg_diphone(prev_diph(mb));
	len_anal = nb_frame * MBRPeriod;

	theta1 = ((float) Length1(prev_diph(mb))) / (float) halfseg;
	if (len_anal-halfseg!=0)
    {
		theta2 = ((float) Length2(prev_diph(mb))) / (float) (len_anal-halfseg);
		beta   = (float) Length1(prev_diph(mb)) - (theta2*halfseg);
    }

	/*
	 * Assign to each synthesis frame the corresponding one in segment analysis
	 */
	frame_number(mb)[0]=1;
	frame_pos(mb)[0]=0;

	k=1;   /* first window pos (sample)   */
	
	if ( pmrk_DiphoneSynthesis( prev_diph(mb), 1)
		 & VOICING_MASK )
    {
		cur_sample = GetPitchPeriod(prev_diph(mb), 0, MBRFreq);
    }
	else 
    {
		cur_sample = MBRPeriod;
    }
  
	t= MBRPeriod;
  
	/* stride through analysis frames  */
	for(i=1; i<=nb_frame ;i++)
    {
		if (t <= halfseg)	/* Left phone */
			limit = (int) (t*theta1);
		else		/* Right phone */
			limit = (int) (t*theta2 + beta);
      
		/* Stride through synthesis windows */
		while (cur_sample <= limit)
		{
			frame_number(mb)[k]=i;    /* NB : first frame number = 1 */
			frame_pos(mb)[k]= cur_sample;
			k++;

	
			if (pmrk_DiphoneSynthesis( prev_diph(mb), i)
				& VOICING_MASK )
			{
				cur_sample += GetPitchPeriod(prev_diph(mb),cur_sample,MBRFreq);
			}
			else
			{
				cur_sample += MBRPeriod;
			}
			if (k>=NBRE_PM_MAX)
			{
				printf("%s-%s Concat : PANIC, check your pitch :-)\n",
							   name_Phone(LeftPhone(prev_diph(mb))),	
							   name_Phone(RightPhone(prev_diph(mb))));
				return False;
			}
		}
		t=t+MBRPeriod;
    }
 
	frame_number(mb)[k]=0; /* End tag */
	nb_pm(prev_diph(mb)) = k-1; /* Number of pitchmarks for synthesis */
  
	/* total length that should have been synthesized -last effective sample */
	last_time_crumb(mb)+= (old_len1+Length2(prev_diph(mb))) - frame_pos(mb)[k-1] ;

	return True;
}

StatePhone NextDiphone(Mbrola* mb)
/*
 * Reads a phone from the phonetic command buffer and prepares the next
 * diphone to synthesize ( prev_diph )
 * Return value may be: PHO_EOF,PHO_FLUSH,PHO_OK, PHO_ERROR
 */
{
	int len_left, len_right, tot_len;
	float wanted_len;
	int len_anal;	   /* Number of sample in last segment analysed     */
	Phone* my_phone;
	StatePhone state;
  
	DiphoneSynthesis *temp;
  
  
    if ((state = next_PhoneBuff(mb, &my_phone))) 
    {
		return(state);
    }
	/* length and freq modified if the vocal tract length is not 1.0 */
	applyRatio_Phone(my_phone,VoiceRatio(mb));
  
	/* Prev_diph has been used ... release LeftPhone 
	 * don't RightPhone, as reference on it is still active in cur_diph 
	 */
	if (LeftPhone(prev_diph(mb)))
    {
		close_Phone(LeftPhone(prev_diph(mb)));   /* once used, release the phoneme */
		LeftPhone(prev_diph(mb))=NULL;
    }
  
	/* Shift the diphone being worked on */
	/* prev_diph is the one that will be synthesized */
    temp=prev_diph(mb);	
	prev_diph(mb)=cur_diph(mb); 
	cur_diph(mb)=temp;
        
	/* Initialize minimal info of the new cur_diph */
	RightPhone(cur_diph(mb))= my_phone;
	LeftPhone(cur_diph(mb)) = RightPhone(prev_diph(mb)); /* Reuse common part ! */
	/* 
	 * Load the diphone from the database !!
	 */
	if ( ! getdiphone_Database(mb, cur_diph(mb)) )
    {
		uint8_t success= False; /* then ... we failed */
      
		/* Try to arrange a solution for the missing diphone */
		if (no_error(mb) || 1) 
		{
		    char temp_left[4], temp_right[4];
			strcpy(temp_left, name_Phone( LeftPhone( cur_diph(mb))));
			strcpy(temp_right, name_Phone( RightPhone( cur_diph(mb))));
	  
	  
			/* Momentary replacement with _-_ */
			strcpy(name_Phone( LeftPhone( cur_diph(mb))), sil_phone);
			strcpy(name_Phone( RightPhone( cur_diph(mb))), sil_phone);
	  
			success= getdiphone_Database(mb, cur_diph(mb));
	  
			/* Restore situation */
			strcpy(name_Phone( LeftPhone( cur_diph(mb))), temp_left);
			strcpy(name_Phone( RightPhone( cur_diph(mb))), temp_right);
		}
    	if (!success)
      	{ 
			printf("Fatal error: Unknown recovery for %s-%s segment\n",
						  name_Phone(LeftPhone(cur_diph(mb))),
						  name_Phone(RightPhone(cur_diph(mb))));
	  
			return PHO_ERROR;
		}
    }
  
	/*
	 * Computation of length of 2nd part of prev_diph and of 1st part of current
	 * one : current phoneme gives current diphone => impose length of 2nd
	 * part of previous and of 1st part of current one proportionally to their
	 * relative importance in the database.
	 * Ex : phoneme sequence = A B C. When synthesizer receives C, B-C is
	 * scheduled, and the length of B in A-B and B-C  are derived. Synthesis of
	 * A-B can then start.
	 */
  
	len_anal = ( nb_frame_diphone(prev_diph(mb)) ) * MBRPeriod;
	len_left= len_anal -  halfseg_diphone(prev_diph(mb));
	len_right= halfseg_diphone(cur_diph(mb)) ;
	tot_len=len_left+len_right;
  
	wanted_len= length_Phone( LeftPhone(cur_diph(mb)) );
  
	Length2(prev_diph(mb))= (int) (wanted_len * (float)len_left * (float)MBRFreq
								   / 1000.0f / (float)tot_len );
	Length1(cur_diph(mb)) = (int) (wanted_len * (float) MBRFreq / 1000.0f - 
								   Length2(prev_diph(mb))) ;

	return(PHO_OK);
}

void oneshot_Mbrola(Mbrola* mb)
/*
 * Completely perform MBROLA synthesis of prev_diph(mb)
 * Different from audio chunks synthesized on demand in library mode 
 */
{

	MatchProsody(mb); /* we're in standalone */
	Concat(mb);

	odd(mb)=False;
	for (frame_counter(mb)=1; frame_counter(mb)<=nb_pm(prev_diph(mb)); frame_counter(mb)++)
    {
		OverLapAdd(mb,frame_counter(mb));
    }
  
}
void FlushFile(Mbrola* mb, int shift, int shift_zero)
/*
 * Flush on file what's computed
 */
{
	int k;
  
	for (k=0;k<shift;k++) 
    {
		if (ola_win(mb)[k] > 32765)
		{
			saturation(mb)=True;
			ola_integer(mb)[k]=32765;
		}
		else if (ola_win(mb)[k] < -32765)
		{
			saturation(mb)=True;
			ola_integer(mb)[k]=-32765;
		}
		else 
			ola_integer(mb)[k]= (int16) ola_win(mb)[k];
    }
  
	/* Amount that has been flushed */
	buffer_shift(mb)=shift;

	/* Zero padding ! */
	zero_padding(mb)= shift_zero;  
    if (mb->output_function) {
        audio_length(mb)+=mb->output_function(ola_integer(mb), shift);
        /* Fill the gap between 2 frames for extra low pitch */
        /* No lower limit, but write MBRPeriod buffer each time */
        if (shift_zero>0)
        {		 
            int shift_mod;						  /* Modulo for shift zero */
            int written;
            
            if (shift_zero>2*MBRPeriod)
                shift_mod=2*MBRPeriod;
            else
                shift_mod=shift_zero;
            
            for (k=0; k<shift_mod; k++) ola_integer(mb)[k]=0;
            while (shift_zero>2*MBRPeriod)
            {
                written = mb->output_function(ola_integer(mb), 2*MBRPeriod);
                buffer_shift(mb)+=written;
                audio_length(mb)+=written;
                shift_zero-=written;
            }
            written= mb->output_function(ola_integer(mb),shift_zero);
            audio_length(mb)+=written;
            buffer_shift(mb)+=written;
        }
    }
}


void Concat(Mbrola* mb)
/*
 * This is a unique feature of MBROLA.
 * Smoothes diphones around their concatenation point by making the left
 * part fade into the right one and conversely. This is possible because
 * MBROLA frames have the same length everywhere.
 *
 * output : nb_begin, nb_end -> number of stable voiced frames to be used
 * for interpolation at the end of Leftphone(prev_diph(mb)) and the beginning
 * of RightPhone(prev_diph(mb)).
 */
{
	int c;
	int begin, end; /* Nbr of voiced frames at begining and end of prev_diph(mb)*/
	int first,last;	       /* Number of the first frame */
	int last_frame, first_frame; /* offset in sample for the last and first  */
	/* frame of concatenation point             */
	int16 *buff_left;            /* speech buffer on left of junction  */
	int16 *buff_right;           /* speech buffer on right of junction */
	int i,j;
	int cur_sample;	         /* sample offset in synthesis window        */
	int maxnconcat;
	int limitframe;
  

	/* We compute the first Olaed frame for cur_diph -> we can't do
	 * Matchprosody on it yet since we lack pitch points...
	 * But at least we have a pitch point at 0 for cur_diph
	 */
	cur_sample = GetPitchPeriod(cur_diph(mb), 0, MBRFreq);

	if (Length1(cur_diph(mb))!=0)
    {
		float theta= ((float) Length1(cur_diph(mb))) / halfseg_diphone(cur_diph(mb));
      
		/* Use CEIL as first belongs to [1..N] */
		first= (int) ceil((double)cur_sample / MBRPeriod / theta);
    }
	else
    {
		first=0;	  /* 0 length phonemes */
    }
  
	if (first > frame_number(mb)[ nb_pm(cur_diph(mb)) ])
    {
		first= frame_number(mb)[ nb_pm(cur_diph(mb)) ];
    }
  
	last= frame_number(mb)[nb_pm(prev_diph(mb))];
  
	if ( 
		(first!=0) &&   /* Degenerated case= phoneme with 0ms length */ 
		(pmrk_DiphoneSynthesis( prev_diph(mb), last ) & VOICING_MASK) && 
		(pmrk_DiphoneSynthesis( cur_diph(mb),  first )  & VOICING_MASK))
    { 
		/* Difference vector beetween LAST and FIRST Voiced frame */
		smooth(cur_diph(mb))=True;

		/* buffer goes from 0 to NB_SAMPLE while real_frame goes from 1 to N */
		last_frame = MBRPeriod * ( real_frame(prev_diph(mb))[last] -1 );
		buff_left= &buffer(prev_diph(mb))[last_frame];
      
		first_frame = MBRPeriod * (real_frame(cur_diph(mb))[first] -1 );
		buff_right= &buffer(cur_diph(mb))[first_frame];
		
		/* For the first half, no problem */
		for(i=0;i<MBRPeriod;i++)
			smoothw(cur_diph(mb))[i] = (int) ((buff_left[i]-buff_right[i]) * weight(mb)[i]);
	 
		/* For the second half, reset counters of looped frames */
		for(j=0;i<(MBRPeriod*2); i++,j++)
			smoothw(cur_diph(mb))[i] = (int) ((buff_left[j]-buff_right[j]) * weight(mb)[i]);
    }
	else
		smooth(cur_diph(mb))=False;

	/*
	 * Count voiced frames on prev_diph(mb) ( available for smoothing )
	 * 'begin' for the left part and 'end' for the right part
	 */
	limitframe= halfseg_diphone(prev_diph(mb)) / MBRPeriod;
	maxnconcat=(int) (0.5*nb_pm(prev_diph(mb))+1.0);
	if (maxnconcat>6) maxnconcat=6;
	
	begin=1;
	while ((pmrk_DiphoneSynthesis(prev_diph(mb), frame_number(mb)[begin]) == V_REG)
		   && (frame_number(mb)[begin] <= limitframe) 
		   && (begin < maxnconcat))
		begin++;
  
	/* if VREG VREG ... VREG VTRA : smooth VTRA too */
	if ( ( begin>1 )
		 && ( pmrk_DiphoneSynthesis(prev_diph(mb),frame_number(mb)[begin]) == V_TRA)
		 && ( frame_number(mb)[begin] <= limitframe)
		 && ( begin < maxnconcat))
		begin++;
  
	nb_begin(mb) = begin -1;			    
	/* Check previous value of nb_end */
	if (nb_begin(mb) > nb_end(mb)) 
		nb_begin(mb)=nb_end(mb);


	end=1;
	c=nb_pm(prev_diph(mb))+1;
	while ((pmrk_DiphoneSynthesis(prev_diph(mb),frame_number(mb)[c-end]) == V_REG) 
		   && (frame_number(mb)[c-end] > limitframe) 
		   && (end < maxnconcat))
		end++;

	/* if VTRA VREG VREG ... VREG : smooth VTRA too */
	if ((end > 1) 
		&& (pmrk_DiphoneSynthesis(prev_diph(mb),frame_number(mb)[c-end]) == V_TRA)
		&& (frame_number(mb)[c-end] > limitframe) 
		&& (end < maxnconcat))
		end++;
	nb_end(mb) = end - 1;

	/* simplification: just check 1st frame of cur_diph, if not
	 * voiced, no interpolation at end of prev_diph(mb)
	 */
	if (pmrk_DiphoneSynthesis(cur_diph(mb), 1) != V_REG) 
		nb_end(mb)=0; 

}
void OverLapAdd(Mbrola* mb, int frame)
/*
 *  OLA routine
 */
{
	int k;
	float correction;	        /* Energy correction factor */
	int end_window, add_window;
	int lim_smooth;		/* Beyond this limit -> left smoothing */
	float tmp;
	uint8_t type;			/* Frame type */
	int shift_zero;		/* Noman's land between 2 ola filled with 0 */
	int shift;			/* Shift between pulses */
  

	shift = frame_pos(mb)[frame]-frame_pos(mb)[frame-1];
	if ((correction = (float)shift/(float)MBRPeriod)>=1) correction=1.0f;
  
	/* Keep nothing of previous frames as there's no overlap */
	shift_zero= shift - 2*MBRPeriod;
	if (shift_zero>0)
		shift= 2*MBRPeriod;
  
	end_window = 2*MBRPeriod - shift;
	add_window =MBRPeriod * (real_frame(prev_diph(mb))[frame_number(mb)[frame]]-1);
	lim_smooth = nb_pm(prev_diph(mb))-nb_end(mb);

	/* Flush on file what's flushable */
	FlushFile(mb,shift,shift_zero);
    
	if (saturation(mb))
    {
		saturation(mb)=False;
    }
  
	/* !! SHIFTING CAN BE REMOVED IN CASE OF STATIC OUTPUT BUFFER !! */
	/* shift the ola window and completion with 0 */
	memmove(&ola_win(mb)[0], &ola_win(mb)[shift], end_window*sizeof(ola_win(mb)[0]));
	for(k=end_window; k<2*MBRPeriod ; k++) 
		ola_win(mb)[k]=0.0f;

	/* Two kinds of OLA depending if the frame is unvoiced */
	type= pmrk_DiphoneSynthesis(prev_diph(mb), frame_number(mb)[frame] );
  

	if (! (type & VOICING_MASK))
    {	/* UNVOICED FRAME => PLAY BACK */
		
		/* Check if we're duplicating an unvoiced frame */
		if ( odd(mb) && 
			 (frame_number(mb)[frame] == frame_number(mb)[frame-1]))
		{
			/* reverse every second duplicated UV frame */
			add_window=add_window+2*MBRPeriod-1;
			for (k=0; k< 2*MBRPeriod ; k++)
			{
				tmp = weight(mb)[k] * (float) buffer(prev_diph(mb))[add_window - k];
				  
				/* Energy correction  */
				tmp *= correction;
				ola_win(mb)[k] += tmp;
			}
		}
		else		  /* Don't reverse the unvoiced frame */
		{
			for (k=0; k< 2*MBRPeriod ; k++)
			{
				tmp = weight(mb)[k] * (float) buffer(prev_diph(mb))[add_window + k];
				  
				/* Energy correction  */
				tmp *= correction;
				ola_win(mb)[k] += tmp;
			}
		}
    }
	else
    { 
		/* Meeep meeep, you're entering a restrictead area, helmet is
		 * recommended if you don't have rights to exploit Belgian
		 * patent BE09600524 or US Patent Nr. 5,987,413
		 *
		 * Voiced frame -> autoloop ! MBROLA unique feature !!
		 * Many case depending on smoothing or not
		 */       
	   
		if ((frame<=nb_begin(mb)) && 
			smooth(prev_diph(mb)) && 
			smoothing(mb) )
			/* Left smoothing */
		{
			float smooth_left = (float)(nb_begin(mb)-frame+1) / (2*(float)nb_begin(mb));
	  
			for (k=0; k<MBRPeriod ; k++)
				ola_win(mb)[k] += correction * 
					( weight(mb)[k] * (float)buffer(prev_diph(mb))[add_window + k]
					  + smooth_left * smoothw(prev_diph(mb))[k] );
	  
			for ( ; k<2*MBRPeriod ; k++)
				ola_win(mb)[k] += correction * 
					( weight(mb)[k] * (float) buffer(prev_diph(mb)) [add_window + k - MBRPeriod]
					  + smooth_left * smoothw(prev_diph(mb))[k] );
		}
		else if ( (frame>lim_smooth)   && 
				  smooth(cur_diph(mb)) &&
				  smoothing(mb) )
			/* Right smoothing */
		{
			float smooth_right= (float)(nb_end(mb)-(nb_pm(prev_diph(mb))-frame))
				/(2*(float)nb_end(mb));

			for (k=0; k<MBRPeriod ; k++)
				ola_win(mb)[k] += correction * 
					( weight(mb)[k] * (float)buffer(prev_diph(mb))[add_window + k]
					  - smooth_right * smoothw(cur_diph(mb)) [k]);
			 
			for ( ; k<2*MBRPeriod ; k++)
				ola_win(mb)[k] += correction * 
					( weight(mb)[k] * (float) buffer(prev_diph(mb)) [add_window + k - MBRPeriod]
					  - smooth_right * smoothw(cur_diph(mb))[k]);
		}
		else 
			/* No smoothing */
		{
			for (k=0; k<MBRPeriod ; k++)
				ola_win(mb)[k] += correction * weight(mb)[k] * 
					(float)buffer(prev_diph(mb))[add_window + k];
			for ( ; k<2*MBRPeriod ; k++)
				ola_win(mb)[k] += correction * weight(mb)[k] * 
					(float) buffer(prev_diph(mb))[add_window + k - MBRPeriod];
		}
    }  
  
	odd(mb)= !odd(mb);							  /* Flip flop for NV frames */

}
#if 0
StatePhone Synthesis(Mbrola* mb)
/*
 * Main loop: performs MBROLA synthesis of all diphones
 * Returns a value indicating the reasons of the break
 * (a flush request, a end of file, end of phone sequence)
 */
{
	StatePhone stream_state;	/* Indicate a # or eof has been encountered in the   */
	/* command file -> flush and continue         */

  
	/* 
	 * Put something in the pipe, and initialization of junk diphone cur_diph
	 * that will be prev_diph at time of Concat -> needed for "buff_left"
	 */
	NextDiphone(mb);  /* standalone mode */
	while ((stream_state=NextDiphone(mb)) == PHO_OK)
		oneshot_Mbrola(mb);
  
	return(stream_state);
}

#endif
int read_Mbrola(Mbrola* mb, int16_t *buffer_out, int nb_wanted)
/*
 * Reads nb_wanted samples in an audio buffer
 * Returns the effective number of samples read
 *
 * negative means error code
 */
{
	int to_go= nb_wanted; /* number of samples to go */
	int nb_move;
    
	if (first_call(mb))
    {
		odd(mb)=0;
		eaten(mb)=0;
		buffer_shift(mb)=0;
		zero_padding(mb)=0;
		frame_counter(mb)=1;
		if (!reset_Mbrola(mb))
			return lasterr_code;
		
		/* Test if there is something in the buffer */
		switch (  NextDiphone(mb) )
		{
		case PHO_OK: /* go ahead */
			break;
		case PHO_ERROR:  /* return error code */
			return lasterr_code;
		case PHO_EOF:
		case PHO_FLUSH:  /* Flush or EOF, 0 samples */
			return 0;      
		default:
			/* panic ! */
			printf("NextDiphone PANIC!\n");
			return ERROR_NEXTDIPHONE ; 
		}
		first_call(mb)=False;
    }
  
	while (to_go>0)
    {
		/* Zero padding between 2 Ola frames for extra low pitch */
		if (zero_padding(mb)>0)
		{
			int nb_generated;
			 
			if ( zero_padding(mb) > to_go)
				nb_generated= to_go;
			else
				nb_generated= zero_padding(mb);

			/* decrement in case the requested buffer is very small */
			zero_padding(mb)-= nb_generated; 
			if (nb_generated) {
                memset(buffer_out, 0, 2*nb_generated);
                buffer_out += nb_generated;
            }
			to_go -= nb_generated;
		}
		
		/* Available frames from previous OLA */
		if (to_go > (buffer_shift(mb)-eaten(mb)))
			nb_move= buffer_shift(mb)-eaten(mb);
		else
			nb_move= to_go;
		memcpy(buffer_out, &ola_integer(mb)[eaten(mb)], 2*nb_move);
        buffer_out += nb_move;
		to_go-= nb_move;
		eaten(mb)+= nb_move;
		
		if (to_go<=0)
			break;
		
		/* Still some frames available in the same diphone ? */
		if (frame_counter(mb)<nb_pm(prev_diph(mb)))
			frame_counter(mb)++;
		else
		{ /* Else, nextdiphone */
			StatePhone stream_state;
			 
			stream_state= NextDiphone(mb);
			 
			if (stream_state != PHO_OK)
			{
				/* handle errors in the parser */
				if (stream_state == PHO_ERROR)
					return lasterr_code;
	      
				/* Flush or EOF */
				if (stream_state == PHO_FLUSH)
					first_call(mb)=True;
				break;
			}
			 
			if ( !MatchProsody(mb) )
				return lasterr_code;
			 
			Concat(mb);
			odd(mb)=0;
			frame_counter(mb)=1;
		}
		
		eaten(mb)=0;

		/* condition against phonemes with 0 length */
		if (frame_counter(mb)<=nb_pm(prev_diph(mb)))
			OverLapAdd(mb, frame_counter(mb));
    }
	/* old C++ catch throw  "}  catch(int ret) { return ret;  }"  */
	return(nb_wanted - to_go);
}

/* flash.c */

static void init_real_frame(DiphoneSynthesis *diph)
/* verbatim copy from original code */
/*
 * Make the link between logical and physical frames
 */
{
        int pred_type;          /* Type ( Voiced/ Unvoiced ) of the previous fra
me */
        uint8 i;                          /* physical number of frames of the di
phone */
  
        /* Stride thru the pitch marks to spot Unvoiced-Voiced transitions */
        tot_frame(diph)=1;
        real_frame(diph)[0]=1;
        pred_type=V_REG;
  
        for(i=1; i<= nb_frame_diphone(diph) ; tot_frame(diph)++,i++)
    {
                /* Check for extra frame at the end of an unvoiced sequence */
               if ( !(pred_type & VOICING_MASK )
                         && 
                         ( pmrk_DiphoneSynthesis(diph, i) & VOICING_MASK ))
                {
                        tot_frame(diph)++;
                }
      
                real_frame(diph)[i]=tot_frame(diph);
                pred_type=pmrk_DiphoneSynthesis(diph, i);
    }
        tot_frame(diph)--;
  
        /* If the last is unvoiced, bonus frame !!! */
        if (! (pred_type & VOICING_MASK ))
                tot_frame(diph)++;
}

static int nameIdx(const char *name)
{
    int i;
    for (i=0; i<PhoneNameCount; i++) if (!strcmp(PhoneNames[i], name)) return i;
    return -1;
}
static uint8_t init_common_Database(DiphoneSynthesis *diph)
{
    int n1 = nameIdx(name_Phone(LeftPhone(diph)));
    if (n1 < 0) return False;
    int n2 = nameIdx(name_Phone(RightPhone(diph)));
    if (n2 < 0) return False;
    int idx = n1 * PhoneNameCount + n2;
#ifndef INCOMPLETE_DIPHONES
    Descriptor(diph)= &parahash[idx];
#else
    /* bsearch */
    int low=0;
    int hig = PARAHASH_COUNT-1;
    int mid;
    int sdx;
    while (low <= hig) {
        mid = (low + hig) / 2;
        sdx = parahash[mid].left * PhoneNameCount + parahash[mid].right;
        if (sdx == idx) break;
        if (sdx < idx) low = mid+1;
        else hig = mid-1;
    }
    if (low > hig) return False;
    Descriptor(diph)= &parahash[mid];
#endif
    
           /* Substract 1 as index will go from 1 to N (instead of 0..N-1) */
    diph->p_pmrk= & FrameTypeT[ diph->Descriptor->pos_pm / 4 ];
    diph->p_pmrk_offset= diph->Descriptor->pos_pm % 4;
  
        /* link between physical and logical frames */
        init_real_frame(diph);

    return True;
}

#if COMPRESSION_TYPE > 0
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static int alaw2linear(unsigned char	a_val)
{
	int		t;
	int		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

static void uncompress_alaw(int16_t *buf, int n)
{
    uint8_t *abuf=(uint8_t *)buf;
    int i;
    for (i=n-1; i>=0; i--) buf[i]=alaw2linear(abuf[i]);
}

#if COMPRESSION_TYPE == 2
static void dup_wave(int16_t *buf, int n)
{
    int i,last=buf[n-1];
    for (i=n-1; i>=0; i-=2) {
        buf[i-1] = buf[i/2];
        buf[i] = (buf[i/2] + last) / 2;
        last = buf[i/2];
    }
}
#endif
#endif

static void getdiphone_data(Mbrola *mb,int pos, int16_t *whereto, int count)
{
#ifdef HAVE_WAVEBLOB
    memcpy(whereto, waveblob+pos, count);
#else
    count = (count+3) & 0xffffc;
    spi_flash_read(mb->flash_address + pos, whereto, count);
#endif
}


uint8_t getdiphone_Database(Mbrola *mb,DiphoneSynthesis *diph)
/* 
 * Basic loading of the diphone specified by diph. Stores the samples
 * Return False in case of error
 */
{

        if (! init_common_Database(diph))
                return False;

#if COMPRESSION_TYPE == 0
    getdiphone_data(mb,pos_wave_diphone(diph), buffer(diph), tot_frame(diph)*MBRPeriod * 2);
#elif COMPRESSION_TYPE == 1
    getdiphone_data(mb,pos_wave_diphone(diph), buffer(diph), tot_frame(diph)*MBRPeriod);
    uncompress_alaw(buffer(diph), tot_frame(diph)*MBRPeriod);
#else
    getdiphone_data(mb,pos_wave_diphone(diph), buffer(diph), tot_frame(diph)*MBRPeriod / 2);
    uncompress_alaw(buffer(diph), tot_frame(diph)*MBRPeriod / 2);
    dup_wave(buffer(diph), tot_frame(diph)*MBRPeriod);
#endif
    return True;
}
