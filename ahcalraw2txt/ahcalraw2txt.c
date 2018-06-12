#include <string.h>
#include <stdlib.h>
#include <argp.h> /*parsing arguments*/
const int C_ROC_READ_LIMIT = 50000000;/*for debugging - reading only few RO cycles*/
const int C_MAX_BIF_EVENTS = 50000000;
//const int C_MAX_ROCYCLES = 1000;

const int C_MEM_CELLS = 16;

/* Program documentation. */
static char doc[] = "Converts the ahcal raw files to a text file. For more info try --help";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options will be parsed. */
static struct argp_option options[] =
      {
            { "spiroc_raw_file", 'w', "SPIROC_RAW_FILE", 0, "filename/path of the Spiroc RAW file saved by EUDAQ" },
            { "bif_raw_file", 'b', "BIF_FILE", 0, "filename/path of the BIF raw data file" },
            { "triggered_only", 'g', 0, 0, "print only hits, that were validated externally (via bif/ahcal)" },
            { "triggered_reject", 'k', 0, 0, "print only hits, that were NOT validated externally (via bif/ahcal)" },
            { "asic", 'a', "ASIC_NUMBER", 0, "asic number (must be < 256)" },
            { "channel", 'c', "CHANNEL_NUMBER", 0, "channel number to be correlated" },
            { "memcell", 'm', "CELL_NUM", 0, "memory cell to only take into account (0..15)" },
            { "from_memcell", 259, "CELL_NUM", 0, "from which memory cell (including) should be the data processed (0..15)" },
            { "to_memcell", 260, "MEMCELL_NUMBER", 0, "to which memory cell (including) should be the data processed (0..15)" },
            { "start_position", 't', "START_POSITION", 0,"Start position in the BIF data (corresponds to the event counter of the first start acq channel). When no number is given, first data entry is considered as a first readout cycle" },
            { "correlation_shift", 'r', "RELATIVE_TIMESTAMP", 0, "Correlation timestamp, which synchronizes BXIDs between BIF and SPIROC. Default:13448" },
            { "from_bxid", 261, "MIN_BXID", 0, "BXIDs smaller than MIN_BXID will be ignored. Default:1" },
            { "to_bxid", 262, "MAX_BXID", 0, "BXIDs greater than MAX_BXID will be ignored. Default:4095" },
            { "bxid_length", 'l', "LENGTH", 0, "length of the BXID in BIF tics (to convert from ns: multiply by 1.28)" },
            { "require_hitbit", 'h', 0, 0, "Filter only the hitbit==1 data" },
            { "reject_hitbit", 'e', 0, 0, "Filter only the hitbit==0 data" },
            { "require_gainbit", 257, 0, 0, "Filter only the gainbit==1 data (high gain)" },
            { "reject_gainbit", 258, 0, 0, "Filter only the gainbit==0 data (low gain)" },
            { "even_bxid_only", 263, 0, 0, "Filter only even bxids(0,2,4,...)" },
            { "odd_bxid_only", 264, 0, 0, "Filter only odd bxids (1,3,...)" },
            { "reject_gainbit", 258, 0, 0, "Filter only the gainbit==0 data (low gain)" },
            { "dif_id", 267, "NUM", 0, "use only specified DIF-ID number" },
            { "lda_port", 268, "NUM", 0, "use only LDA port" },
            { "report_EOR", 269,"LEVEL", 0, "report of EOR packets [0=off,1=summary,2=details" },
            { "histogram", 'i', 0, 0, "Print histogram instead of events" },
            { "rebin", 'n', "BINNING", 0, "histogram will be rebinned" },
            { "from_trigger_time", 265, "TDC_BIN", 0, "minimal external external trigger time (mind the bxid length" },
            { "from_cycle", 270, "ROC", 0, "minimal readout cycle" },
            { "to_cycle", 271, "ROC", 0, "maximal readout cycle" },
            { "to_trigger_time", 266, "TDC_BIN", 0, "maximal external trigger time (mind the bxid length" },           
            { "run_number", 'u', "RUN_NUMBER", 0, "Run number used for the prints" },
            { "empty_bxid_only", 'y',0, 0, "uses only BXID, which doesn't have any hitbit. require_hitbit is not allowed" },
            
            { 0 } };

/* Used by main to communicate with parse_opt. */
struct arguments_t {
   char *spiroc_raw_filename;
   char *bif_filename;
   int asic;
   int channel;
   int memcell;
   int from_memcell;
   int to_memcell;
   int start_position;
   int correlation_shift;
   int from_bxid;
   int to_bxid;
   int bxid_length;
   int hitbit_only;
   int hitbit_reject;
   int require_gainbit;
   int reject_gainbit;
   int run_number;
   int triggered_only;
   int triggered_reject;
   int histogram;
   int binning;
   int empty_bxid;
   int even_bxid_only;
   int odd_bxid_only;
   int from_trigger_time;
   int to_trigger_time;
   int dif_id;
   int lda_port;
   int report_EOR;
   int from_cycle;
   int to_cycle;
};
struct arguments_t arguments;

void arguments_init(struct arguments_t* arguments) {
   /* Default values. */
   arguments->spiroc_raw_filename = NULL;
   arguments->bif_filename = NULL;
   arguments->asic = -1;
   arguments->channel = -1;
   arguments->memcell = -1;
   arguments->from_memcell = 0;
   arguments->to_memcell = 15;
   arguments->start_position = -1;
   arguments->correlation_shift = 2121;
   arguments->hitbit_only = 0;
   arguments->hitbit_reject = 0;
   arguments->from_bxid = 1;/*by default ship BXID 0*/
   arguments->to_bxid = 4095;
   arguments->bxid_length = 160;
   arguments->run_number = -1;
   arguments->triggered_only = 0;
   arguments->triggered_reject = 0;
   arguments->histogram = 0;
   arguments->binning = 1;
   arguments->empty_bxid = 0;
   arguments->reject_gainbit = 0;
   arguments->require_gainbit = 0;
   arguments->even_bxid_only = 0;
   arguments->odd_bxid_only = 0;
   arguments->from_trigger_time = 0;
   arguments->to_trigger_time= 1000000;
   arguments->dif_id = -1;
   arguments->lda_port = -1;
   arguments->report_EOR = 0;
   arguments->from_cycle = 0;
   arguments->to_cycle = 0x7FFFFFFF; /* max_int */
   
}

void arguments_print(struct arguments_t* arguments) {
   printf("# --- PROGRAM PARAMETERS ---");
   printf("#run number=%d\n", arguments->run_number);
   printf("#spiroc_raw_file=\"%s\"\n", arguments->spiroc_raw_filename);
   printf("#bif_raw_file=\"%s\"\n", arguments->bif_filename);
   printf("#asic=%d\n", arguments->asic);
   printf("#channel=%d\n", arguments->channel);
   printf("#memcell=%d\n", arguments->memcell);
   printf("#from_memcell=%d\n", arguments->from_memcell);
   printf("#to_memcell=%d\n", arguments->to_memcell);
   printf("#start_position=%d\n", arguments->start_position);
   printf("#correlation_shift=%d\n", arguments->correlation_shift);
   printf("#require_hitbit=%d\n", arguments->hitbit_only);
   printf("#reject_hitbit=%d\n", arguments->hitbit_reject);
   printf("#bxid_length=%d\n", arguments->bxid_length);
   printf("#from_bxid=%d\n", arguments->from_bxid);
   printf("#to_bxid=%d\n", arguments->to_bxid);
   printf("#triggered_only=%d\n", arguments->triggered_only);
   printf("#triggered_reject=%d\n", arguments->triggered_reject);
   printf("#histogram=%d\n", arguments->histogram);
   printf("#binning=%d\n",arguments->binning);
   printf("#empty_bxid_only=%d\n",arguments->empty_bxid);
   printf("#even_bxid_only=%d\n",arguments->even_bxid_only);
   printf("#odd_bxid_only=%d\n",arguments->odd_bxid_only);
   printf("#reject_gainbit=%d\n",arguments->reject_gainbit);
   printf("#require_gainbit=%d\n",arguments->require_gainbit);
   printf("#from_trigger_time=%d\n", arguments->from_trigger_time);
   printf("#to_trigger_time=%d\n", arguments->to_trigger_time);
   printf("#dif_id=%d\n",arguments->dif_id);
   printf("#lda_port=%d\n",arguments->lda_port);
   printf("#report_EOR=%d\n",arguments->report_EOR);   
   printf("#from_cycle=%d\n",arguments->from_cycle);
   printf("#to_cycle=%d\n",arguments->to_cycle);   
   printf("# --- END PROGRAM PARAMETERS ---\n");
}

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
   /* Get the input argument from argp_parse, which we
    know is a pointer to our arguments structure. */
   struct arguments_t *arguments = state->input;

   switch (key) {
      case 'a':
         arguments->asic = atoi(arg);
         break;
      case 'b':
         arguments->bif_filename = arg;
         break;
      case 'c':
         arguments->channel = atoi(arg);
         break;
      case 'e':
         arguments->hitbit_reject = 1;
         break;
      case 'g':
         arguments->triggered_only = 1;
         break;
      case 'h':
         arguments->hitbit_only = 1;
         break;
      case 'i':
         arguments->histogram = 1;
         break;
      case 261:
         arguments->from_bxid = atoi(arg);
         break;
      case 262:
         arguments->to_bxid = atoi(arg);
         break;
      case 263:
         arguments->even_bxid_only = 1;
         break;
      case 264:
         arguments->odd_bxid_only = 1;
         break;
      case 'k':
         arguments->triggered_reject = 1;
         break;
      case 'l':
         arguments->bxid_length = atoi(arg);
         break;
      case 'm':
         arguments->memcell = atoi(arg);
         break;
      case 259:
         arguments->from_memcell = atoi(arg);
         break;
      case 260:
         arguments->to_memcell = atoi(arg);
         break;
      case 'n':
	 arguments->binning = atoi(arg);
	 break;
      case 'r':
         arguments->correlation_shift = atoi(arg);
         break;
      case 't':
         arguments->start_position = atoi(arg);
         break;
      case 'u':
         arguments->run_number = atoi(arg);
         break;
      case 'w':
         arguments->spiroc_raw_filename = arg;
         break;
      case 'y':
         arguments->empty_bxid = 1;
         break;
      case 257:
         arguments->require_gainbit = 1;
         break;
      case 258:
         arguments->reject_gainbit = 1;
         break;
      case 265:
         arguments->from_trigger_time = atoi(arg);
         break;
      case 266:
         arguments->to_trigger_time = atoi(arg);
         break;
      case 267:
         arguments->dif_id = atoi(arg);
         break;
      case 268:
         arguments->lda_port = atoi(arg);
         break;
      case 269:
         arguments->report_EOR = atoi(arg);
         break;
      case 270:
         arguments->from_cycle = atoi(arg);
         break;
      case 271:
         arguments->to_cycle = atoi(arg);
         break;
      case ARGP_KEY_END:
         if ((arguments->spiroc_raw_filename == NULL)) {
            argp_error(state, "missing SPIROC data filename (This is essential)!\n");
         }
         break;
      default:
         return ARGP_ERR_UNKNOWN;
   }
   return 0;
}

/* argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

typedef struct {
   int16_t bxid;
   int16_t tdc;
   int16_t adc;
   int8_t hitbit;
   int8_t gainbit;
} SPIROC_record_t;

int hist_tdc[4096*2]; //TDC histogram
int hist_adc[4096*2]; //pseudo adc histogram
int hist_ch_adc[36][4096*2];
int hist_ch_tdc[36][4096*2];

typedef struct {
   u_int64_t tdc; //48-bit+5bit finestamp
   u_int32_t trig_count; //bif trigger counter
   u_int32_t ro_cycle; //derived from the Readout cycle corresponding to the start of acquisition
} BIF_record_t;

//SPIROC_record_t *spiroc_data = 0;
BIF_record_t *bif_data = 0;
int bif_last_record = 0;

void byteswap(unsigned char minibuf[]) {
   unsigned char byteswapped_minibuf[8];
   int i;
//	printf("Raw before: ");
   for (i = 0; i < 4; ++i) {
      byteswapped_minibuf[4 + i] = minibuf[i];
      byteswapped_minibuf[i] = minibuf[i + 4];
   }
//	for (i = 0; i < 8; ++i) {
//		printf("%02x ", minibuf[7 - i]);
//	}
//	printf("\n");
   for (i = 0; i < 8; ++i) {
      minibuf[i] = byteswapped_minibuf[i];
   }
//	printf("\tAfter: ");
//	for (i = 0; i < 8; ++i) {
//		printf("%02x ", minibuf[7 - i]);
//	}
//	printf("\n");
}

/* modulo counter reconstructs the full unsigned int caunter value from limited number of bits available */
int update_counter_modulo(unsigned int oldvalue, unsigned int newvalue_modulo, unsigned int modulo, unsigned int max_backwards) {
   unsigned int newvalue = oldvalue - max_backwards;
   unsigned int mask = modulo - 1;
   if ((newvalue & mask) > (newvalue_modulo & mask)) {
      newvalue += modulo;
   }
   newvalue = (newvalue & (~mask)) | (newvalue_modulo & mask);
   return newvalue;
}

int load_timestamps_from_ahcal_raw(struct arguments_t * arguments, BIF_record_t * bif_data, int * bif_last_record) {
   
   printf("#start reading BIF data from AHCAL raw data\n");
   int bif_data_index = 0;
   int i;
   for (i = 0; i < C_MAX_BIF_EVENTS; ++i) {
      bif_data[i] = (BIF_record_t ) { -1, -1, -1 };
   }
   // int j = 0;
   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the file\n");
      return -1;
   }

   int ROC = 0;
   u_int64_t TS = 0;
   /* u_int64_t lastTS = 0; */
   u_int64_t lastStartTS = 0;
   /* u_int64_t lastStopTS = 0; */

   int within_ROC = 0;
   unsigned char minibuf[8];
   while (1) {
      //int i = 0;
      if (fread(minibuf, 1, 1, fp) <= 0) goto file_finished2;
      if (minibuf[0] != 0xCD) continue;
      if (fread(minibuf, 1, 1, fp) <= 0) goto file_finished2;
      if (minibuf[0] != 0xCD) continue;
      if (fread(minibuf, sizeof(minibuf), 1, fp) <= 0) goto file_finished2;
      int length = (int) minibuf[0] + ((int) minibuf[1] << 8);
      if ((minibuf[0] != 0x10) || (minibuf[1] != 0x0) || (minibuf[7] != 0x08)) {
         //not packets we want
         //         printf("seek %04x\n", length);
         fseek(fp, length, SEEK_CUR);
         continue;
      }
      //      for (i = 0; i < 8; ++i) {
      //         fprintf(stdout, "byte%d=0x%02x, ", i, minibuf[i]);
      //      };
      //      fprintf(stdout, " // word 1\n");
      int newROC = (int) minibuf[2];
      //      printf("new ROC %d\n", newROC);
      //-----------------------------------------------------------
      // now read the TIME header + type + trigID
      if (fread(minibuf, sizeof(minibuf), 1, fp) <= 0) goto file_finished2;
      if ((minibuf[0] != 0x45) || (minibuf[1] != 0x4D) || (minibuf[2] != 0x49) || (minibuf[3] != 0x54)) {
         fseek(fp, 8, SEEK_CUR);
         continue;
      }
      //      for (i = 0; i < 8; ++i) {
      //         fprintf(stdout, "byte%d=0x%02x, ", i, minibuf[i]);
      //      };
      //      fprintf(stdout, " // word 2\n");
      int type = minibuf[4];

      int trigid = ((int) minibuf[6]) + (((int) minibuf[7]) << 8); //possible trigid

      //-------------------------------------------------------------
      // now read the Timestamp
      if (fread(minibuf, sizeof(minibuf), 1, fp) <= 0) goto file_finished2;
      if ((minibuf[6] != 0xAB) || (minibuf[7] != 0xAB)) continue;
      TS = (u_int64_t) minibuf[0] +
            ((u_int64_t) minibuf[1] << 8) +
            ((u_int64_t) minibuf[2] << 16) +
            ((u_int64_t) minibuf[3] << 24) +
            ((u_int64_t) minibuf[4] << 32) +
            ((u_int64_t) minibuf[5] << 40);
      if (type != 0x10) {
         if (type == 0x01) {      //start acq
            within_ROC = 1;
            ROC = update_counter_modulo(ROC, newROC, 256, 100);
            lastStartTS = TS;
         }
         if (type == 0x02) {
            within_ROC = 0;
            /* lastStopTS = TS; */
         }
         if (type == 0x20) within_ROC = 2; //busy raised, but did not yet received stop acq
         //         fseek(fp, 8, SEEK_CUR);
         continue;
      }
      int increment = (newROC - ROC) & 0xFF;
      if (increment > 5) {
         fprintf(stdout, "#ERROR wrong increment of ROC: %d\n", increment);
         //continue;
      }
      //      fprintf(stdout, "old ROC=%d, increment=%d\n", ROC, increment);
      /* ROC = ROC + increment; */
      //      fprintf(stdout, "%05d\t%05d\t%llu\t%d\t%d\t%lli\t#trigid,roc,TS,withinROC,ROCincrement,triggerTSinc\n", trigid, ROC, TS, within_ROC, increment, TS - lastTS);
      if ((bif_data_index < C_MAX_BIF_EVENTS) && (within_ROC == 1)) {/*if the data index is in range*/
         bif_data[bif_data_index].ro_cycle = (u_int32_t) ROC; // - first_shutter;
         bif_data[bif_data_index].tdc = (TS - lastStartTS); // << 5;
         bif_data[bif_data_index++].trig_count = trigid;
      }
      /* lastTS = TS; */
   }
   file_finished2:
   *bif_last_record = bif_data_index;
   if (fp != NULL) {
      if (fclose(fp)) {
         perror("#Unable to close the file\n");
      }
      //   printf("#finished reading BIF data\n");
   }
   if (arguments->start_position < 0) arguments->start_position = 0;
   printf("#finished reading trigger data from raw LDA packets. Processed %d triggers. Last ROC %d\n", bif_data_index, ROC);
   return 0;
}

int load_bif_data(struct arguments_t * arguments, BIF_record_t * bif_data, int * bif_last_record) {
   printf("#start reading BIF data\n");
//	int j = 0;
   int bif_data_index = 0;
   int i;
   for (i = 0; i < C_MAX_BIF_EVENTS; ++i) {
      bif_data[i] = (BIF_record_t ) { -1, -1, -1 };
   }
   FILE *fp;
   if (!(fp = fopen(arguments->bif_filename, "r"))) {
      perror("Unable to open the bif file\n");
      return -1;
   }
   u_int32_t time_h = 0, time_l = 0; /*timestamp MSB part and LSB part*/
   u_int64_t time = 0; /*merged timestamp*/
   u_int64_t oldtime_fcmd = 0;
   u_int64_t finetime_trig = 0;
   u_int16_t details = 0; /*part of the packet*/
   u_int16_t details_last = 0;
   u_int32_t trig_counter = 0;
   u_int8_t trig_details[4];
   u_int64_t shutter_cnt = 0;
   u_int64_t first_shutter = 0;
   /* u_int64_t last_accepted_trigger_TS = 0LLU; */
   unsigned char minibuf[8];
   while (1) {
      if (fread(minibuf, sizeof(minibuf), 1, fp) <= 0) /*read first 8 bytes*/
      goto file_finished;
//		byteswap(minibuf);
      time_h = *((u_int32_t *) minibuf + 1) & 0x0000FFFF; /*time_h: 16 bits from 0th and 1st byte */
      time_l = *(u_int32_t *) minibuf; /*time_l: bytes 4 to 7*/
      time = ((u_int64_t) time_h << 32) | time_l; /*merge time*/
      details_last = details;
      details = (*((u_int16_t *) minibuf + 3)) & 0x0FFF; /*this can be a shutter counter of binary list of fired channels*/
      switch (minibuf[7] >> 4) { //type of the packet
         case 0: /*internal trigger has similar format as external trigger*/
         case 1: /*external trigger*/
            if (fread(minibuf, sizeof(minibuf), 1, fp) <= 0) /*read next 8 bytes into the buffer*/
            goto file_finished;
//            byteswap(minibuf);
            trig_counter = *((u_int32_t *) minibuf + 0); /*extract the trigger counter*/
            trig_details[0] = minibuf[4]; /*we use only 1 trigger input at the moment*/
            /*the fine timestamp needs to be converted to the direct timestamp by subtracting 8 (= adding 24)*/
            finetime_trig = (time << 5) | ((trig_details[0] + 0x18) & 0x1F);
            /* last_accepted_trigger_TS = finetime_trig; */
            if ((shutter_cnt >= arguments->start_position) && (bif_data_index < C_MAX_BIF_EVENTS)) {/*if the data index is in range*/
               bif_data[bif_data_index].ro_cycle = shutter_cnt; // - first_shutter;
               bif_data[bif_data_index].tdc = finetime_trig - (oldtime_fcmd << 5);
               bif_data[bif_data_index++].trig_count = trig_counter;
            }
            break;
         case 2:
            //stop acquisition
            oldtime_fcmd = time;
            break;
         case 3:
            //"details" variable is treated as shutter_counter (12 bit)
            if ((details_last == 4095) && (details == 0)) {
               /*when the shutter counter overflows, we have to increment the shutter counter properlyew*/
               shutter_cnt += 4096;
            }
            /*use the lower 12 bits from the BIF data directly*/
            shutter_cnt &= 0xFFFFFFFFFFFFF000;
            shutter_cnt |= details;
            if (!first_shutter) {
               first_shutter = shutter_cnt;
               fprintf(stdout, "#First start acq cycle: %llu\n", (long long unsigned int) shutter_cnt);
               if (arguments->start_position < 0) arguments->start_position = shutter_cnt;
            }
            //start acquisition
//				printf("%llu\t%llu\t%llu\t#start acq\n", time, time - oldtime_fcmd, shutter_cnt);
            oldtime_fcmd = time;
            break;
         default:
            fprintf(stderr, "unknown BIF packet\n");
            //				exit(-1);
            break;
      }
   }
   file_finished: *bif_last_record = bif_data_index;
   //	if (fp != NULL) {
   if (fclose(fp)) {
      perror("Unable to close the file\n");
   }
   printf("#finished reading BIF data\n");
   return 0;
}

/* int get_first_iterator(const struct arguments_t * arguments, const BIF_record_t *bif_data, int ROcycle, int bxid) { */
/*    /\*perform a quick search in the iterator array*\/ */
/* //	bif_roc = bif_data[bif_iterator].ro_cycle - arguments->start_position; */
/* //	bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / 5120; */
/* //	bif_tdc = (bif_data[bif_iterator].tdc - arguments->correlation_shift) % 5120; */
/*    int wanted_bif_roc = ROcycle + arguments->start_position; */
/*    u_int64_t wanted_bif_tdc = bxid * arguments->bxid_length + arguments->correlation_shift; */
/*    u_int64_t wanted_bif_tdc_following = wanted_bif_tdc + arguments->bxid_length; */
/* //	printf("Inputs: req.ROC:%d\treq.BXID:%d\t", ROcycle, bxid); */
/* //	printf("WBROC:%llu\tWBTDC:%llu\tWBTDC_F:%llu\n", wanted_bif_roc, wanted_bif_tdc, wanted_bif_tdc_following); */
/*    int start = 0; */
/*    int end = bif_last_record;/\**\/ */
/*    int guess = 0; */
/*    int hit = -1; */
/*    if (arguments->correlation_shift + bxid * arguments->bxid_length < 0) return -1; */
/* //	printf("\n"); */
/*    while (start <= end) { */
/*       int half = (end - start) >> 1; */

/*       guess = start + half; */
/* //		printf("%d\t%d\t%d\t%d\t%d\n", start, end, guess, bif_data[guess].ro_cycle, bif_data[guess].trig_count); */
/*       if (bif_data[guess].ro_cycle == -1) {/\*we are out of the stored data*\/ */
/*          end = guess - 1; */
/*          continue; */
/*       } */
/*       if (bif_data[guess].ro_cycle < wanted_bif_roc) {/\*ROC smaller than wanted*\/ */
/*          start = guess + 1;/\*sart from the next*\/ */
/*       } else {/\*same or later readout cycle*\/ */
/*          if (bif_data[guess].ro_cycle > wanted_bif_roc) { */
/*             end = guess - 1;/\*higher readout cycle*\/ */
/*          } else { */
/*             /\*********************\/ */
/*             /\* same readout cycle*\/ */
/*             /\*********************\/ */
/*             /\* needs further comparisons*\/ */
/*             if (bif_data[guess].tdc < wanted_bif_tdc) { */
/*                /\*previous bxid. not ineterested*\/ */
/*                start = guess + 1; */
/*             } else { */
/*                if (bif_data[guess].tdc >= wanted_bif_tdc_following) { */
/*                   /\*next bxid. not interrested*\/ */
/*                   end = guess - 1; */
/*                } else { */
/*                   /\*same BXID. still can be more events in the same bxid. we need the first*\/ */
/*                   hit = guess;/\*it is a valid result, lets save it and continue searching better*\/ */
/*                   end = guess - 1; /\*and continue searching for earlier events*\/ */
/*                } */
/*             } */
/*          } */
/*       } */
/*    } */
/*    return hit; */
/* } */

int get_first_iterator2(const struct arguments_t * arguments, const BIF_record_t *bif_data, int ROcycle) {
   /*perform a quick search in the iterator array and return the bif trigger before the readout cycle*/
   int wanted_bif_roc = ROcycle + arguments->start_position;
   int resultindex = 0;
   int mask;
   //   printf("search started: rocycle %d, Wanted_bif_roc %d\n", ROcycle, wanted_bif_roc);
   for (mask = 1 << 30; mask; mask = mask >> 1) {
      resultindex |= mask;
      //      printf("mask: %08X\tresultindex:%d\n", mask, resultindex);
      if ((resultindex > bif_last_record) || (bif_data[resultindex].ro_cycle > wanted_bif_roc)) {
         resultindex &= ~mask;
         continue;
      }
      if (bif_data[resultindex].ro_cycle < wanted_bif_roc) continue;
      /*here we are in the same rocycle*/
      resultindex &= ~mask;
   }
   //   printf("search finished: rocycle wanted %d, found %d.\tindex:%d\n", ROcycle, bif_data[resultindex].ro_cycle, resultindex);
   return resultindex;
}

int get_ROCLength(const struct arguments_t * arguments, const BIF_record_t *bif_data, int ROcycle) {
   /*perform a quick search in the iterator array*/
   int wanted_bif_roc = ROcycle + arguments->start_position;
   int start = 0;
   int end = bif_last_record;/**/
   int guess = 0;
   int lastROC_TDC = -1;
   while (start <= end) {
      int half = (end - start) >> 1;
      guess = start + half;
      if (bif_data[guess].ro_cycle == -1) {/*we are out of the stored data*/
         end = guess - 1;
         continue;
      }
      if (bif_data[guess].ro_cycle < wanted_bif_roc) {/*ROC smaller than wanted*/
         start = guess + 1;/*sart from the next*/
      } else {/*same or later readout cycle*/
         if (bif_data[guess].ro_cycle > wanted_bif_roc) {
            end = guess - 1;/*higher readout cycle*/
         } else {
            /*********************/
            /* same readout cycle*/
            /*********************/
            /* needs further comparisons*/
            lastROC_TDC = bif_data[guess].tdc;
            start = guess + 1;
         }
      }
   }
   return (lastROC_TDC - arguments->correlation_shift) / arguments->bxid_length; //returns the last BxID TODO
}

FILE *of = NULL;/*output file*/
unsigned char buf[4096];

int isTriggered(const struct arguments_t * arguments, u_int32_t ROcycle, int bxid, int first_bif_iterator) {
   //int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
   int bif_iterator = first_bif_iterator;
   for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
      int bif_roc = bif_data[bif_iterator].ro_cycle - arguments->start_position;
      if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
      if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/

      int bif_bxid = ((int) bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
      if (bif_bxid > bxid) break; /*we jumped to another bxid with the bif_iterator*/
      if (bif_bxid < bxid) continue;/*not yet in the correct bxid*/
      //previous_bif_bxid = bif_bxid;
      return bif_iterator; //iterator found
   }
   return -1; //not found
}

int getPedestal(const int chip, const int channel, const int memcell, const int highgain) {
   return 0; //TODO
}
int convert_raw(const struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   int port_EORs[256];
   for (int i=0 ; i < (sizeof(port_EORs)/sizeof(port_EORs[0])); i++){
      port_EORs[i]=0;
   }
   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }
   memset(hist_adc, 0, sizeof hist_adc);
   memset(hist_tdc, 0, sizeof hist_tdc);
   memset(hist_ch_adc, 0, sizeof hist_ch_adc);
   memset(hist_ch_tdc, 0, sizeof hist_ch_tdc);
   /*spiroc datafile iteration*/
   int lda_port = 0;
   int dif_id = 0;
   int ro_chain = 0;
   int asic_index = 0;
   int bxid = 0;
   int asic = 0;
   int memcell = 0;
   int channel = 0;
   int tdc, adc = 0;
   int adc_hit = 0;
   int adc_gain = 0;
   int tdc_hit = 0;
   int tdc_gain = 0;
   int mismatches_hit = 0;
   int mismatches_hit_adc0_tdc1 = 0;
   int mismatches_gain = 0;
   int mismatches_gain_adc0_tdc1 = 0;
   int mismatches_length = 0;
   int bxids_even = 0;//counts even bxids: 0,2,4,...
   int bxids_odd = 0;//counts odd bxids: 1,3,5,...
   u_int32_t ROCLength = 0;
   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   //u_int32_t bif_roc = 0;
   int bif_bxid = 0;
//   u_int64_t bif_tdc = 0;
   u_int32_t entries = 0;//printed entries
   u_int32_t matches = 0;//correlated events
   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   /* int roc_prev = -1; */
   u_int32_t ROcycle = -1;
   if (!(arguments->histogram)) {
      printf("#1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\n");
      printf("#ROC\tbxid\tasic\tmcell\tchan\ttdc\tadc\thitb\tgainb\tBIF_TDC\tbxid(BIF-DIF)\tROCLen\tmem_filled\tport\tdifid\n");
   }
   while (1) {
      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         printf("#unable to read / EOF\n");
         break;
      }
      if (b != 0xCD) continue;/*try to look for first 0xCD. restart if not found*/

      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         perror("#unable to read / EOF\n");
         break;
      }
      if (b != 0xCD) continue;/*try to look for second 0xCD. restart if not found*/
      freadret = fread(&headlen, sizeof(headlen), 1, fp);
      freadret = fread(&headinfo, sizeof(headinfo), 1, fp);
      if (((headlen & 0xFFFF) > 4095) || ((headlen & 0xFFFF) < 4)) {
         printf("#wrong header length: %d\n", headlen & 0xffff);
         continue;
      }
      if ((headlen & 0xFFFF) == 0x10) {
         fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip timestamp packets
         //fprintf(stdout, "#TS packet\n");
         continue;
      }
      lda_port = (headinfo & 0xFF00)>>8;
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
//		printf("%05d\t", row_index);
//		printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF);
      freadret = fread(buf, 1, headlen & 0xFFF, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if (ROcycle < arguments->from_cycle) continue;
      if (ROcycle > (arguments->to_cycle+100)) break; /* for sure out of the range. let's stop */
      if (ROcycle > arguments->to_cycle) continue;    /* continue reading - some earlier roc can still appear */
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
//			printf("no spiroc data packet!\n");
         if (arguments->report_EOR>1) {
            printf("%d\t%d\tinfo=0x%08x len=0x%08x, data=",ROcycle,lda_port,headinfo, headlen);
            for (int i=0; i<freadret; i++){
               if ((i&0x03)==0) printf(" ");
               printf("%02x",buf[i]);
            }
            printf("#EOR\n");
         }
         if ((buf[0]==0x02) && (buf[1]==0xCC) && (buf[2]==0x0f) && (buf[4]==0x0e) && (buf[6]==0x03) ){
            port_EORs[lda_port]++;            
         }
         continue;
      }
      if ((arguments->lda_port != -1 ) && (lda_port != arguments->lda_port) ) continue;
//      fprintf(stdout,"#ROC: %d\n",ROcycle);
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      dif_id = buf[6] | (buf[7]<<8);//16-bit DIF ID
      ro_chain = buf[5];
      asic_index = buf[4];
      /* printf("#DEBUG: lda=%d, chain=%d, a_idx=%d, a=%d\n",lda_port,ro_chain,asic_index,asic); */
      if (((headlen & 0x0fff) - 12) % 146) { //if the length of the packet does not align with the number of memory cells
         mismatches_length++;
         printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle, asic);
      }
      if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
      
      if ((arguments->dif_id != -1) && (dif_id != arguments->dif_id)) continue; /* skip data from unwanted dif ID */
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//		printf("#memory cells: %d\n", memcell_filled);
      ROCLength = get_ROCLength(arguments, bif_data, ROcycle);
      int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
      for (memcell = arguments->from_memcell; (memcell<=arguments->to_memcell) && (memcell < memcell_filled); ++memcell) {
         if ((arguments->memcell != -1) && (memcell != arguments->memcell)) continue;/*skip data from unwanted memory cell*/
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         if ( (bxid < arguments->from_bxid) || (bxid > arguments->to_bxid) ) continue;
         if (bxid & 0x01) {
            bxids_odd++ ;
            if (arguments->even_bxid_only) continue;
         } else {
            bxids_even++;
            if (arguments->odd_bxid_only) continue;
         }
         
         int emptybxid=1;//assume at the beginning, that there is no hit
         for (channel = 0; channel < 36; ++channel) {
            //if there is a hit, then set emptybxid=0
            if (( (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8) & 0x1000)) emptybxid=0;
         }
         if ((arguments->empty_bxid) && (!emptybxid)) continue;//there was a hit somewere and wer were told to look only on empty bxids
         for (channel = 0; channel < 36; ++channel) {
            if ((arguments->channel != -1) && (channel != arguments->channel)) continue;/*ship data from unwanted channel*/

            tdc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1)]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8);
            adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8);
            adc_hit = (adc & 0x1000) ? 1 : 0;
            adc_gain = (adc & 0x2000) ? 1 : 0;
            tdc_hit = (tdc & 0x1000) ? 1 : 0;
            tdc_gain = (tdc & 0x2000) ? 1 : 0;\
            tdc = tdc & 0x0fff;
            adc = adc & 0x0fff;
            if (adc_hit != tdc_hit) {
               mismatches_hit++;
               if (adc_hit==0)mismatches_hit_adc0_tdc1++;
            }
            if (adc_gain != tdc_gain){
               mismatches_gain++;
               if (adc_gain == 0) mismatches_gain_adc0_tdc1++;
               /* printf("Gain mismatch: ROC=%d, A=%d, Ch=%d, mem=%d, ADC=%d, Gain_ADC=%d, Gain_TDC=%d\n", */
               /*        ROcycle, asic, channel, memcell, adc,adc_gain, tdc_gain); */
            }
            

            /*here is the main correlation loop*/
            if ((arguments->hitbit_only == 1) && (adc_hit == 0)) continue;/*skip data without hitbit, if required*/
            if ((arguments->hitbit_reject == 1) && (adc_hit == 1)) continue;/*skip data without hitbit, if required*/
            if ((arguments->require_gainbit == 1) && (adc_gain == 0)) continue; /* skip data withou gainbit */
            if ((arguments->reject_gainbit == 1) && (adc_gain == 1)) continue; /* skip data with gainbit */
            if (bif_iterator >= C_MAX_BIF_EVENTS) break;
            if (ROcycle >= C_ROC_READ_LIMIT) /*for debugging: if we do not want to read the while file. */
            break;

//				if ((memcell == 0) && (channel == 0))
//				printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", ROcycle, bxid, asic, memcell, channel, tdc, adc, hit, gain);
//            ext_search = (0 - arguments->extended_search);
//            if (ext_search + bxid < 0) /*fix the beginning of the runs*/
//            ext_search = 0;
//            for (; ext_search < (1 + arguments->extended_search); ++ext_search) {
            int matchingTriggerIterator = isTriggered(arguments, ROcycle, bxid, first_bif_iterator);
            if ((matchingTriggerIterator >= 0) && arguments->triggered_reject) continue;
            if ((matchingTriggerIterator < 0) && arguments->triggered_only) continue;
            u_int64_t bif_tdc = 0;
            if (matchingTriggerIterator >= 0) {
               bif_iterator = matchingTriggerIterator;
               //bif_roc = bif_data[bif_iterator].ro_cycle - arguments->start_position;
               bif_bxid = ((int) bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
               bif_tdc = (bif_data[bif_iterator].tdc - arguments->correlation_shift) % arguments->bxid_length;
               if (bif_tdc < arguments->from_trigger_time) continue;
               if (bif_tdc > arguments->to_trigger_time) continue;
               matches++;
            }
            
            if (arguments->histogram) {
               int adc_index = (adc & 0xfff) +  (adc_gain?0:4096);//low gain shifted to >4096
               if (adc_index > 8191) {//should not happen, but better be sure
                  adc_index = (8191/arguments->binning)*arguments->binning;
               }
               hist_adc[(adc_index/arguments->binning)*arguments->binning]++; 
               hist_ch_adc[channel][(adc_index/arguments->binning)*arguments->binning]++;
               //shift odd bxid to >4096
               hist_tdc[(( (tdc & 0xfff) | ((bxid & 0x01)<<12) )/arguments->binning)*arguments->binning]++;
               hist_ch_tdc[channel][(( (tdc & 0xfff) | ((bxid & 0x01)<<12) )/arguments->binning)*arguments->binning]++;
               entries++;
               if (matchingTriggerIterator >= 0) matches++;
               continue;//don't print details and go to next channel
            }
            entries++;
            if (matchingTriggerIterator >= 0) {
               /************************************************/
               /* here we have correlated candidate in memory  */
               /************************************************/
               printf("%d\t", ROcycle);
               printf("%d\t", bxid);
               printf("%d\t", asic);
               printf("%d\t", memcell);
               printf("%d\t", channel);
               printf("%d\t", tdc);
               printf("%d\t", adc);
               printf("%d\t", adc_hit);
               printf("%d\t", adc_gain);
               printf("%llu\t", (long long unsigned int) bif_tdc); //            printf("%llu\t", bif_data[bif_iterator].tdc);
               printf("%d\t", ((int) bif_bxid) - ((int) bxid));
               //                  printf("%d\t", same_bif_bxid_index);
               printf("%d\t", ROCLength);
               printf("%d\t", memcell_filled);
               printf("%d\t", lda_port);
               printf("%d\t", dif_id);
               if (adc_hit != tdc_hit) printf("\t#mismatched_tdc_hit");
               if (adc_gain != tdc_gain) printf("\t#mismatched_tdc_gain");
               printf("\n");
            } else {
               printf("%d\t", ROcycle);
               printf("%d\t", bxid);
               printf("%d\t", asic);
               printf("%d\t", memcell);
               printf("%d\t", channel);
               printf("%d\t", tdc);
               printf("%d\t", adc);
               printf("%d\t", adc_hit);
               printf("%d\t", adc_gain);
               printf("NaN\t");
               printf("NaN\t");
               printf("NaN\t");
               printf("%d\t", memcell_filled);
               printf("%d\t", lda_port);
               printf("%d\t", dif_id);
               if (adc_hit != tdc_hit) printf("\t#mismatched_tdc_hit");
               if (adc_gain != tdc_gain) printf("\t#mismatched_tdc_gain");
               printf("\n");
            }
         }
      }
   }
   fclose(fp);
   if (arguments->histogram) {
      int i, ch;
      printf("#bin\tadc\ttdc\tadc[ch0..35]\ttdc[ch0..35]\n");
      for (i = 0; i < ((sizeof hist_adc)/(sizeof (int))); i+=arguments->binning) {
         printf("%d\t%d\t%d", i, hist_adc[i], hist_tdc[i]);
         for (ch = 0; ch < 36; ch++)
            printf("\t%d", hist_ch_adc[ch][i]);
         for (ch = 0; ch < 36; ch++)
            printf("\t%d", hist_ch_tdc[ch][i]);
         printf("\n");
      }
   }
   printf("#Mismatched hit bits: %d (%f%% of all)\n", mismatches_hit,100.0*mismatches_hit/entries);
   printf("#Mismatched hit bits type adc0_tdc1: %d (%f%% of mismatched)\n", mismatches_hit_adc0_tdc1,(100.0)*mismatches_hit_adc0_tdc1/mismatches_hit);
   printf("#Mismatched gain bits: %d (%f%% of all)\n", mismatches_gain,100.0*mismatches_gain/entries);
   printf("#Mismatched gain bits type adc0_tdc1: %d, (%f%% of mismatched)\n", mismatches_gain_adc0_tdc1,100.0*mismatches_gain_adc0_tdc1/mismatches_gain);
   printf("#Mismatched packet lengths: %d\n",mismatches_length);
   printf("#odd bxids: %d\n",bxids_odd);
   printf("#even bxids: %d\n",bxids_even);
   printf("#bxid balance (odd vs all): %f\n",100.0*bxids_odd/(bxids_odd+bxids_even));
   printf("#Matched %d\n",matches);
   printf("#Entries %d\n",entries);
   if (arguments->report_EOR>0){
      for (int i=0 ; i < (sizeof(port_EORs)/sizeof(port_EORs[0])); i++){
         printf("#port=%d; EORs=%d\n",i,port_EORs[i]);
      }
   }
   return 0;
}

int main(int argc, char **argv) {
   /* Default values. */
   arguments_init(&arguments);
   bif_data = (BIF_record_t*) malloc(sizeof(BIF_record_t) * C_MAX_BIF_EVENTS);
   /* Parse our arguments; every option seen by parse_opt will
    be reflected in arguments. */
//error_t parseret =
   argp_parse(&argp, argc, argv, 0, 0, &arguments);
//   fprintf(of, "hi\n");
   arguments_print(&arguments);
   if (arguments.triggered_only || arguments.triggered_reject){
      if (arguments.bif_filename == NULL) {
	 load_timestamps_from_ahcal_raw(&arguments, bif_data, &bif_last_record);
      } else {
	 load_bif_data(&arguments, bif_data, &bif_last_record);
      }
   }
   if (arguments.spiroc_raw_filename != NULL) {
      convert_raw(&arguments, bif_data, bif_last_record);
      goto final;
   }
   final:
   free(bif_data);
   exit(0);
}
