#include <string.h> 		/* memset */
#include <stdlib.h>
#include <argp.h> /*parsing arguments*/
const int C_ROC_READ_LIMIT = 5000000;/*for debugging - reading only few RO cycles*/
const int C_MAX_BIF_EVENTS = 10*1000*1000;
//const int C_MAX_ROCYCLES = 1000;

const int C_MEM_CELLS = 16;

/* Program documentation. */
static char doc[] = "Statistics printout tool for RAW data dumped from EUDAQ BIF producer.  For more info try --help";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options will be parsed. */
static struct argp_option options[] =
{
   { "spiroc_raw_file", 'w', "SPIROC_RAW_FILE", 0, "!!!NOT IMPLEMENTED!!! raw file for the statistics data from AHCAL RAW file saved by EUDAQ. This file will be used instead of BIF data when used" },
   { "bif_raw_file", 'b', "BIF_FILE", 0, "filename/path of the BIF raw data file" },
   { "trig_data_from_spiroc_raw", 'd', 0, 0, "read the timestamps from the AHCAL raw data" },
//   { "output_file", 'o', "OUTPUT_FILE", 0, "!!!NOT IMPLEMENTED!!! filename to which the data should be written" },
//   { "force", 'f', 0, 0, "Force overwrite" },
//   { "start_position", 't', "START_POSITION", 0, "Start position in the BIF data (corresponds to the event counter of the first start acq channel). When no number is given, first data entry is considered as a first readout cycle" },
   { "correlation_shift", 'r', "RELATIVE_TIMESTAMP", 0, "Correlation timestamp, which synchronizes BXIDs between BIF and SPIROC. Default:13448" },
   { "bxid_length", 'l', "LENGTH", 0, "length of the BXID in BIF tics (to convert from ns: multiply by 1.28)" },
   { "aftertrigger_veto", 'v', "VETO_LENGTH", 0, "Period in ns after the last trigger, where another triggers are vetoed. Useful to filter glitches on trigger line" },
   { "report_gaps_ms", 'q', "LENGTH_MS", 0, "Report readout cycles, that have larger gaps before start (to detect temperature readouts and starts)" },
//   { "realign_bif_starts", 'z', "CLOCK_PHASE", 0, "Realign the clock phase in the BIF data to desired phase (0..7)" },
//   { "print_bif_start_phases", 'p', 0, 0, "simply print histogram of last 3 bits of the start acquisition commands seen by BIF" },
   { "print_triggers", 257, 0, 0, "print trigger entries" },
   { "print_cycles", 258, 0, 0, "print readout cycle informationtrigger entries" },
   { "roc_offset", 259, "OFFSET", 0, "readout cycle number adjustment" },
   { "trig_num_offset", 260, "OFFSET", 0, "Trigger number adjustemnt" },
   { "ignored_gaps_ms", 'g', "MS", 0, "Do not include into statistics gaps longed than MS milisecond s. Default=5000 ms" },
   { "run_number", 'n', "RUN_NUMBER", 0, "Run number used for the prints" },   
   { 0 } };

/* Used by main to communicate with parse_opt. */
struct arguments_t {
   char *bif_filename;
   char *spiroc_raw_filename;
   int trig_data_from_spiroc_raw;
   int correlation_shift;
   int bxid_length;
   int aftertrigger_veto;
   int report_gaps_ms;
   int print_triggers;
   int print_cycles;
   int run_number;
   int roc_offset;
   int trig_num_offset;
   int ignored_gaps_ms;
//   int print_bif_start_phases;
};
struct arguments_t arguments;

void arguments_init(struct arguments_t* arguments) {
   /* Default values. */
   arguments->spiroc_raw_filename = NULL;
   arguments->bif_filename = NULL;
   arguments->trig_data_from_spiroc_raw = 0;
   arguments->correlation_shift = 68080;
   arguments->bxid_length = 5120;
   arguments->aftertrigger_veto = 0;
   arguments->report_gaps_ms = 500;
   arguments->run_number = 0;
   arguments->print_triggers = 0;
   arguments->print_cycles = 0;
   arguments->roc_offset = 0;
   arguments->trig_num_offset = 0;
   arguments->ignored_gaps_ms = 5000;
//   arguments->print_bif_start_phases = 0;
}

void arguments_print(struct arguments_t* arguments) {
   printf("#BIF_data_file=\"%s\"\n", arguments->bif_filename);
   printf("#SPIROC_RAW_data_file=\"%s\"\n", arguments->spiroc_raw_filename);
   printf("#Correlation_shift=%d\n", arguments->correlation_shift);
   printf("#BXID_length=%d\n", arguments->bxid_length);
   printf("#trig_data_from_spiroc_raw=%d\n", arguments->trig_data_from_spiroc_raw);
   printf("#aftertrigger_veto=%d\n", arguments->aftertrigger_veto);
   printf("#report_gaps_ms=%d\n", arguments->report_gaps_ms);
//   printf("#print_bif_start_phases=%d\n", arguments->aftertrigger_veto);
   printf("#print_triggers=%d\n",arguments->print_triggers);
   printf("#print_cycles=%d\n",arguments->print_cycles);
   printf("#roc_offset=%d\n",arguments->roc_offset);
   printf("#trig_num_offset=%d\n",arguments->trig_num_offset);
   printf("#ignored_gaps_ms=%d\n",arguments->ignored_gaps_ms);
}

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
   /* Get the input argument from argp_parse, which we
    know is a pointer to our arguments structure. */
   struct arguments_t *arguments = state->input;

   switch (key) {
   case 'b':
      arguments->bif_filename = arg;
      break;
   case 'd':
      arguments->trig_data_from_spiroc_raw = 1;
      break;
   case 'g':
      arguments->ignored_gaps_ms = atoi(arg);
      break;
   case 'l':
      arguments->bxid_length = atoi(arg);
      break;
   case 'n':
      arguments->run_number = atoi(arg);
      break;
   case 'q':
      arguments->report_gaps_ms = atoi(arg);
      break;
   case 'r':
      arguments->correlation_shift = atoi(arg);
      break;
   case 'v':
      arguments->aftertrigger_veto = atoi(arg);
      break;
   case 'w':
      arguments->spiroc_raw_filename = arg;
      break;
   case 257:
      arguments->print_triggers = 1;
      break;
   case 258:
      arguments->print_cycles = 1;
      break;
   case 259:
      arguments->roc_offset = atoi(arg);
      break;
   case 260:
      arguments->trig_num_offset = atoi(arg);
      break;
   case ARGP_KEY_END:
      if ((arguments->bif_filename == NULL) && (arguments->trig_data_from_spiroc_raw == 0)) {
	 argp_error(state, "missing BIF data filename!\n");
         }
//			argp_usage(state);
//			argp_state_help(state, state->err_stream, ARGP_HELP_USAGE);
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

typedef struct {
   u_int64_t tdc; //48-bit+5bit finestamp
   u_int32_t trig_count; //bif trigger counter
   u_int32_t ro_cycle; //derived from the Readout cycle corresponding to the start of acquisition
} BIF_record_t;

static const int C_START_PHASES_LENGTH=8; //must be power of 2 !!! otherwise modulo will not work
u_int32_t *bif_start_phases;//[];//={0,0,0,0,0,0,0,0};

//typedef struct {
//      int16_t bxid;
//      int16_t tdc;
//      int16_t adc;
//} correlation_record_t

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

typedef struct {
   int ROCs;
   int triggers;
   u_int64_t OnTime;
   u_int64_t RunStart;
   u_int64_t RunFinish;
   u_int64_t IgnoredGaps;
} stats_t;

char * rocphases = NULL;
/* modulo counter reconstructs the full unsigned int caunter value from limited number of bits available */
int update_counter_modulo(unsigned int oldvalue, unsigned int newvalue_modulo, unsigned int modulo, unsigned int max_backwards){
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
   stats_t stats={0,0,0LLU,0LLU,0LLU,0LLU};

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
   u_int64_t TS, lastTS = 0;
   u_int64_t lastStartTS = 0;
   u_int64_t lastStopTS = 0;

   int within_ROC = 0;
   unsigned char minibuf[8];
   fprintf(stdout, "#ROC\tTrigid\tTS\tinROC\tROCincr\tTSfromStart\tfromLastTS\tphase\t#Trig\n");
   fprintf(stdout,"#RunNr\nROC\tstartTS\tstopTS\tlen_ROC\tlen_gap\t#cycle\n");
   
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
         if (type == 0x01) {//start acq
            within_ROC = 1;
	    if (lastStartTS==0LLU) stats.RunStart = TS;
	    ROC = update_counter_modulo(ROC, newROC, 256, 10);
	    stats.ROCs += 1;
	    if ((TS - lastStartTS) > (40000LLU * arguments->report_gaps_ms)) {
               fprintf(stdout, "#Long gap before start: roc=%d,\ttime_s=%f\n", ROC, (25.0E-9)*(TS - lastStartTS));
	    }
	    //--fill the start phases
	    u_int32_t bif_start_phase = ((u_int32_t)TS)&(C_START_PHASES_LENGTH - 1);
	    if (ROC<C_ROC_READ_LIMIT){
	       rocphases[ROC]=(char)bif_start_phase;
	    }
	    /* fprintf(stdout,"#ROC %d\tphase: %d\n",ROCReport,bif_start_phase); */
	    if (bif_start_phase<C_START_PHASES_LENGTH) bif_start_phases[bif_start_phase]++;
	    //--end of the fill of start phases
	    //if (arguments->realign_bif_starts > -100) TS = ((TS + arguments->realign_bif_starts) & (0xFFFFFFFFFFFFFFF8LLU));// - arguments->realign_bif_starts ;
            lastStartTS = TS;
            /* if (arguments->print_cycles) { */
            /*    fprintf(stdout,"%d\t",ROC); */
            /*    fprintf(stdout,"%llu\t",TS); */
            /*    fprintf(stdout,"1\tNaN\t#cycle\n"); */
            /* } */
         }
         if (type == 0x02) {//stop acq
            if (arguments->print_cycles) {
	       //fprintf(stdout,"#ROC\tstartTS\tstopTS\tlen_ROC\tlen_gap\t#cycle");
	       fprintf(stdout,"%d\t",arguments->run_number);
               fprintf(stdout,"%d\t",ROC);
	       fprintf(stdout,"%llu\t",(long long unsigned int)lastStartTS);
               fprintf(stdout,"%llu\t",(long long unsigned int)TS);
               fprintf(stdout,"%llu\t",(long long unsigned int)(TS - lastStartTS));
	       u_int64_t gap=lastStartTS-lastStopTS;
	       if (gap < 2400000000ULL)
		  fprintf(stdout,"%llu\t",(long long unsigned int)gap);
	       else
		  fprintf(stdout,"NaN\t");
	       fprintf(stdout, "%d\t",(int) rocphases[ROC]);
               fprintf(stdout,"#cycle\n");
            }
            within_ROC = 0;
	    stats.OnTime += TS - lastStartTS;
	    stats.RunFinish = TS;
            lastStopTS = TS;
         }	 
         if (type == 0x20) within_ROC = 2; //busy raised, but did not yet received stop acq
         //         fseek(fp, 8, SEEK_CUR);
         continue;
      }

      //      printf(".\n");

      int increment = (newROC - ROC) & 0xFF;
      if (increment > 50) {
	fprintf(stdout, "#ERROR wrong increment of ROC: %d\n", increment);
	//continue;
      }
      //      fprintf(stdout, "old ROC=%d, increment=%d\n", ROC, increment);
      ROC = ROC + increment;
      //      fprintf(stdout,"#Trigger\t%llu",(long long unsigned int) TS);
      stats.triggers++;
      //      fprintf(stdout, "%05d\t%05d\t%llu\t%d\t%d\t%lli\t#trigid,roc,TS,withinROC,ROCincrement,triggerTSinc\n", trigid, ROC, TS, within_ROC, increment, TS - lastTS);
      if ((bif_data_index < C_MAX_BIF_EVENTS)) {/*if the data index is in range*/
	bif_data[bif_data_index].ro_cycle = (u_int32_t) ROC; // - first_shutter;
	bif_data[bif_data_index].tdc = (TS - lastStartTS); // << 5;
	bif_data[bif_data_index++].trig_count = trigid;
      }
      if (arguments->print_triggers){
         fprintf(stdout, "%05d\t", ROC);
         fprintf(stdout, "%05d\t", trigid);
         fprintf(stdout, "%llu\t", (long long unsigned int) TS);
         fprintf(stdout, "%d\t", within_ROC);
         fprintf(stdout, "%d\t", increment);
         fprintf(stdout, "%lli\t", (long long unsigned int) (TS - lastStartTS));
         fprintf(stdout, "%lli\t", (long long unsigned int) (TS - lastTS));
         fprintf(stdout, "%d\t",(int) rocphases[ROC]);
         fprintf(stdout, "#Trig\n");
      }
      lastTS = TS;
   }
   file_finished2:
   *bif_last_record = bif_data_index;
   if (fp != NULL) {
      if (fclose(fp)) {
         perror("Unable to close the file\n");
      }
      //   printf("#finished reading BIF data\n");
   }
//   if (arguments->start_position < 0) arguments->start_position = 0;
   printf("#finished reading BIF data from raw LDA packets. Processed %d triggers. Last ROC %d\n", bif_data_index, ROC);
   int phase = -1;
   int phase_triggers = 0;
   for (i=0; i<8; i++){
      if (bif_start_phases[i]>phase_triggers){
	 phase = i;
	 phase_triggers = bif_start_phases[i];
      }
   }
   printf("#--- Statistics ---------------------------------------------\n");
   printf("#RunNr:%d\n",arguments->run_number);
   printf("#ROCs[count]:%llu\n", (long long unsigned int) stats.ROCs);
   printf("#ROC/s:%.1f\n",((long long unsigned int) (stats.ROCs))/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("#Triggers[count]:%d\n",stats.triggers);
   printf("#Length[s]:%.1f\n",25E-9*(stats.RunFinish-stats.RunStart));
   printf("#AvgROCLength[ms]:%.2f\n",25E-6*(stats.OnTime)/ (stats.ROCs));
   printf("#ontime[s]:%.1f\n",25E-9*(stats.OnTime));
   printf("#ontime[%%]:%.1f\n",100.0 * (stats.OnTime) / (stats.RunFinish-stats.RunStart) );
   printf("#Triggers/s:%.2f\n",stats.triggers/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("#Triggers/ROC:%.2f\n",1.0*stats.triggers/ (long long unsigned int) (stats.ROCs));
   printf("#Phase:%d\n",phase);
   printf("#Start:%f\n",25E-9*stats.RunStart);
   //-----------
   printf("%d\t",arguments->run_number);
   printf("%llu\t", (long long unsigned int) (stats.ROCs));
   printf("%.1f\t",((long long unsigned int) (stats.ROCs))/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("%d\t",stats.triggers);
   printf("%.1f\t",25E-9*(stats.RunFinish-stats.RunStart));
   printf("%.2f\t",25E-6*(stats.OnTime)/ (stats.ROCs));
   printf("%.1f\t",25E-9*(stats.OnTime));
   printf("%.1f\t",100.0 * (stats.OnTime) / (stats.RunFinish-stats.RunStart) );
   printf("%.2f\t",stats.triggers/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("%.2f\t",1.0*stats.triggers/ (long long unsigned int) (stats.ROCs));
   printf("%d\t",phase);
   printf("%.1f\t",25E-9*stats.RunStart);
   printf("#ShortStats\n");
   printf("#------------------------------------------------------------\n");
   return 0;
}

int load_bif_data(struct arguments_t * arguments, BIF_record_t * bif_data, int * bif_last_record) {
   printf("#start reading BIF data\n");
   stats_t stats={0,0,0LLU,0LLU,0LLU,0LLU};
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
   u_int64_t first_shutter = 0;//first shutter from BIF is random
   u_int32_t first_trigger = 0;
   u_int64_t last_accepted_trigger_TS = 0LLU;
   unsigned char minibuf[8];
   if (arguments->print_triggers) fprintf(stdout,"ROC\ttirg#\tTS\tinside\trocDiff\tfrom_start\tfrom_previous\t#Trig\n");
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
            if (first_trigger == 0) first_trigger = trig_counter;
            trig_details[0] = minibuf[4]; /*we use only 1 trigger input at the moment*/
            /*the fine timestamp needs to be converted to the direct timestamp by subtracting 8 (= adding 24)*/
            finetime_trig = (time << 5) | ((trig_details[0] + 0x18) & 0x1F);
	    if ((finetime_trig - last_accepted_trigger_TS) < ((arguments->aftertrigger_veto*128)/100)) continue;
	    stats.triggers++;
            if ((bif_data_index < C_MAX_BIF_EVENTS)) {/*if the data index is in range*/
               bif_data[bif_data_index].ro_cycle = shutter_cnt; // - first_shutter;
               bif_data[bif_data_index].tdc = finetime_trig - (oldtime_fcmd << 5);
               bif_data[bif_data_index++].trig_count = trig_counter;
            }
            if (arguments->print_triggers){
               fprintf(stdout,"%d\t",shutter_cnt - first_shutter + arguments->roc_offset);
               fprintf(stdout,"%d\t",trig_counter - first_trigger + arguments->trig_num_offset);
               fprintf(stdout,"%llu\t",(long long unsigned int) (finetime_trig>>5));
               fprintf(stdout,"1\t");//in Bif it is inside acquisition by definition
               fprintf(stdout,"NaN\t");//no information about the ROC in the packet
               fprintf(stdout,"%llu\t",(long long unsigned int) ((finetime_trig>>5)-oldtime_fcmd));
               fprintf(stdout,"%llu\t",(long long unsigned int) ((finetime_trig-last_accepted_trigger_TS)>>5));
               fprintf(stdout,"#Trig\n");
            }
	    last_accepted_trigger_TS = finetime_trig;
            break;
         case 2:
            //stop acquisition
	    stats.OnTime += (time-oldtime_fcmd);
	    stats.RunFinish = time;
            oldtime_fcmd = time;
	    break;
         case 3:
	    stats.ROCs++;
            //"details" variable is treated as shutter_counter (12 bit)
            if ((details_last == 4095) && (details == 0)) {
               /*when the shutter counter overflows, we have to increment the shutter counter properlyew*/
               shutter_cnt += 4096;
            }
            /*use the lower 12 bits from the BIF data directly*/
            shutter_cnt &= 0xFFFFFFFFFFFFF000;
            shutter_cnt |= details;
            if (!first_shutter) {
	       stats.RunStart = time;
               first_shutter = shutter_cnt;
               fprintf(stdout, "#First start acq cycle: %llu\n", (long long unsigned int) shutter_cnt);
               //if (arguments->start_position < 0) arguments->start_position = shutter_cnt;
            }
            //start acquisition
//				printf("%llu\t%llu\t%llu\t#start acq\n", time, time - oldtime_fcmd, shutter_cnt);
	    //if (arguments->realign_bif_starts > -100) time = ((time + arguments->realign_bif_starts) & (0xFFFFFFFFFFFFFFF8LLU));// - arguments->realign_bif_starts ;
            if (oldtime_fcmd) {
               if ((time - oldtime_fcmd) > (40000LLU * arguments->report_gaps_ms)) {
                  fprintf(stdout, "#Long gap before start: raw=%llu,\tFrom0=%llu,\ttime_s=%f\n", 
                          (long long unsigned int) shutter_cnt, 
                          (long long unsigned int) (shutter_cnt-first_shutter), 
                          (25.0E-9)*(time - oldtime_fcmd));
               }
               if ((time - oldtime_fcmd) > (40000LLU * arguments->ignored_gaps_ms)) {
                  stats.IgnoredGaps += (time - oldtime_fcmd);
               }
            }
	    u_int32_t bif_start_phase = ((u_int32_t)time)&(C_START_PHASES_LENGTH - 1);
	    //printf("%d\t%d\t#Phase\n", bif_start_phase, (u_int32_t)shutter_cnt);//DEBUG
	    if (bif_start_phase<C_START_PHASES_LENGTH) bif_start_phases[bif_start_phase]++;
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
   int phase=-1;
   int phase_triggers=0;
   for (i=0; i<8; i++){
      if (bif_start_phases[i]>phase_triggers){
	 phase = i;
	 phase_triggers = bif_start_phases[i];
      }
   }
   
   printf("#finished reading BIF data\n");
   printf("#--- Statistics ---------------------------------------------\n");
   printf("#RunNr:%d\n",arguments->run_number);
   printf("#ROCs[count]:%llu\n", (long long unsigned int) (shutter_cnt-first_shutter));
   printf("#ROC/s:%.1f\n",((long long unsigned int) (shutter_cnt-first_shutter))/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("#ROC_without_gaps/s:%.1f\n",((long long unsigned int) (shutter_cnt-first_shutter))/(25E-9*(stats.RunFinish-stats.RunStart-stats.IgnoredGaps)));
   printf("#Triggers[count]:%d\n",stats.triggers);
   printf("#Length[s]:%.1f\n",25E-9*(stats.RunFinish-stats.RunStart));
   printf("#Length_without_gaps[s]:%.1f\n",25E-9*(stats.RunFinish-stats.RunStart-stats.IgnoredGaps));
   printf("#AvgROCLength[s]:%.2f\n",25E-6*(stats.OnTime)/ (shutter_cnt-first_shutter));
   printf("#ontime[s]:%.1f\n",25E-9*(stats.OnTime));
   printf("#ontime[%%]:%.1f\n",100.0 * (stats.OnTime) / (stats.RunFinish-stats.RunStart) );
   printf("#ontime_without_gaps[%%]:%.1f\n",100.0 * (stats.OnTime) / (stats.RunFinish-stats.RunStart-stats.IgnoredGaps) );
   printf("#Triggers/s:%.2f\n",stats.triggers/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("#Triggers_without_gaps/s:%.2f\n",stats.triggers/(25E-9*(stats.RunFinish-stats.RunStart-stats.IgnoredGaps)));
   printf("#Triggers/ROC:%.2f\n",1.0*stats.triggers/ (long long unsigned int) (shutter_cnt-first_shutter));
   printf("#Phase:%d\n",phase);
   printf("#Start:%f\n",25E-9*stats.RunStart);
   //-----------
   printf("%d\t",arguments->run_number);
   printf("%llu\t", (long long unsigned int) (shutter_cnt-first_shutter));
   printf("%.1f\t",((long long unsigned int) (shutter_cnt-first_shutter))/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("%.1f\t",((long long unsigned int) (shutter_cnt-first_shutter))/(25E-9*(stats.RunFinish-stats.RunStart-stats.IgnoredGaps)));
   printf("%d\t",stats.triggers);
   printf("%.1f\t",25E-9*(stats.RunFinish-stats.RunStart));
   printf("%.1f\t",25E-9*(stats.RunFinish-stats.RunStart-stats.IgnoredGaps));
   printf("%.2f\t",25E-6*(stats.OnTime)/ (shutter_cnt-first_shutter));
   printf("%.1f\t",25E-9*(stats.OnTime));
   printf("%.1f\t",100.0 * (stats.OnTime) / (stats.RunFinish-stats.RunStart) );
   printf("%.1f\t",100.0 * (stats.OnTime) / (stats.RunFinish-stats.RunStart-stats.IgnoredGaps) );
   printf("%.2f\t",stats.triggers/(25E-9*(stats.RunFinish-stats.RunStart)));
   printf("%.2f\t",stats.triggers/(25E-9*(stats.RunFinish-stats.RunStart-stats.IgnoredGaps)));
   printf("%.2f\t",1.0*stats.triggers/ (long long unsigned int) (shutter_cnt-first_shutter));
   printf("%d\t",phase);
   printf("%.1f\t",25E-9*stats.RunStart);
   printf("#ShortStats\n");
   printf("#------------------------------------------------------------\n");
   return 0;
}




FILE *of = NULL;/*output file*/
unsigned char buf[4096];



void print_bif_phases(){
   int i=0;
   printf("#phase\tcounts\n");
   for (;i<C_START_PHASES_LENGTH; i++){
      printf("%d\t%d\n", i, bif_start_phases[i]);
   };
}

int main(int argc, char **argv) {
   /* Default values. */
   arguments_init(&arguments);
   bif_data = (BIF_record_t*) malloc(sizeof(BIF_record_t) * C_MAX_BIF_EVENTS);
   bif_start_phases = (u_int32_t*) malloc(sizeof(int) * C_START_PHASES_LENGTH);
   memset(bif_start_phases,0,sizeof(int) * C_START_PHASES_LENGTH);
   rocphases = (char *) malloc (sizeof(char) * C_ROC_READ_LIMIT);
   memset(rocphases, (char)0, sizeof(char) * C_ROC_READ_LIMIT);
   /* Parse our arguments; every option seen by parse_opt will
    be reflected in arguments. */
//error_t parseret =
   argp_parse(&argp, argc, argv, 0, 0, &arguments);
   arguments_print(&arguments);
   
   if (arguments.trig_data_from_spiroc_raw == 1) {
      load_timestamps_from_ahcal_raw(&arguments, bif_data, &bif_last_record);
   } else {
      load_bif_data(&arguments, bif_data, &bif_last_record);
   }
   
   print_bif_phases();

//	printf("ARG1 = %s\nARG2 = %s\nOUTPUT_FILE = %s\n"
//			"VERBOSE = %s\nSILENT = %s\n",
//	        arguments.args[0], arguments.args[1],
//	        arguments.output_file,
//	        arguments.verbose ? "yes" : "no",
//	        arguments.silent ? "yes" : "no");
final:
   free(bif_data);
   free(bif_start_phases);
   free(rocphases);

				
   exit(0);
}
