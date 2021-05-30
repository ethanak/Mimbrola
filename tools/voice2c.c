#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>


FILE *dbfile;
char *dbfname;
char *dbname;
const char *outdirname;
uint8_t *vblob;

size_t freadp(void *dst, size_t size, size_t nmemb)
{
    size_t n = fread(dst, 1, size * nmemb, dbfile);
    if (n<0) {
        perror(dbfname);
        exit(1);
    }
    if (n != size * nmemb) {
        fprintf(stderr, "Premature end of file\n");
        exit(1);
    }
    return n;
}

uint16_t read_uint16(uint16_t *whereto)
{
    uint16_t result;
    freadp(&result, 1, 2);
    if (whereto) *whereto = result;
    return result;
}

uint8_t read_uint8(uint8_t *whereto)
{
    uint8_t result;
    freadp(&result, 1, 1);
    if (whereto) *whereto = result;
    return result;
}

uint32_t read_uint32(uint32_t *whereto)
{
    uint32_t result;
    freadp(&result, 1, 4);
    if (whereto) *whereto = result;
    return result;
}

char *readString(char *buf, int maxlen)
{
    int i;
    for (i=0;;i++) {
        int c= fgetc(dbfile);
        if (!c) break;
        if (i >= maxlen-1) {
            fprintf(stderr, "String too long\n");
            exit(1);
        }
        buf[i]=c;
    }
    buf[i]=0;
    return buf;
}

typedef uint16_t PhonemeCode;
/* database common */
int nb_diphone;
uint8_t MBRPeriod;
uint16_t Freq;
uint8_t Coding;
uint32_t SizeMrk, SizeRaw;
char sil_phon[16];
int max_frame, max_samples;
int round_size;
uint8_t *pmrk;
uint32_t RawOffset;
typedef struct
{
	/* Name of the diphone     */
    char left_name[16];
    char right_name[16];
	PhonemeCode left;
	PhonemeCode right;	

	uint32_t pos_wave;	/* position in SPEECH_FILE */
	uint16_t halfseg;	/* position of center of diphone */
	uint32_t pos_pm;		/* index in PITCHMARK_FILE */
	uint8_t nb_frame;    	/* Number of pitch markers */
    uint32_t newposwave;
} DiphoneInfo;

DiphoneInfo *newtab;

#define MAGIC_HEADER 0x4f52424d 
#define SYNTH_VERSION "3.4-dev"

void openDB()
{
    uint32_t magic[2],OldSizeMrk;
    char version[6];
    if (!(dbfile=fopen(dbfname,"rb"))) {
        perror(dbfname);
        exit(1);
    }
    freadp(magic,6,1);
    if (strncmp((void *)magic,"MBROLA",6) || magic[0] != MAGIC_HEADER) {
        fprintf(stderr,"Not a MBROLA file\n");
        exit(1);
    }
    freadp(version, 5, 1);
    version[5]=0;
    nb_diphone = read_uint16(NULL);
	OldSizeMrk=read_uint16(NULL);
	if (OldSizeMrk==0)		 
    { /* Sign of the new format !!! */
		SizeMrk=read_uint32(NULL);
    }
	else
    { /* old format */
		SizeMrk = OldSizeMrk;
    }
  
	SizeRaw=read_uint32(NULL);
    Freq = read_uint16(NULL);
    MBRPeriod = read_uint8(NULL);
    Coding = read_uint8(NULL);
  
	if (strcmp( version, SYNTH_VERSION) > 0 )
    {
        fprintf(stderr, "Database comes from future\n");
        exit(1);
    }
    if (strcmp("2.05",version)>0)
    {	/* Oh my, we're older than 2.05, let's call the compatibility driver! */
		Coding=0;
    }
    
	printf("VERSION %s\n", version);
	printf("nb_diphone %i\n",nb_diphone);
	printf("SizeMrk %i\n",SizeMrk);
	printf("SizeRaw %i\n",SizeRaw);
	printf("Freq %i\n",Freq);
	printf("MBRPeriod %i\n",MBRPeriod);
	printf("Coding %i\n",Coding);
}

void readIndex(void)
{
    int error=0;
	int i;

	int32_t indice_pm=0;   /* cumulated pos in pitch mark vector */
	int32_t indice_wav=0;  /* cumulated pos in the waveform dba */
	uint8_t nb_wframe;     /* Physical number of frame */

	char new_left[16];
	char new_right[16];
	int16_t new_halfseg;
	uint8_t new_nb_frame;
	int32_t new_pos_pm;
	int32_t new_pos_wave;

	/* initialize hash table: add extra 25% to enhance performances */
	//diphone_table(dba)=init_HashTab( nb_diphone(dba) + nb_diphone(dba)/4);

    newtab=calloc(nb_diphone, sizeof (*newtab));
  
	/* Insert diphones one by one in the hash table */
	for(i=0;  (indice_pm!=SizeMrk)&&(i<nb_diphone); i++)
    {
        readString(new_left,16);
        readString(new_right,16);
		new_halfseg = read_uint16(NULL);
        new_nb_frame = read_uint8(NULL);
        nb_wframe = read_uint8(NULL);
        
		new_pos_pm= indice_pm;
		indice_pm+= new_nb_frame;
      
		if (indice_pm==SizeMrk)
		{
			/* The last diphone of the database is _-_  */
			strcpy(sil_phon, new_left);
		}
		new_pos_wave= indice_wav;
		indice_wav+= (long) nb_wframe* (long) MBRPeriod;
        strcpy(newtab[i].left_name,new_left);
        strcpy(newtab[i].right_name,new_right);
        newtab[i].left = newtab[i].right = 0xffff;
        newtab[i].pos_wave = new_pos_wave;
        newtab[i].halfseg = new_halfseg;
        newtab[i].pos_pm = new_pos_pm;
        newtab[i].nb_frame = new_nb_frame;
		if (nb_wframe > max_frame)
		{ max_frame=nb_wframe; }
    }
    for( ; i<nb_diphone; i++)
    {
		int position;char left[16], right[16];
        readString(left,16);
        readString(right,16);
        
		for (position=0; position<i; position++) {
            if (!strcmp(left,newtab[position].left_name) && !strcmp(right,newtab[position].right_name)) break;
        }
        if (position >= i ) {
            fprintf(stderr, "Cannot duplicate %s-%s\n", left, right);
            exit(1);
        }
        readString(left,16);
        readString(right,16);
        int j;
        for (j=0; j<i; j++) {
            if (!strcmp(left,newtab[j].left_name) && !strcmp(right,newtab[j].right_name)) break;
        }
        if (j < i) {
            fprintf(stderr, "Segment %s-%s already exists\n", left, right);
            exit(1);
        }
        strcpy(newtab[i].left_name,left);
        strcpy(newtab[i].right_name,right);
        newtab[i].left = newtab[i].right = 0xffff;
        newtab[i].pos_wave = newtab[position].pos_wave;
        newtab[i].halfseg = newtab[position].halfseg;
        newtab[i].pos_pm = newtab[position].pos_pm;
        newtab[i].nb_frame = newtab[i].nb_frame;
		
    }
	/* Size of the buffer that will be allocated in Diphonesynthesis */
	max_samples= MBRPeriod * max_frame;
	round_size= (SizeMrk+3)/4;  /* round to upper value */
    pmrk = (uint8_t *) malloc( round_size);
	freadp(pmrk, 1, round_size);
    RawOffset=ftell(dbfile);
/*
    fseek(dbfile,0, SEEK_END);
    uint32_t bfs = ftell(dbfile) - RawOffset;
    fseek(dbfile,RawOffset, SEEK_SET);
    vblob = malloc(bfs);
    freadp(vblob, bfs, 1);
    fclose(dbfile);
    
    printf("Wave size %d\n", bfs);
    * */
}

int PhoneListNo;
char **phoneList;
int mpl;
int findPhoneme(char *name, int autoalloc)
{
    int i;
    for (i=0; i< PhoneListNo; i++) if (!strcmp(phoneList[i], name)) return i;
    if (!autoalloc) return -1;
    if (PhoneListNo >= mpl) {
        mpl *= 2;
        phoneList = realloc(phoneList, mpl * sizeof(*phoneList));
    }
    phoneList[PhoneListNo++] = strdup(name);
    return i;
}

void createPhonemeList(void)
{
    int i,j;
    mpl=64;
    PhoneListNo = 0;
    phoneList = malloc(mpl * sizeof(*phoneList));
    for (i=0; i<nb_diphone; i++) {
        newtab[i].left = findPhoneme(newtab[i].left_name, 1);
        newtab[i].right = findPhoneme(newtab[i].right_name, 1);
    }
    printf("Phoneme count %d\n", PhoneListNo);
}

int hashcomp(const void *p1, const void *p2)
{
    const DiphoneInfo *h1 = p1, *h2=p2;
    unsigned long pho1 = (h1->left << 16) | (h1->right & 0xffff);
    unsigned long pho2 = (h2->left << 16) | (h2->right & 0xffff);
    return (pho1 > pho2) ? 1: (pho1 < pho2) ?-1 : 0;
}

struct duplicate {
    int origin;
    int start_offset; // in bytes
    int size; // in bytes
    int frames; // real frames
    int flags;
} *duplicador;

#define FDUP_DUPS 1
#define FDUP_SAVED 2

uint8_t compression = 0;
uint8_t mkh=0;
#define VOICING_MASK 2 /* Voiced/Unvoiced mask  */
#define TRANSIT_MASK 1 /* Stationary/Transitory mask */
#define NV_REG 0          /* unvoiced stable state */
#define NV_TRA TRANSIT_MASK    /* unvoiced transient    */
#define V_REG VOICING_MASK    /* voiced stable state   */
#define V_TRA (VOICING_MASK | TRANSIT_MASK)  /* voiced transient      */

#define pmrk_DS(INDEX) ((p_pmrk[ ( (INDEX-1)+p_pmrk_offset)/ 4 ] >> (  2*( ((INDEX-1) + p_pmrk_offset)%4))) & 0x3)
int computeRealFrames(DiphoneInfo *di)
{
    int pred_type=V_REG;
    int tot_frame=1;
    uint8_t *p_pmrk= & pmrk[ di->pos_pm / 4 ];
    int p_pmrk_offset= di->pos_pm % 4;
    int i;
    //return di->nb_frame;
    for (i=1;i<=di->nb_frame;tot_frame++, i++) {
        /* Check for extra frame at the end of an unvoiced sequence */
        if ( !(pred_type & VOICING_MASK ) && (pmrk_DS(i) & VOICING_MASK )) {
            tot_frame++;
        }
        pred_type=pmrk_DS(i);
    }
    tot_frame--;
  
        /* If the last is unvoiced, bonus frame !!! */
    if (! (pred_type & VOICING_MASK ))
                tot_frame++;
    return tot_frame;
}

void find_dups(void)
{
    int i, j,ni,nj,bi,bj,n;
    int mask = mkh? ((1<<compression) - 1) : 3;
    int ds=0;
    duplicador = calloc(nb_diphone, sizeof(*duplicador));
    for (j=0;j< nb_diphone; j++) {
        nj = computeRealFrames(&newtab[j]) * MBRPeriod*2;
        bj = newtab[j].pos_wave*2;
        n = computeRealFrames(&newtab[j]);
        for (i=0; i<nb_diphone; i++) {
            if (duplicador[i].flags & FDUP_DUPS) continue;
            if (j == i) continue;
            ni = computeRealFrames(&newtab[i]) * MBRPeriod*2;
            bi = newtab[i].pos_wave*2;
            if (bj < bi || bj >=ni + bi) continue;
            int start = bj - bi;
            if (start + nj > ni) continue;
#if 0
            {
                printf("Samples overlap, giving up (%d, %d)\n",i,j);
                exit(1);
            }
#endif
            ds += nj;
            // start offset in samples
            if (start & mask) continue;
#if 0
            {
                printf("Start position %d not aligned, giving up\n",start);
                exit(1);
            }
#endif
            if (compression == 2 && (nj & 1)) continue;
#if 0            
            {
                printf("Duplicator length odd, giving up\n");
                exit(1);
            }
#endif
            duplicador[j].origin = i;
            duplicador[j].start_offset = start >> compression;
            duplicador[j].size = (nj + 2) >> compression;
            duplicador[j].frames = n;
            duplicador[j].flags = FDUP_DUPS;
            break;
        }
    }
}

uint32_t outwave_pos;

int wvalign4(FILE *f)
{
    static char pad[16]={0};
    int pos = ftell(f);
    if (pos & 3) {
        int delta = 4-(pos & 3);
        fwrite(pad, delta, 1, f);
    }
    return ftell(f);
    
}



#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
						   0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
static int search(int val,short *table,int size)
{
	int		i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

uint8_t alaw(int16_t pcm_val)
{
	int		mask;
	int		seg;
	uint8_t	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 8;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (uint8_t) (0x7F ^ mask);
	else {
		aval = (uint8_t) (seg << SEG_SHIFT);
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (uint8_t)(aval ^ mask);
	}
    
}

uint8_t *ubuf=NULL;
int ubuf_size=0;
void convert_a(int16_t *in, int n)
{
    if (!ubuf) ubuf= malloc(ubuf_size = 2*n);
    else if (ubuf_size < n) {
        ubuf=realloc(ubuf, ubuf_size = 2*n);
    }
    int i;
    for (i=0;i<n;i++) ubuf[i]=alaw(in[i]);
}

int wavebyte=0;

void putWaveBytes(uint8_t *data, int nframes, int framesize, FILE *f)
{
    if (!mkh) {
        fwrite(data, nframes, framesize, f);
    }
    else {
        int i;
        for (i=0; i<nframes * framesize; i++,wavebyte++) {
            if (wavebyte) fprintf(f,",");
            if (!(wavebyte&15)) fprintf(f,"\n");
            fprintf(f,"%d", data[i]);
        }
    }
}



void writeBlobFrames(FILE *f, uint32_t offset, int nframes)
{
    int i;
    uint16_t *data=malloc(MBRPeriod * nframes * 2);
    fseek(dbfile, RawOffset + offset, SEEK_SET);
    //printf("Reading %d at offset %d\n", MBRPeriod * 2 * nframes, RawOffset + offset);
    freadp(data, nframes, MBRPeriod * 2);
    if (!compression) {
        putWaveBytes((uint8_t *)data, nframes, MBRPeriod *2, f);
        return;
    }
    
    if (compression == 1) {
        convert_a(data, nframes * MBRPeriod);
        putWaveBytes((uint8_t *)ubuf, nframes, MBRPeriod, f);
        return;
    }
    for (i=0; i< nframes * MBRPeriod; i+=2) {
        //half[i/2] = ((int32_t)data[i] + (int32_t)data[i+1]) / 2;
        data[i/2] = data[i];
    }
    convert_a(data, nframes * MBRPeriod / 2);
    putWaveBytes((uint8_t *)ubuf, nframes, MBRPeriod/2, f);
    free(data);
}
const char *autogen=R"EE(/*
 * This file is autogenerated from %s database
 * Do not edit manually!
 */
)EE";


static char *helpme(char *name)
{
    fprintf(stderr,"Usage: %s [-O dirname] [-a|-A|-h|-H] <rom file> [output prefix]\n", name);
    fprintf(stderr,R"EE( -a : binary blob, A-law compressed
 -A : binary blob, downsampled, compressed (low quality)
 -h : A-law compressed into .h file (only for small voice files)
 -H : downsampled, compressed into .h file (low quality)
 Without modifier: binary blob, not compressed
 -O <name>: name of data directory for output files\n"
            Subdirectory containing output files will be created here
 )EE");
    exit(1);
}

#define poparg ((argc>0)?(argc--,*argv++):helpme(cmdname))
int getCmdParams(int argc, char **argv)
{
    char *cmdname = poparg, *c;
    for (outdirname = NULL;;) {
        c=poparg;
        if (*c=='-') {
            if (!c[1]) break;
            switch(c[1]) {
                case 'a':
                    compression = 1;
                    mkh = 0;
                    break;
                case 'A':
                    compression = 2;
                    mkh = 0;
                    break;
                case 'h':
                    compression = 1;
                    mkh = 1;
                    break;
                case 'H':
                    compression = 2;
                    mkh = 1;
                    break;
                case 'O':
                    if (c[2]) outdirname = c+2;
                    else outdirname = poparg;
                    break;
                default:
                    helpme(cmdname);
            }
        }
        else break;
    }
    dbfname = strdup(c);
    dbname = strrchr(c,'/');
    if (dbname) dbname++;
    else dbname = c;
    if (strlen(dbname) != 3 ||
        !isalpha(dbname[0]) ||
        !isalpha(dbname[1]) ||
        !isdigit(dbname[2])) {
            fprintf(stderr,"Bad database name: %s\n", dbname);
            exit(1);
    }
    char buf[256];
    
    static const char *cpt[]={"full","alaw","low"};
    if (outdirname && *outdirname) {
        strcpy(buf,outdirname);
        c=strrchr(buf,'/');
        if (!c || c[1]) {
            strcat(buf,"/");
        }
    }
    else {
        buf[0]=0;
    }
    sprintf(buf + strlen(buf), "%s_%s", dbname, cpt[compression]);
    if (mkh) strcat(buf, "_app");
    strcat(buf,"/");
    mkdir(buf);
    strcat(buf,"espola");
    outdirname = strdup(buf);    
}
                
int main(int argc, char *argv[])
{
    int i,j;
    // probably getopt is safe here, but...
    getCmdParams(argc, argv);
    openDB();
    readIndex();
    createPhonemeList();
    qsort(newtab, nb_diphone, sizeof(*newtab), hashcomp);
    find_dups();
    FILE *fout, *dfout,*wvout;
    char buf[256];
    sprintf(buf,"%s_header.h",outdirname);
    fout=fopen(buf,"w");
    if (!fout) {
        perror(buf);exit(1);
    }
    sprintf(buf,"%s_data.h",outdirname);
    dfout=fopen(buf,"w");
    if (!dfout) {
        perror(buf);exit(1);
    }
    sprintf(buf,"%s.blob",outdirname);
    if (!mkh) {
        wvout=fopen(buf,"wb");
        if (!wvout) {
            perror(buf);exit(1);
        }
    }

    
    fprintf(fout, "#ifndef FLAMBROLA_H\n#define FLAMBROLA_H 1\n\n");
    fprintf(dfout, "#ifndef FLAMBROLAD_H\n#define FLAMBROLAD_H 1\n\n");
    fprintf(fout,autogen,dbname);
    fprintf(dfout,autogen,dbname);
    sprintf(buf,"%c%s","MaA"[compression], dbname);
    fprintf(dfout,"#define BLOB_ID \"%s\"\n",buf);
    if (!mkh) fwrite(buf,4,1,wvout);
    else {
        fprintf(dfout,"#define HAVE_WAVEBLOB 1\n");
        fprintf(dfout,"static const uint8_t waveblob[]={");
    }

    int n;

    for (i=j=0; i< nb_diphone;i++) {
        if (duplicador[i].flags & FDUP_DUPS) continue;
        uint32_t offset = newtab[i].pos_wave*2;
        uint32_t filepos = mkh?wavebyte:wvalign4(wvout);
//#define pmrk_DS(INDEX) ((p_pmrk[ ( (INDEX-1)+p_pmrk_offset)/ 4 ] >> (  2*( ((INDEX-1) + p_pmrk_offset)%4))) & 0x3)
        n=computeRealFrames(&newtab[i]);
        j += n;
        writeBlobFrames(mkh?dfout:wvout,offset,n);
        newtab[i].newposwave=filepos;
        duplicador[i].flags |= FDUP_SAVED;
    }
    // duplicates
    int nr,k;
    for (k=0;;k++) {
        for (i=nr=0; i< nb_diphone; i++) {
            if (!(duplicador[i].flags & FDUP_DUPS)) continue;
            int origin = duplicador[i].origin;
            if (!(duplicador[origin].flags & FDUP_SAVED)) continue;
            newtab[i].newposwave = newtab[origin].newposwave + duplicador[i].start_offset;
            duplicador[i].flags = FDUP_SAVED;
            nr++;
        }
        if (!nr) break;
    }
    if (mkh) {
        fprintf(dfout,"};\n");
    }
    else {
        wvalign4(wvout);
        fclose(wvout);
    }
    fprintf(fout,"#define COMPRESSION_TYPE %d\n", compression);
    fprintf(fout,"#define PARAHASH_COUNT %d\n", nb_diphone);
    fprintf(dfout,"static DiphoneInfo parahash[]={\n");
    for (i=0;i<nb_diphone;i++) {
        if (i) fprintf(dfout,",\n");
        fprintf(dfout,"  {0x%02x, 0x%02x, %d, %d, %d, %d}",
            newtab[i].left, newtab[i].right,
            newtab[i].newposwave,//newtab[i].content.pos_wave, 
            newtab[i].halfseg,
            newtab[i].pos_pm,
            newtab[i].nb_frame);
    }
    fprintf(dfout,"};\n\n");
    fprintf(dfout,"static const uint8_t FrameTypeT[%d]={\n  ", round_size);
    for (i=0; i<round_size;i++) {
        if (i) fprintf(dfout,",");
        if (i && !(i&15)) fprintf(dfout,"\n  ");
        
        fprintf(dfout,"0x%02X",pmrk[i]);
    }
    fprintf(dfout,"};\n\n");
    fprintf(dfout, "static const char *sil_phone = \"%s\";\n", sil_phon);
    fprintf(fout, "#define MBRPeriod %d\n", MBRPeriod);
    fprintf(fout, "#define MBRFreq %d\n", Freq);
    fprintf(fout, "#define nb_diphone %d\n", PhoneListNo);
    fprintf(fout, "#define max_frame %d\n", max_frame);
    fprintf(fout, "#define max_samples %d\n", max_frame * MBRPeriod);
    fprintf(dfout, "static const int PhoneNameCount = %d;\n",PhoneListNo);
    fprintf(dfout, "static const char *const PhoneNames[]={\n");
    for (i=0;i<PhoneListNo;i++) {
        if (i) fprintf(dfout,",\n");
        fprintf(dfout,"\"%s\"", phoneList[i]);
    }
    fprintf(dfout,"};\n\n");
        

    if (nb_diphone != PhoneListNo * PhoneListNo) {
        fprintf(fout,"#define INCOMPLETE_DIPHONES 1\n");
    }
    fprintf(fout, "#endif\n\n");
    fprintf(dfout, "#endif\n\n");
    printf("Done\n");

    
}

    
