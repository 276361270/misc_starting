/*****************************************************************************
 *		                    
 *  Application notes :
 *		                    
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "xvid.h"

/*****************************************************************************
 *               Global vars in module and constants
 ****************************************************************************/

#define USE_PNM 0
#define USE_TGA 1

static int XDIM = 0;
static int YDIM = 0;
static int ARG_SAVEDECOUTPUT = 0;
static int ARG_SAVEMPEGSTREAM = 0;
static char *ARG_INPUTFILE = NULL;
static char *ARG_OUTPUTFILE = NULL;
static int CSP = XVID_CSP_I420;
static int BPP = 1;
static int FORMAT = USE_PNM;

static char filepath[256] = "./";
static void *dec_handle = NULL;

#define BUFFER_SIZE (2*1024*1024)

static const int display_buffer_bytes = 0;

#define MIN_USEFUL_BYTES 1

/*****************************************************************************
 *               Local prototypes
 ****************************************************************************/

static double msecond();
static int dec_init(int use_assembler, int debug_level);
static int dec_main(unsigned char *istream,
					unsigned char *ostream,
					int istream_size,
					xvid_dec_stats_t *xvid_dec_stats);
static int dec_stop();
static void usage();
//static int write_image(char *prefix, unsigned char *image);
//static int write_pnm(char *filename, unsigned char *image);
///static int write_tga(char *filename, unsigned char *image);

const char * type2str(int type)
{
    if (type==XVID_TYPE_IVOP)
        return "I";
    if (type==XVID_TYPE_PVOP)
        return "P";
    if (type==XVID_TYPE_BVOP)
        return "B";
    return "S";
}

/*****************************************************************************
 *        Main program
 ****************************************************************************/
//-i e:\\yuv\\4cif.yuv -o e:\\bitmp4\\enc_mp4.mp4 -w 704 -h 576 -key_interval 100 -frames 1000 -bitrate 1536000
int main(int argc, char *argv[])
{
	unsigned char *mp4_buffer = NULL;
	unsigned char *mp4_ptr    = NULL;
	unsigned char *out_buffer = NULL;
	int useful_bytes;
	int chunk;
	xvid_dec_stats_t xvid_dec_stats;
	
	double totaldectime;
  
	long totalsize;
	int status;
  
	int use_assembler = 1;
	int debug_level = 0;
  
	char filename[256];
  
	FILE *in_file,*out_file;
	int filenr;
	int i;

	
	printf("\n xvid dncoder\n");
	printf("\n Is Dncoding Image, Plese wait...\n");
/*****************************************************************************
 * Command line parsing
 ****************************************************************************/

	for (i=1; i< argc; i++) {
 
		if (strcmp("-asm", argv[i]) == 0 ) {
			use_assembler = 1;
		} else if (strcmp("-i", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_INPUTFILE = argv[i];
		} else if (strcmp("-m", argv[i]) == 0) {
			ARG_SAVEMPEGSTREAM = 1;
		} else if (strcmp("-o", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_OUTPUTFILE = argv[i];
		} else {
			usage();
			exit(-1);
		}
	}
  
/*****************************************************************************
 * Values checking
 ****************************************************************************/

	if ( ARG_INPUTFILE == NULL || strcmp(ARG_INPUTFILE, "stdin") == 0) {
		in_file = stdin;
	}
	else {

		in_file = fopen(ARG_INPUTFILE, "rb");
		if (in_file == NULL) {
			fprintf(stderr, "Error opening input file %s\n", ARG_INPUTFILE);
			return(-1);
		}
	}

	if (ARG_OUTPUTFILE != NULL)
	{
		out_file = fopen(ARG_OUTPUTFILE,"wb");
		if (!out_file)
			exit(-1);
	}


	/* PNM/PGM format can't handle 16/32 bit data */
	if (BPP != 1 && BPP != 3 && FORMAT == USE_PNM) {
		FORMAT = USE_TGA;
	}

/*****************************************************************************
 *        Memory allocation
 ****************************************************************************/

	/* Memory for encoded mp4 stream */
	mp4_buffer = (unsigned char *) malloc(BUFFER_SIZE);
	mp4_ptr = mp4_buffer;
	if (!mp4_buffer)
		goto free_all_memory;	
    
/*****************************************************************************
 *        xvid PART  Start
 ****************************************************************************/

	status = dec_init(use_assembler, debug_level);
	if (status) {
		fprintf(stderr,
				"Decore INIT problem, return value %d\n", status);
		goto release_all;
	}


/*****************************************************************************
 *	                         Main loop
 ****************************************************************************/

	/* Fill the buffer */
	useful_bytes = fread(mp4_buffer, 1, BUFFER_SIZE, in_file);

	totaldectime = 0;
	totalsize = 0;
	filenr = 0;
	mp4_ptr = mp4_buffer;
	chunk = 0;
	
	do {
		int used_bytes = 0;
		double dectime;

		/*
		 * If the buffer is half empty or there are no more bytes in it
		 * then fill it.
		 */
		if (mp4_ptr > mp4_buffer + BUFFER_SIZE/2) {
			int already_in_buffer = (mp4_buffer + BUFFER_SIZE - mp4_ptr);

			/* Move data if needed */
			if (already_in_buffer > 0)
				memcpy(mp4_buffer, mp4_ptr, already_in_buffer);

			/* Update mp4_ptr */
			mp4_ptr = mp4_buffer; 

			/* read new data */
            if(!feof(in_file)) {

				useful_bytes += fread(mp4_buffer + already_in_buffer,
									  1, BUFFER_SIZE - already_in_buffer,
									  in_file);
			}
		}


		/* This loop is needed to handle VOL/NVOP reading */
		do {

			/* Decode frame */
			dectime = msecond();
			used_bytes = dec_main(mp4_ptr, out_buffer, useful_bytes, &xvid_dec_stats);
			dectime = msecond() - dectime;

			/* Resize image buffer if needed */
			if(xvid_dec_stats.type == XVID_TYPE_VOL) {

				/* Check if old buffer is smaller */
				if(XDIM*YDIM < xvid_dec_stats.data.vol.width*xvid_dec_stats.data.vol.height) {

					/* Copy new witdh and new height from the vol structure */
					XDIM = xvid_dec_stats.data.vol.width;
					YDIM = xvid_dec_stats.data.vol.height;

					/* Free old output buffer*/
					if(out_buffer) free(out_buffer);

					/* Allocate the new buffer */
					out_buffer = (unsigned char*)malloc(XDIM*YDIM*4);
					if(out_buffer == NULL)
						goto free_all_memory;

					fprintf(stderr, "\n widthxheight= %dx%d\n\n", XDIM, YDIM);
				}

				/* Save individual mpeg4 stream if required */

			}

			/* Update buffer pointers */
			if(used_bytes > 0) {
				mp4_ptr += used_bytes;
				useful_bytes -= used_bytes;

				/* Total size */
				totalsize += used_bytes;
			}

			if (display_buffer_bytes) {
				printf("Data chunk %d: %d bytes consumed, %d bytes in buffer\n", chunk++, used_bytes, useful_bytes);
			}
		} while (xvid_dec_stats.type <= 0 && useful_bytes > MIN_USEFUL_BYTES);

		/* Check if there is a negative number of useful bytes left in buffer
		 * This means we went too far */
        if(useful_bytes < 0)
            break;
		
    	/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

			
		if (!display_buffer_bytes) {
			printf(" No.%4d: type = %s, | dectime(ms) =%4.1f, length(bytes) =%5d\n",
					filenr, type2str(xvid_dec_stats.type), dectime, used_bytes);
		}

		/* Save individual mpeg4 stream if required */
				
		/* Save output frame if required */

		//luhanyu
		fwrite(out_buffer,1,XDIM*YDIM*3/2,out_file);

		filenr++;

	} while (useful_bytes>MIN_USEFUL_BYTES || !feof(in_file));

	useful_bytes = 0; /* Empty buffer */

/*****************************************************************************
 *     Flush decoder buffers
 ****************************************************************************/

	do {

		/* Fake vars */
		int used_bytes;
		double dectime;

        do {
		    dectime = msecond();
		    used_bytes = dec_main(NULL, out_buffer, -1, &xvid_dec_stats);
		    dectime = msecond() - dectime;
			if (display_buffer_bytes) {
				printf("Data chunk %d: %d bytes consumed, %d bytes in buffer\n", chunk++, used_bytes, useful_bytes);
			}
        } while(used_bytes>=0 && xvid_dec_stats.type <= 0);

        if (used_bytes < 0) {   /* xvid_ERR_END */
            break;
        }

		/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

		/* Prints some decoding stats */
		if (!display_buffer_bytes) {
			printf("Frame %5d: type = %s, dectime(ms) =%6.1f, length(bytes) =%7d\n",
					filenr, type2str(xvid_dec_stats.type), dectime, used_bytes);
		}

		//luhanyu
		fwrite(out_buffer,1,XDIM*YDIM*3/2,out_file);


		filenr++;

	}while(1);
	
/*****************************************************************************
 *     Calculate totals and averages for output, print results
 ****************************************************************************/

	if (filenr>0) {
		totalsize    /= filenr;
		totaldectime /= filenr;
		printf(" Avg: dectime(ms) =%7.2f, fps =%7.2f, length(bytes) =%7d\n",
			   totaldectime, 1000/totaldectime, (int)totalsize);
	}else{
		printf("Nothing was decoded!\n");
	}
		
/*****************************************************************************
 *      xvid PART  Stop
 ****************************************************************************/

 release_all:
  	if (dec_handle) {
	  	status = dec_stop();
		if (status)    
			fprintf(stderr, "decore RELEASE problem return value %d\n", status);
	}

 free_all_memory:
	free(out_buffer);
	free(mp4_buffer);

	return(0);
}

/*****************************************************************************
 *               Usage function
 ****************************************************************************/

static void usage()
{
	fprintf(stderr, "Please look readme file!\n");
}

/*****************************************************************************
 *               "helper" functions
 ****************************************************************************/

/* return the current time in milli seconds */
static double
msecond()
{	
#ifndef WIN32
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return((double)tv.tv_sec*1.0e3 + (double)tv.tv_usec*1.0e-3);
#else
	clock_t clk;
	clk = clock();
	return(clk * 1000.0 / CLOCKS_PER_SEC);
#endif
}


/*****************************************************************************
 * Routines for decoding: init decoder, use, and stop decoder
 ****************************************************************************/

/* init decoder before first run */
static int
dec_init(int use_assembler, int debug_level)
{
	int ret;

	xvid_gbl_init_t   xvid_gbl_init;
	xvid_dec_create_t xvid_dec_create;

	/* Reset the structure with zeros */
	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init_t));
	memset(&xvid_dec_create, 0, sizeof(xvid_dec_create_t));

	/*------------------------------------------------------------------------
	 * xvid core initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_gbl_init.version = XVID_VERSION;

	/* Assembly setting */
	if(use_assembler)
#ifdef ARCH_IS_IA64
		xvid_gbl_init.cpu_flags = xvid_CPU_FORCE | xvid_CPU_IA64;
#else
	xvid_gbl_init.cpu_flags = 0;
#endif
	else
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;

	xvid_gbl_init.debug = debug_level;

	xvid_global(NULL, 0, &xvid_gbl_init, NULL);

	/*------------------------------------------------------------------------
	 * xvid encoder initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_dec_create.version = XVID_VERSION;

	/*
	 * Image dimensions -- set to 0, xvidcore will resize when ever it is
	 * needed
	 */
	xvid_dec_create.width = 0;
	xvid_dec_create.height = 0;

	ret = xvid_decore(NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL);

	dec_handle = xvid_dec_create.handle;

	return(ret);
}

/* decode one frame  */
static int
dec_main(unsigned char *istream,
		 unsigned char *ostream,
		 int istream_size,
		 xvid_dec_stats_t *xvid_dec_stats)
{

	int ret;

	xvid_dec_frame_t xvid_dec_frame;

	/* Reset all structures */
	memset(&xvid_dec_frame, 0, sizeof(xvid_dec_frame_t));
	memset(xvid_dec_stats, 0, sizeof(xvid_dec_stats_t));

	/* Set version */
	xvid_dec_frame.version = XVID_VERSION;
	xvid_dec_stats->version = XVID_VERSION;

	/* No general flags to set */
	xvid_dec_frame.general          = 0;
	//xvid_dec_frame.general |= xvid_DEBLOCKY;// | xvid_DEBLOCKUV;
	/* Input stream */
	xvid_dec_frame.bitstream        = istream;
	xvid_dec_frame.length           = istream_size;

	/* Output frame structure */
	xvid_dec_frame.output.plane[0]  = ostream;
	xvid_dec_frame.output.stride[0] = XDIM*BPP;
	xvid_dec_frame.output.csp = CSP;

	ret = xvid_decore(dec_handle, XVID_DEC_DECODE, &xvid_dec_frame, xvid_dec_stats);

	return(ret);
}

/* close decoder to release resources */
static int
dec_stop()
{
	int ret;

	ret = xvid_decore(dec_handle, XVID_DEC_DESTROY, NULL, NULL);

	return(ret);
}
