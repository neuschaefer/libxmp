/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#include "load.h"
#include "mod.h"
#include "period.h"

static int flt_test (FILE *, char *, const int);
static int flt_load (struct xmp_context *, FILE *, const int);

struct xmp_loader_info flt_loader = {
    "FLT",
    "Startrekker/Audio Sculpture",
    flt_test,
    flt_load
};

static int flt_test(FILE *f, char *t, const int start)
{
    char buf[4];

    fseek(f, start + 1080, SEEK_SET);
    if (fread(buf, 1, 4, f) < 4)
	return -1;

    /* Also RASP? */
    if (memcmp(buf, "FLT", 3) && memcmp(buf, "EXO", 3))
	return -1;

    if (buf[3] != '4' && buf[3] != '8' && buf[3] != 'M')
	return -1;

    fseek(f, start + 0, SEEK_SET);
    read_title(f, t, 28);

    return 0;
}



/* Waveforms from the Startrekker 1.2 AM synth replayer code */

static int8 am_waveform[3][32] = {
	{    0,   25,   49,   71,   90,  106,  117,  125,	/* Sine */
	   127,  125,  117,  106,   90,   71,   49,   25,
	     0,  -25,  -49,  -71,  -90, -106, -117, -125,
	  -127, -125, -117, -106,  -90,  -71,  -49,  -25
	},

	{ -128, -120, -112, -104,  -96,  -88,  -80,  -72,	/* Ramp */
	   -64,  -56,  -48,  -40,  -32,  -24,  -16,   -8,
	     0,    8,   16,   24,   32,   40,   48,   56,
	    64,   72,   80,   88,   96,  104,  112,  120
	},

	{ -128, -128, -128, -128, -128, -128, -128, -128,	/* Square */
	  -128, -128, -128, -128, -128, -128, -128, -128,
	   127,  127,  127,  127,  127,  127,  127,  127,
	   127,  127,  127,  127,  127,  127,  127,  127
	},
};

static int8 am_noise[1024];


struct am_instrument {
	int16 l0;		/* start amplitude */
	int16 a1l;		/* attack level */
	int16 a1s;		/* attack speed */
	int16 a2l;		/* secondary attack level */
	int16 a2s;		/* secondary attack speed */
	int16 sl;		/* sustain level */
	int16 ds;		/* decay speed */
	int16 st;		/* sustain time */
	int16 rs;		/* release speed */
	int16 wf;		/* waveform */
	int16 p_fall;		/* ? */
	int16 v_amp;		/* vibrato amplitude */
	int16 v_spd;		/* vibrato speed */
	int16 fq;		/* base frequency */
};



static int is_am_instrument(FILE *nt, int i)
{
    char buf[2];
    int16 wf;

    fseek(nt, 144 + i * 120, SEEK_SET);
    if (fread(buf, 1, 2, nt) < 2)
	return 0;
    if (memcmp(buf, "AM", 2))
	return 0;

    fseek(nt, 24, SEEK_CUR);
    wf = read16b(nt);
    if (wf < 0 || wf > 3)
	return 0;

    return 1;
}

static void read_am_instrument(struct xmp_context *ctx, FILE *nt, int i)
{
    struct xmp_mod_context *m = &ctx->m;
    struct xmp_module *mod = &m->mod;
    struct am_instrument am;
    char *wave;
    int a, b;

    fseek(nt, 144 + i * 120 + 2 + 4, SEEK_SET);
    am.l0 = read16b(nt);
    am.a1l = read16b(nt);
    am.a1s = read16b(nt);
    am.a2l = read16b(nt);
    am.a2s = read16b(nt);
    am.sl = read16b(nt);
    am.ds = read16b(nt);
    am.st = read16b(nt);
    read16b(nt);
    am.rs = read16b(nt);
    am.wf = read16b(nt);
    am.p_fall = -(int16)read16b(nt);
    am.v_amp = read16b(nt);
    am.v_spd = read16b(nt);
    am.fq = read16b(nt);

#if 0
printf("L0=%d A1L=%d A1S=%d A2L=%d A2S=%d SL=%d DS=%d ST=%d RS=%d WF=%d\n",
am.l0, am.a1l, am.a1s, am.a2l, am.a2s, am.sl, am.ds, am.st, am.rs, am.wf);
#endif

    if (am.wf < 3) {
	mod->xxs[i].len = 32;
	mod->xxs[i].lps = 0;
	mod->xxs[i].lpe = 32;
	wave = (char *)&am_waveform[am.wf][0];
    } else {
	int j;

	mod->xxs[i].len = 1024;
	mod->xxs[i].lps = 0;
	mod->xxs[i].lpe = 1024;

	for (j = 0; j < 1024; j++)
	    am_noise[j] = rand() % 256;

	wave = (char *)&am_noise[0];
    }

    mod->xxs[i].flg = XMP_SAMPLE_LOOP;
    mod->xxi[i].sub[0].vol = 0x40;		/* prelude.mod has 0 in instrument */
    mod->xxi[i].nsm = 1;
    mod->xxi[i].sub[0].xpo = -12 * am.fq;
    mod->xxi[i].sub[0].vwf = 0;
    mod->xxi[i].sub[0].vde = am.v_amp;
    mod->xxi[i].sub[0].vra = am.v_spd;

    /*
     * AM synth envelope parameters based on the Startrekker 1.2 docs
     *
     * L0    Start amplitude for the envelope
     * A1L   Attack level
     * A1S   The speed that the amplitude changes to the attack level, $1
     *       is slow and $40 is fast.
     * A2L   Secondary attack level, for those who likes envelopes...
     * A2S   Secondary attack speed.
     * DS    The speed that the amplitude decays down to the:
     * SL    Sustain level. There is remains for the time set by the
     * ST    Sustain time.
     * RS    Release speed. The speed that the amplitude falls from ST to 0.
     */
    if (am.a1s == 0) am.a1s = 1;
    if (am.a2s == 0) am.a2s = 1;
    if (am.ds  == 0) am.ds  = 1;
    if (am.rs  == 0) am.rs  = 1;

    mod->xxi[i].aei.npt = 6;
    mod->xxi[i].aei.flg = XMP_ENVELOPE_ON;

    mod->xxi[i].aei.data[0] = 0;
    mod->xxi[i].aei.data[1] = am.l0 / 4;

    /*
     * Startrekker increments/decrements the envelope by the stage speed
     * until it reaches the next stage level.
     *
     *         ^ 
     *         |
     *     100 +.........o
     *         |        /:
     *     A2L +.......o :        x = 256 * (A2L - A1L) / (256 - A1L)
     *         |      /: :
     *         |     / : :
     *     A1L +....o..:.:
     *         |    :  : :
     *         |    :x : :
     *         +----+--+-+----->
     *              |    |
     *              |256/|
     *               A2S
     */

    if (am.a1l > am.l0) {
	a = am.a1l - am.l0;
	b = 256 - am.l0;
    } else {
	a = am.l0 - am.a1l;
	b = am.l0;
    }
    if (b == 0) b = 1;

    mod->xxi[i].aei.data[2] = mod->xxi[i].aei.data[0] + (256 * a) / (am.a1s * b);

    mod->xxi[i].aei.data[3] = am.a1l / 4;

    if (am.a2l > am.a1l) {
	a = am.a2l - am.a1l;
	b = 256 - am.a1l;
    } else {
	a = am.a1l - am.a2l;
	b = am.a1l;
    }
    if (b == 0) b = 1;

    mod->xxi[i].aei.data[4] = mod->xxi[i].aei.data[2] + (256 * a) / (am.a2s * b);

    mod->xxi[i].aei.data[5] = am.a2l / 4;

    if (am.sl > am.a2l) {
	a = am.sl - am.a2l;
	b = 256 - am.a2l;
    } else {
	a = am.a2l - am.sl;
	b = am.a2l;
    }
    if (b == 0) b = 1;

    mod->xxi[i].aei.data[6] = mod->xxi[i].aei.data[4] + (256 * a) / (am.ds * b);

    mod->xxi[i].aei.data[7] = am.sl / 4;
    mod->xxi[i].aei.data[8] = mod->xxi[i].aei.data[6] + am.st;
    mod->xxi[i].aei.data[9] = am.sl / 4;
    mod->xxi[i].aei.data[10] = mod->xxi[i].aei.data[8] + (256 / am.rs);
    mod->xxi[i].aei.data[11] = 0;

    /*
     * Implement P.FALL using pitch envelope
     */

    if (am.p_fall) {
	mod->xxi[i].fei.npt = 2;
	mod->xxi[i].fei.flg = XMP_ENVELOPE_ON;

	mod->xxi[i].fei.data[0] = 0;
	mod->xxi[i].fei.data[1] = 0;

	mod->xxi[i].fei.data[2] = 1024 / abs(am.p_fall);
	mod->xxi[i].fei.data[3] = 10 * (am.p_fall < 0 ? -256 : 256);
    }

    load_sample(ctx, NULL, mod->xxi[i].sub[0].sid, SAMPLE_FLAG_NOLOAD,
					&mod->xxs[mod->xxi[i].sub[0].sid], wave);
}


static int flt_load(struct xmp_context *ctx, FILE *f, const int start)
{
    struct xmp_mod_context *m = &ctx->m;
    struct xmp_module *mod = &m->mod;
    int i, j;
    struct xmp_event *event;
    struct mod_header mh;
    uint8 mod_event[4];
    char *tracker;
    char filename[1024];
    char buf[16];
    FILE *nt;
    int am_synth;

    LOAD_INIT();

    /* See if we have the synth parameters file */
    am_synth = 0;
    snprintf(filename, 1024, "%s%s.NT", m->dirname, m->basename);
    if ((nt = fopen(filename, "rb")) == NULL) {
	snprintf(filename, 1024, "%s%s.nt", m->dirname, m->basename);
	if ((nt = fopen(filename, "rb")) == NULL) {
	    snprintf(filename, 1024, "%s%s.AS", m->dirname, m->basename);
	    if ((nt = fopen(filename, "rb")) == NULL) {
	        snprintf(filename, 1024, "%s%s.as", m->dirname, m->basename);
	        nt = fopen(filename, "rb");
	    }
	}
    }

    tracker = "Startrekker";

    if (nt) {
	fread(buf, 1, 16, nt);
	if (memcmp(buf, "ST1.2 ModuleINFO", 16) == 0) {
	    am_synth = 1;
	    tracker = "Startrekker 1.2";
	} else if (memcmp(buf, "ST1.3 ModuleINFO", 16) == 0) {
	    am_synth = 1;
	    tracker = "Startrekker 1.3";
	} else if (memcmp(buf, "AudioSculpture10", 16) == 0) {
	    am_synth = 1;
	    tracker = "AudioSculpture 1.0";
	}
    }

    fread(&mh.name, 20, 1, f);
    for (i = 0; i < 31; i++) {
	fread(&mh.ins[i].name, 22, 1, f);	/* Instrument name */
	mh.ins[i].size = read16b(f);		/* Length in 16-bit words */
	mh.ins[i].finetune = read8(f);		/* Finetune (signed nibble) */
	mh.ins[i].volume = read8(f);		/* Linear playback volume */
	mh.ins[i].loop_start = read16b(f);	/* Loop start in 16-bit words */
	mh.ins[i].loop_size = read16b(f);	/* Loop size in 16-bit words */
    }
    mh.len = read8(f);
    mh.restart = read8(f);
    fread(&mh.order, 128, 1, f);
    fread(&mh.magic, 4, 1, f);

    if (mh.magic[3] == '4')
	mod->chn = 4;
    else
	mod->chn = 8;

    mod->ins = 31;
    mod->smp = mod->ins;
    mod->len = mh.len;
    mod->rst = mh.restart;
    memcpy(mod->xxo, mh.order, 128);

    for (i = 0; i < 128; i++) {
	if (mod->chn > 4)
	    mod->xxo[i] >>= 1;
	if (mod->xxo[i] > mod->pat)
	    mod->pat = mod->xxo[i];
    }

    mod->pat++;

    mod->trk = mod->chn * mod->pat;

    strncpy(mod->name, (char *) mh.name, 20);
    set_type(m, "%s %4.4s", tracker, mh.magic);
    MODULE_INFO();

    INSTRUMENT_INIT();

    for (i = 0; i < mod->ins; i++) {
	mod->xxi[i].sub = calloc(sizeof (struct xmp_subinstrument), 1);
	mod->xxs[i].len = 2 * mh.ins[i].size;
	mod->xxs[i].lps = 2 * mh.ins[i].loop_start;
	mod->xxs[i].lpe = mod->xxs[i].lps + 2 * mh.ins[i].loop_size;
	mod->xxs[i].flg = mh.ins[i].loop_size > 1 ? XMP_SAMPLE_LOOP : 0;
	mod->xxi[i].sub[0].fin = (int8)(mh.ins[i].finetune << 4);
	mod->xxi[i].sub[0].vol = mh.ins[i].volume;
	mod->xxi[i].sub[0].pan = 0x80;
	mod->xxi[i].sub[0].sid = i;
	mod->xxi[i].nsm = !!(mod->xxs[i].len);
	mod->xxi[i].rls = 0xfff;

	copy_adjust(mod->xxi[i].name, mh.ins[i].name, 22);

	if (am_synth && is_am_instrument(nt, i)) {
	    _D(_D_INFO "[%2X] %-22.22s SYNT ---- ----   V40 %+d",
			i, mod->xxi[i].name, mod->xxi[i].sub[0].fin >> 4);
	} else if (*mod->xxi[i].name || mod->xxs[i].len > 2) {
	    _D(_D_INFO "[%2X] %-22.22s %04x %04x %04x %c V%02x %+d",
			i, mod->xxi[i].name, mod->xxs[i].len, mod->xxs[i].lps,
			mod->xxs[i].lpe, mh.ins[i].loop_size > 1 ? 'L' : ' ',
			mod->xxi[i].sub[0].vol, mod->xxi[i].sub[0].fin >> 4);
	}
    }

    PATTERN_INIT();

    /* Load and convert patterns */
    _D(_D_INFO "Stored patterns: %d", mod->pat);

    /* The format you are looking for is FLT8, and the ONLY two differences
     * are: It says FLT8 instead of FLT4 or M.K., AND, the patterns are PAIRED.
     * I thought this was the easiest 8 track format possible, since it can be
     * loaded in a normal 4 channel tracker if you should want to rip sounds or
     * patterns. So, in a 8 track FLT8 module, patterns 00 and 01 is "really"
     * pattern 00. Patterns 02 and 03 together is "really" pattern 01. Thats
     * it. Oh well, I didnt have the time to implement all effect commands
     * either, so some FLT8 modules would play back badly (I think especially
     * the portamento command uses a different "scale" than the normal
     * portamento command, that would be hard to patch).
     */
    for (i = 0; i < mod->pat; i++) {
	PATTERN_ALLOC(i);
	mod->xxp[i]->rows = 64;
	TRACK_ALLOC(i);
	for (j = 0; j < (64 * 4); j++) {
	    event = &EVENT(i, j % 4, j / 4);
	    fread(mod_event, 1, 4, f);
	    cvt_pt_event(event, mod_event);
	}
	if (mod->chn > 4) {
	    for (j = 0; j < (64 * 4); j++) {
		event = &EVENT(i, (j % 4) + 4, j / 4);
		fread(mod_event, 1, 4, f);
		cvt_pt_event(event, mod_event);

		/* no macros */
		if (event->fxt == 0x0e)
		   event->fxt = event->fxp = 0;
	    }
	}
    }

    /* no such limit for synth instruments
     * mod->flg |= XXM_FLG_MODRNG;
     */

    /* Load samples */

    _D(_D_INFO "Stored samples: %d", mod->smp);

    for (i = 0; i < mod->smp; i++) {
	if (mod->xxs[i].len == 0) {
	    if (am_synth && is_am_instrument(nt, i)) {
		read_am_instrument(ctx, nt, i);
	    }
	    continue;
	}
	load_sample(ctx, f, mod->xxi[i].sub[0].sid, SAMPLE_FLAG_FULLREP,
					&mod->xxs[mod->xxi[i].sub[0].sid], NULL);
    }

    if (nt)
	fclose(nt);

    return 0;
}
