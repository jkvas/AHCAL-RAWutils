#include <stdlib.h>
#include <argp.h> /*parsing arguments*/
const int C_ROC_READ_LIMIT = 1000000;/*for debugging - reading only few RO cycles*/
const int C_MAX_BIF_EVENTS = 5000000;
#define C_MAX_PORTS 48
//const int C_MAX_ROCYCLES = 1000;

const int C_MEM_CELLS = 16;

/* Program documentation. */
static char doc[] = "BIF vs SPIROC data correlation tool. For more info try --help";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options will be parsed. */
static struct argp_option options[] =
      {
            { "spiroc_txt_file", 's', "SPIROC_FILE", 0, "filename/path of the Spiroc TXT file produced by the labview" },
            { "spiroc_raw_file", 'w', "SPIROC_RAW_FILE", 0, "filename/path of the Spiroc RAW file saved by EUDAQ" },
            { "bif_raw_file", 'b', "BIF_FILE", 0, "filename/path of the BIF raw data file" },
            { "trig_data_from_spiroc_raw", 'd', 0, 0, "read the timestamps from the AHCAL raw data" },
            { "output_file", 'o', "OUTPUT_FILE", 0, "!!!NOT IMPLEMENTED!!! filename to which the data should be written" },
            { "force", 'f', 0, 0, "Force overwrite" },
            { "asic", 'a', "ASIC_NUMBER", 0, "asic number (must be < 256)" },
            { "channel", 'c', "CHANNEL_NUMBER", 0, "channel number to be correlated" },
            { "memcell", 'm', "MEMCELL_NUMBER", 0, "memory cell to only take into account (0..15). Will set minimum and maximum to this value" },
            { "minimum_memcell", 262, "MEMCELL_NUMBER", 0, "lowest memory cell to take into account (0..15)" },
            { "maximum_memcell", 263, "MEMCELL_NUMBER", 0, "highest memory cell to take into account (0..15)" },
            { "extended_search", 'x', "BXID_TOLERANCE", 0, "extended search in +-BIXID_TOLERANCE BXIDs. Useful only for investigating BXID errors. Default 0" },
            { "start_position", 't', "START_POSITION", 0, "Start ROC offset in the BIF data. (=-1 when BIF starts from 0 and AHCAL from 1)" },
            { "correlation_shift", 'r', "RELATIVE_TIMESTAMP", 0, "Correlation timestamp, which synchronizes BXIDs between BIF and SPIROC. Default:13448" },
            { "shift_scan_max", 'n', "SHIFT_SCAN_MAX", 0, "Do the scan for offset values between AHCAL and BIF from 0 to the given maximum. Results are calculated event-wise (every BXID is counted only once for the whole detector). Default:-1, maximum: 2000000" },
            { "shift_scan_method", 'e', "METHOD_NUMBER", 0, "Selects which method do use for the correlation:\n0=BXIDs-wise (same BXID from multiple asics are counted only once)\n1=ASIC-wise: each correlated events is summed up individually (same BXID in 2 chips are counted twice)\n2=channel-wise: only specified channel is used. Default:0" },
            { "bif_trigger_spacing", 'g', 0, 0, "print the time distance between particles as BXID and timestamp differences. Correct offset should be given" },
            { "minimum_bxid", 257, "MIN_BXID", 0, "BXIDs smaller than MIN_BXID will be ignored. Default:1" },
            { "maximum_bxid", 258, "MAX_BXID", 0, "BXIDs greater than MAX_BXID will be ignored. Default:4095" },
            { "bxid_spacing", 'i', "MODE", 0, "print the bxid distance in AHCAL data.. Mode:\n1=print all distances\n2=print distances of correlated BXID\n3=print only uncorrelated distances (from last correlated bxid)" },
            { "bxid_length", 'l', "LENGTH", 0, "length of the BXID in BIF tics (to convert from ns: multiply by 1.28)" },
            { "require_hitbit", 'h', 0, 0, "Filter only the hitbit data" },
            { "aftertrigger_veto", 'v', "VETO_LENGTH", 0,"Period in ns after the last trigger, where another triggers are vetoed. Useful to filter glitches on trigger line" },
            { "report_gaps_ms", 'q', "LENGTH_MS", 0, "Report readout cycles, that have larger gaps before start (to detect temperature readouts and starts)" },
            { "realign_bif_starts", 'z', "CLOCK_PHASE", 0,"Realign the clock phase in the BIF data to desired phase (0..7). Use value <-99 to skip the realignment (or don't use this parameter at all)" },
            { "print_bif_start_phases", 'p', 0, 0, "simply print histogram of last 3 bits of the start acquisition commands seen by BIF" },
            { "run_number", 'u', "RUN_NUMBER", 0, "Run number used for the prints" },
            { "debug_constant", 'k', "VALUE", 0, "Debug constant for various purposes." },
            { "print_triggers", 'y', 0, 0, "prints trigger details." },
            { "trigger_input", 259, "NUMBER",0,"use this BIF trigger number. Default:3 (close to RJ45)" },
            { "module", 260, "NUMBER",0,"use only this module number. NOT IMPLEMENTED" },
            { "lda_port", 261, "NUMBER",0,"use only this port number." },
            { 0 } };

/* Used by main to communicate with parse_opt. */
struct arguments_t {
   char *spiroc_txt_filename;
   char *spiroc_raw_filename;
   char *bif_filename;
   char *output_file;
   int forced; /*forced file overwrite*/
   int asic;
   int channel;
   /* int memcell; */
   int extended_search;
   int start_position;
   int correlation_shift;
   int shift_scan;
   int shift_scan_method;
   int bif_trigger_spacing;
   int minimum_bxid;
   int maximum_bxid;
   int bxid_spacing;
   int bxid_length;
   int require_hitbit;
   int trig_data_from_spiroc_raw;
   int aftertrigger_veto;
   int report_gaps_ms;
   int print_bif_start_phases;
   int realign_bif_starts;
   int run_number;
   int debug_constant;
   int print_triggers;
   int trigger_input;
   int module;
   int lda_port;
   int minimum_memcell;
   int maximum_memcell;
};
struct arguments_t arguments;

void arguments_init(struct arguments_t* arguments) {
   /* Default values. */
   arguments->spiroc_txt_filename = NULL;
   arguments->spiroc_raw_filename = NULL;
   arguments->bif_filename = NULL;
   arguments->output_file = NULL;
   arguments->forced = 0;
   arguments->asic = -1;
   arguments->channel = -1;
   //arguments->memcell = -1;
   arguments->extended_search = 0;
   arguments->start_position = 0;
   arguments->correlation_shift = 13448;
   arguments->shift_scan = -1;
   arguments->shift_scan_method = 0;
   arguments->require_hitbit = 0;
   arguments->minimum_bxid = 1;/*by default ship BXID 0*/
   arguments->maximum_bxid = 4095;
   arguments->bif_trigger_spacing = -1;
   arguments->bxid_length = 5120;
   arguments->bxid_spacing = -1;
   arguments->trig_data_from_spiroc_raw = 0;
   arguments->aftertrigger_veto = 0;
   arguments->report_gaps_ms = 500;
   arguments->print_bif_start_phases = 0;
   arguments->realign_bif_starts = -100;
   arguments->run_number = -1;
   arguments->debug_constant = -1;
   arguments->print_triggers = 0;
   arguments->trigger_input = 3;
   arguments->module = -1;
   arguments->lda_port = -1;
   arguments->minimum_memcell = 0;
   arguments->maximum_memcell = 15;
}

void arguments_print(struct arguments_t* arguments) {
   printf("#Run number=%d\n", arguments->run_number);
   printf("#SPIROC_TXT_data_file=\"%s\"\n", arguments->spiroc_txt_filename);
   printf("#SPIROC_RAW_data_file=\"%s\"\n", arguments->spiroc_raw_filename);
   printf("#BIF_data_file=\"%s\"\n", arguments->bif_filename);
   printf("#Output_file=\"%s\"\n", arguments->output_file);
   printf("#Forced_overwrite=\"%s\"\n", arguments->forced ? "yes" : "no");
   printf("#ASIC_number=%d\n", arguments->asic);
   printf("#Channel_number=%d\n", arguments->channel);
//   printf("#memory_cell=%d\n", arguments->memcell);
   printf("#minimum_memcell=%d\n", arguments->minimum_memcell);
   printf("#maximum_memcell=%d\n", arguments->maximum_memcell);
   printf("#Extended_search=%d\n", arguments->extended_search);
   printf("#BIF_start_position=%d\n", arguments->start_position);
   printf("#Correlation_shift=%d\n", arguments->correlation_shift);
   printf("#Shift_scan_max=%d\n", arguments->shift_scan);
   printf("#Shift_scan_method=%d\n", arguments->shift_scan_method);
   printf("#REQUIRE_Hitbit=%s\n", arguments->require_hitbit ? "yes" : "no");
   printf("#Trigger_spacing=%d\n", arguments->bif_trigger_spacing);
   printf("#BXID_spacing=%d\n", arguments->bxid_spacing);
   printf("#BXID_length=%d\n", arguments->bxid_length);
   printf("#Minimum BXID=%d\n", arguments->minimum_bxid);
   printf("#Maximum BXID=%d\n", arguments->maximum_bxid);
   printf("#trig_data_from_spiroc_raw=%d\n", arguments->trig_data_from_spiroc_raw);
   printf("#aftertrigger_veto=%d\n", arguments->aftertrigger_veto);
   printf("#report_gaps_ms=%d\n", arguments->aftertrigger_veto);
   printf("#print_bif_start_phases=%d\n", arguments->aftertrigger_veto);
   printf("#realign_bif_starts=%d\n", arguments->realign_bif_starts);
   printf("#Debug_constant=%d\n", arguments->debug_constant);
   printf("#Print_triggers=%d\n", arguments->print_triggers);
   printf("#trigger_input=%d\n", arguments->trigger_input);
   printf("#module=%d\n", arguments->module);
   printf("#lda_port=%d\n", arguments->lda_port);
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
      case 'd':
         arguments->trig_data_from_spiroc_raw = 1;
         break;
      case 'e':
         arguments->shift_scan_method = atoi(arg);
         break;
      case 'f':
         arguments->forced = 1;
         break;
      case 'g':
         arguments->bif_trigger_spacing = 1;
         break;
      case 'h':
         arguments->require_hitbit = 1;
         break;
      case 'i':
         arguments->bxid_spacing = atoi(arg);
         break;
      case 257:
         arguments->minimum_bxid = atoi(arg);
         break;
      case 258:
         arguments->maximum_bxid = atoi(arg);
         break;
      case 259:
         arguments->trigger_input = atoi(arg);
         break;
      case 260:
         arguments->module = atoi(arg);
         break;
      case 261:
         arguments->lda_port = atoi(arg);
         break;
      case 262:
         arguments->minimum_memcell = atoi(arg);
         break;
      case 263:
         arguments->maximum_memcell = atoi(arg);
         break;
      case 'k':
         arguments->debug_constant = atoi(arg);
         break;
      case 'l':
         arguments->bxid_length = atoi(arg);
         break;
      case 'm':
         arguments->minimum_memcell = atoi(arg);
         arguments->maximum_memcell = arguments->minimum_memcell;
         break;
      case 'n':
         arguments->shift_scan = atoi(arg);
         break;
      case 'o':
         arguments->output_file = arg;
         break;
      case 'p':
         arguments->print_bif_start_phases = 1;
         break;
      case 'q':
         arguments->report_gaps_ms = atoi(arg);
         break;
      case 'r':
         arguments->correlation_shift = atoi(arg);
         break;
      case 's':
         arguments->spiroc_txt_filename = arg;
         break;
      case 't':
         arguments->start_position = atoi(arg);
         break;
      case 'u':
         arguments->run_number = atoi(arg);
         break;
      case 'v':
         arguments->aftertrigger_veto = atoi(arg);
         break;
      case 'w':
         arguments->spiroc_raw_filename = arg;
         break;
      case 'x':
         arguments->extended_search = atoi(arg);
         break;
      case 'y':
         arguments->print_triggers = 1;
         break;
      case 'z':
         arguments->realign_bif_starts = atoi(arg);
         break;
      case ARGP_KEY_END:
         if ((arguments->spiroc_txt_filename == NULL) && (arguments->spiroc_raw_filename == NULL)) {
            argp_error(state, "missing SPIROC data filename (can be raw or TXT)!\n");
         }
         if ((arguments->spiroc_txt_filename != NULL) && (arguments->spiroc_raw_filename != NULL)) {
            argp_error(state, "too many input files (both raw and TXT)!\n");
         }
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

static const int C_START_PHASES_LENGTH = 8; //must be power of 2 !!! otherwise modulo will not work
u_int32_t *bif_start_phases; //[];//={0,0,0,0,0,0,0,0};

//typedef struct {
//      int16_t bxid;
//      int16_t tdc;
//      int16_t adc;
//} correlation_record_t

//SPIROC_record_t *spiroc_data = 0;
BIF_record_t *bif_data = 0;

typedef struct {
   u_int8_t cell_modules[C_MAX_PORTS];
   u_int8_t cell_detector;
   u_int16_t bxid_modules[C_MAX_PORTS];
   u_int16_t bxid_detector;
   u_int64_t acq_length;
} extra_stats_t;
extra_stats_t *extra_stats;

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

/* recodes a partially decoded gray number. the 16-bit number is partially decoded for bits 0-11. */
u_int16_t grayRecode(const u_int16_t partiallyDecoded) {
   u_int16_t Gray = partiallyDecoded;
   u_int16_t highPart = 0; //bits 12-15 are not decoded
   while (Gray & 0xF000) {
      highPart ^= Gray;
      Gray >>= 1;
   }
   if (highPart & 0x1000) {
      return ((highPart & 0xF000) | ((~partiallyDecoded) & 0xFFF)); //invert the originally decoded data (the bits 0-11)
   } else {
      return ((highPart & 0xF000) | (partiallyDecoded & 0xFFF)); //combine the low and high part
   }
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
   u_int64_t TS, lastTS = 0;
   u_int64_t lastStartTS = 0;
   u_int64_t lastStopTS = 0;

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
            if ((TS - lastStartTS) > (40000LLU * arguments->report_gaps_ms)) {
               fprintf(stdout, "#Long gap before start: roc=%d,\ttime_s=%f\n", ROC, (25.0E-9) * (TS - lastStartTS));
            }
            //--fill the start phases
            u_int32_t bif_start_phase = ((u_int32_t) TS) & (C_START_PHASES_LENGTH - 1);
            if (bif_start_phase < C_START_PHASES_LENGTH) bif_start_phases[bif_start_phase]++;
            //--end of the fill of start phases
            //if (arguments->realign_bif_starts > -100) TS = ((TS + arguments->realign_bif_starts) & (0xFFFFFFFFFFFFFFF8LLU));// - arguments->realign_bif_starts ;
            if (arguments->realign_bif_starts > -100) {
               //time = ((time + arguments->realign_bif_starts) & (0xFFFFFFFFFFFFFFF8LLU));// - arguments->realign_bif_starts ;
               //TODO following procedure does not seem to work correctly. needs further investigation
               u_int64_t c1 = (u_int64_t) 6LLU;	       //(arguments->debug_constant&0x7);//for faster edits
               u_int64_t c2 = (u_int64_t) 1LLU;	       //((arguments->debug_constant >> 3)& 0x07);//for faster edits
               u_int64_t correction1 = (TS + ((c1 - arguments->realign_bif_starts) & 0x07LLU)) & 0xffffFFFFffffFFF8LLU;	//to align all start acquisition phases to the singled one, which was set by the sync CCC command during the start of the run
               u_int64_t correction2 = ((arguments->realign_bif_starts + c2) & 0x07LLU) - c2;//to realight the start phase-corrected value onto the same AHCAL-BIF offset (otherwise this offset would vary with the phase)
               TS = correction1 + correction2;
            }
            lastStartTS = TS;
            /* fprintf(stdout,"Debug start\troc=%d\tNROC=%d\tTS=%llu\n",ROC,newROC,(long long unsigned int) TS); */
            /* fprintf(stdout,"#Debug start\tROC=%d\n",ROC); */
            /* fprintf(stdout,"#Debug start\tNROC=%d\n",newROC); */
            /* fprintf(stdout,"#Debug start\tTS=%llu\n",(long long unsigned int) TS); */
         }
         if (type == 0x02) {
            within_ROC = 0;
            lastStopTS = TS;
            /* fprintf(stdout,"Debug stop\troc=%d\tNROC=%d\tTS=%llu\n",ROC,newROC,(long long unsigned int) TS);  */
            if ((ROC >= 0) && (ROC < C_ROC_READ_LIMIT)) extra_stats[(ROC)].acq_length = lastStopTS - lastStartTS;
         }
         if (type == 0x20) within_ROC = 2; //busy raised, but did not yet received stop acq
         //         fseek(fp, 8, SEEK_CUR);
         continue;
      }

      //      printf(".\n");

      int increment = (newROC - ROC) & 0xFF;
      if (increment > 5) {
         fprintf(stdout, "#ERROR wrong increment of ROC: %d\n", increment);
         //continue;
      }
      //      fprintf(stdout, "old ROC=%d, increment=%d\n", ROC, increment);
      ROC = ROC + increment;

      //      fprintf(stdout, "%05d\t%05d\t%llu\t%d\t%d\t%lli\t#trigid,roc,TS,withinROC,ROCincrement,triggerTSinc\n", trigid, ROC, TS, within_ROC, increment, TS - lastTS);
      if ((bif_data_index < C_MAX_BIF_EVENTS) && (within_ROC == 1)) {/*if the data index is in range*/
         bif_data[bif_data_index].ro_cycle = (u_int32_t) ROC; // - first_shutter;
         bif_data[bif_data_index].tdc = (TS - lastStartTS); // << 5;
         bif_data[bif_data_index++].trig_count = trigid;
      }
      if ( (arguments->print_triggers)) {
         fprintf(stdout, "#%05d\t", ROC);
         fprintf(stdout, "%05d\t", trigid);
         fprintf(stdout, "%llu\t", (long long unsigned int) TS);
         fprintf(stdout, "%d\t", within_ROC);
         fprintf(stdout, "%d\t", increment);
         fprintf(stdout, "%lli\t", (long long int) TS - (long long int) lastStartTS);
         fprintf(stdout, "%lli\t", (long long int) TS - (long long int) lastTS);
         fprintf(stdout, "%lli\t",
               (long long int) (((long long int) TS - (long long int) lastStartTS) - arguments->correlation_shift) / arguments->bxid_length);
         fprintf(stdout, "%lli\t",
               (long long int) (((long long int) TS - (long long int) lastStartTS) - arguments->correlation_shift) % arguments->bxid_length);
         fprintf(stdout, "#Trig \n");
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
   if (arguments->start_position < 0) arguments->start_position = 0;
   printf("#finished reading BIF data from raw LDA packets. Processed %d triggers. Last ROC %d\n", bif_data_index, ROC);
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
//   u_int64_t first_shutter = 0;
   int first_shutter_processed = 0;
   u_int64_t last_accepted_trigger_TS = 0LLU;
   unsigned char minibuf[8];
   int within_ROC=0;
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
            trig_details[3] = minibuf[4];                /* input close to RJ45 port */
            trig_details[2] = minibuf[5]; 
            trig_details[1] = minibuf[6]; 
            trig_details[0] = minibuf[7]; /* input close to the clock port */
            /*the fine timestamp needs to be converted to the direct timestamp by subtracting 8 (= adding 24)*/

            finetime_trig = (time << 5) | ((trig_details[arguments->trigger_input & 0x03] + 0x18) & 0x1F);
            if ( (arguments->print_triggers)) {
               fprintf(stdout, "#%05llu\t", (long long unsigned int) shutter_cnt - arguments->start_position);
               fprintf(stdout, "%05d\t", trig_counter);
               fprintf(stdout, "%llu\t", (long long unsigned int) finetime_trig);
               fprintf(stdout, "%d\t", within_ROC);
	       int i;
               for (i=3; i>=0; i--){
                  if ( details & (1<<(8+i))) {fprintf(stdout,"%x",i);} else {fprintf(stdout,".");};
               }
               fprintf(stdout, "\t");
               fprintf(stdout,"%02x %02x %02x %02x",trig_details[3],trig_details[2],trig_details[1],trig_details[0]);
               /* fprintf(stdout, "%d\t", increment); */
               /* fprintf(stdout, "%lli\t", (long long int) TS - (long long int) lastStartTS); */
               /* fprintf(stdout, "%lli\t", (long long int) TS - (long long int) lastTS); */
               /* fprintf(stdout, "%lli\t", (long long int) (((long long int) TS - (long long int) lastStartTS) - arguments->correlation_shift) / arguments->bxid_length); */
               /* fprintf(stdout, "%lli\t", */
               /*         (long long int) (((long long int) TS - (long long int) lastStartTS) - arguments->correlation_shift) % arguments->bxid_length); */
               /* fprintf(stdout, "#Trig\n"); */
               fprintf(stdout,"\n");
            }
            if ( !(details & (1<<(8+arguments->trigger_input)) )) continue;

            if ((finetime_trig - last_accepted_trigger_TS) < ((arguments->aftertrigger_veto * 128) / 100)) continue;
            last_accepted_trigger_TS = finetime_trig;
            if ((shutter_cnt >= arguments->start_position) && (bif_data_index < C_MAX_BIF_EVENTS)) {/*if the data index is in range*/
               bif_data[bif_data_index].ro_cycle = shutter_cnt; // - first_shutter;
               bif_data[bif_data_index].tdc = finetime_trig - (oldtime_fcmd << 5);
               bif_data[bif_data_index++].trig_count = trig_counter;
            }
            break;
         case 2:
            within_ROC = 0;
            //stop acquisition
            oldtime_fcmd = time;
            break;
         case 3:
            within_ROC = 1;
            //"details" variable is treated as shutter_counter (12 bit)
            if ((details_last == 4095) && (details == 0)) {
               /*when the shutter counter overflows, we have to increment the shutter counter properlyew*/
               shutter_cnt += 4096;
            }
            /*use the lower 12 bits from the BIF data directly*/
            shutter_cnt &= 0xFFFFFFFFFFFFF000;
            shutter_cnt |= details;
            if (!first_shutter_processed) {
               first_shutter_processed = 1;
               fprintf(stdout, "#First start acq cycle: %llu", (long long unsigned int) shutter_cnt);
               arguments->start_position += shutter_cnt;
               fprintf(stdout, ", treating as %llu\n", (long long unsigned int) shutter_cnt - arguments->start_position);
            }
            //start acquisition
//				printf("%llu\t%llu\t%llu\t#start acq\n", time, time - oldtime_fcmd, shutter_cnt);
            if (arguments->realign_bif_starts > -100) {
               //time = ((time + arguments->realign_bif_starts) & (0xFFFFFFFFFFFFFFF8LLU));// - arguments->realign_bif_starts ;
               //TODO following procedure does not seem to work correctly. needs further investigation
               u_int64_t c1 = 1LLU;	       //for faster edits
               u_int64_t c2 = 6LLU;	       //for faster edits
               u_int64_t correction1 = (time + ((c1 - arguments->realign_bif_starts) & 0x07LLU)) & 0xffffFFFFffffFFF8LLU;//to align all start acquisition phases to the singled one, which was set by the sync CCC command during the start of the run
               u_int64_t correction2 = ((arguments->realign_bif_starts + c2) & 0x07LLU) - c2;//to realight the start phase-corrected value onto the same AHCAL-BIF offset (otherwise this offset would vary with the phase)
               time = correction1 + correction2;
            }
            if ((time - oldtime_fcmd) > (40000LLU * arguments->report_gaps_ms)) {
               fprintf(stdout, "#Long gap before start: raw=%llu,\tFrom0=%llu,\ttime_s=%f\n",
                     (long long unsigned int) shutter_cnt,
                     (long long unsigned int) (shutter_cnt - arguments->start_position),
                     (25.0E-9) * (time - oldtime_fcmd));
            }
            u_int32_t bif_start_phase = ((u_int32_t) time) & (C_START_PHASES_LENGTH - 1);
            //printf("%d\t%d\t#Phase\n", bif_start_phase, (u_int32_t)shutter_cnt);//DEBUG
            if (bif_start_phase < C_START_PHASES_LENGTH) bif_start_phases[bif_start_phase]++;
            if (((shutter_cnt - arguments->start_position) >= 0) && ((shutter_cnt - arguments->start_position) < C_ROC_READ_LIMIT)) {
               extra_stats[shutter_cnt - arguments->start_position].acq_length = (time - oldtime_fcmd);
               //printf("#ROC: %d\n",shutter_cnt - arguments->start_position);
            }
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

int get_first_iterator(const struct arguments_t * arguments, const BIF_record_t *bif_data, int ROcycle, int bxid) {
   /*perform a quick search in the iterator array*/
//	bif_roc = bif_data[bif_iterator].ro_cycle - arguments->start_position;
//	bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / 5120;
//	bif_tdc = (bif_data[bif_iterator].tdc - arguments->correlation_shift) % 5120;
   int wanted_bif_roc = ROcycle + arguments->start_position;
   u_int64_t wanted_bif_tdc = bxid * arguments->bxid_length + arguments->correlation_shift;
   u_int64_t wanted_bif_tdc_following = wanted_bif_tdc + arguments->bxid_length;
//	printf("Inputs: req.ROC:%d\treq.BXID:%d\t", ROcycle, bxid);
//	printf("WBROC:%llu\tWBTDC:%llu\tWBTDC_F:%llu\n", wanted_bif_roc, wanted_bif_tdc, wanted_bif_tdc_following);
   int start = 0;
   int end = bif_last_record;/**/
   int guess = 0;
   int hit = -1;
   if (arguments->correlation_shift + bxid * arguments->bxid_length < 0) return -1;
//	printf("\n");
   while (start <= end) {
      int half = (end - start) >> 1;

      guess = start + half;
//		printf("%d\t%d\t%d\t%d\t%d\n", start, end, guess, bif_data[guess].ro_cycle, bif_data[guess].trig_count);
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
            if (bif_data[guess].tdc < wanted_bif_tdc) {
               /*previous bxid. not ineterested*/
               start = guess + 1;
            } else {
               if (bif_data[guess].tdc >= wanted_bif_tdc_following) {
                  /*next bxid. not interrested*/
                  end = guess - 1;
               } else {
                  /*same BXID. still can be more events in the same bxid. we need the first*/
                  hit = guess;/*it is a valid result, lets save it and continue searching better*/
                  end = guess - 1; /*and continue searching for earlier events*/
               }
            }
         }
      }
   }
   return hit;
}

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

int correlate_from_txt(const struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_txt_filename, "r"))) {
      perror("Unable to open the spiroc TXT file\n");
      return -1;
   }
   /*spiroc datafile iteration*/
   char * line = NULL;
   size_t len = 0;
   u_int32_t ROcycle = 0;
   int bxid = 0;
   int asic = 0;
   int memcell = 0;
   int channel = 0;
   int tdc, adc, hit, gain = 0;

   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   u_int32_t bif_roc = 0;
   u_int64_t bif_bxid = 0;
   u_int64_t bif_tdc = 0;
   u_int32_t matches = 0;

   int ext_search = 0;

   printf("#ROC\tbxid\tasic\tmcell\ttdc\tadc\thitb\tgainb\tBIF_TDC\text_bxid_ind\tintra_bxid_evet\n");
   while (getline(&line, &len, fp) != -1) {
      if (len > 0) {
         if (line[0] == '#') {
            printf("%s", line); //copy the comments
            continue;
         } else {
//				u_int32_t ROcycle_old = ROcycle;
            //				printf("%s", line);
            sscanf(line, "%d %d %d %d %d %d %d %d %d", &ROcycle, &bxid, &asic, &memcell, &channel, &tdc, &adc, &hit, &gain);
         }
         if ((arguments->require_hitbit == 1) && (hit == 0)) continue; /*skip data without hitbit, if required*/
         if ((arguments->channel != -1) && (channel != arguments->channel)) continue; /*ship data from unwanted channel*/
         if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
         if ((memcell<arguments->minimum_memcell) || (memcell > arguments->maximum_memcell)) continue; /*skip data from unwanted asic*/
         if (bif_iterator >= C_MAX_BIF_EVENTS) break;
         if (ROcycle >= C_ROC_READ_LIMIT) break; /*for debugging: if we do not want to read the while file. */

         ext_search = (0 - arguments->extended_search);
         if (ext_search + bxid < 0) /*fix the beginning of the runs*/
         ext_search = 0;
         for (; ext_search < (1 + arguments->extended_search); ++ext_search) {
            int first_bif_iterator = get_first_iterator(arguments, bif_data, ROcycle, bxid - ext_search);
            if (first_bif_iterator < 0) continue;/*nothing found*/
            int same_bif_bxid_index = 0; /*counts the possible particles in the same bxid*/
            u_int64_t previous_bif_bxid = -1;
//					int bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
            for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
               bif_roc = bif_data[bif_iterator].ro_cycle - arguments->start_position;
               if (bif_roc != ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/

               bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
               if (bif_bxid + ext_search > bxid) break; /*we jumped to another bxid with the bif_iterator*/
               same_bif_bxid_index = (bif_bxid == previous_bif_bxid) ? same_bif_bxid_index + 1 : 0;
               previous_bif_bxid = bif_bxid;

               bif_tdc = (bif_data[bif_iterator].tdc - arguments->correlation_shift) % arguments->bxid_length;
               matches++;
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
               printf("%d\t", hit);
               printf("%d\t", gain);
               printf("%llu\t", (long long unsigned int) bif_tdc); //				printf("%llu\t", bif_data[bif_iterator].tdc);
               printf("%d\t", ((int) bif_bxid) - ((int) bxid));
               printf("%d\t", same_bif_bxid_index);
               printf("\n");
            }
         }
      }
   }
   if (fclose(fp)) {
      perror("Unable to close the file\n");
   }
//	printf("#done at bif cycle %d\n", bif_data[bif_iterator].ro_cycle);
//	printf("#done at SPIROC cycle %d\n", spiroc_iterator / 16);
//	printf("#Matches: %d\n", matches);
   return matches;
}
FILE *of = NULL;/*output file*/
unsigned char buf[4096];

int correlate_from_raw(const struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }

   /*spiroc datafile iteration*/
   int bxid = 0;
   int asic = 0;
   int memcell = 0;
   int channel = 0;
   int tdc, adc, adc_hit, adc_gain = 0;
   u_int32_t ROCLength = 0;

   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   u_int32_t bif_roc = 0;
   int bif_bxid = 0;
   u_int64_t bif_tdc = 0;
   u_int32_t matches = 0;

   int ext_search = 0;

   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   /* int roc_prev = -1; */
   u_int32_t ROcycle = -1;
   if (arguments->debug_constant == 1) {
      printf("#Extra 15: memcells filled in the module\n");
      printf("#Extra 16: memcells filled in the whole detectors\n");
      printf("#Extra 17: last bxid in the asic\n");
      printf("#Extra 18: last bxid in the module\n");
      printf("#Extra 19: last bxid in the whole detector\n");
      printf("#Extra 20: length in BIF tics\n");
      printf("#Extra 21: bxid of previous memory cell\n");
      printf("#Extra 22: bxid of the next memory cell\n");
   }
   printf("# bxid(BIF-DIF): -1 means, that DIF bxid is higher than it should be.\n");
   printf("#1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\t16\t17\t18\t19\t20\n");
   printf("#ROC\tbxid\tasic\tmcell\tchan\ttdc\tadc\thitb\tgainb\tBIF_TDC\tbxid(BIF-DIF)\tintra_bxid_event\tROCLen\tmem_filled\n");

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
      
      int lda_port = (headinfo >> 8) & 0xFF;
      if (lda_port >= C_MAX_PORTS) {
         printf("#ERROR: wrong LDA port: %d\n", lda_port);
         continue;         //wrong port number
      }
      
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 0x80);
      /* if (((headlen >> 16) & 0xFF) != roc_prev) { */
      /*    roc_prev = ((headlen >> 16) & 0xFF); */
      /*    ++ROcycle; */
      /* //			cycles[row_index] = roc_prev; */
      /* } */
//		printf("%05d\t", row_index);
//		printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
//			printf("no spiroc data packet!\n");
         continue;
      }
      unsigned int dif_id=((unsigned int)buf[6]) | (((unsigned int)buf[7])<<8);
      if ((arguments->module>=0) && (arguments->module != dif_id)) continue;
      if ((arguments->lda_port >= 0) && (lda_port != arguments->lda_port)) continue;
//      fprintf(stdout,"#ROC: %d\n",ROcycle);
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      if (((headlen & 0x0fff) - 12) % 146) { //if the length of the packet does not align with the number of memory cells
         printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle, asic);
      }
      if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//		printf("#memory cells: %d\n", memcell_filled);
      ROCLength = get_ROCLength(arguments, bif_data, ROcycle);
      int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
      int max_asic_bxid = buf[8 + 36 * 4 * memcell_filled] | (buf[8 + 36 * 4 * memcell_filled + 1] << 8);
      for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) {
         if (memcell > arguments->maximum_memcell) continue;
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         bxid = grayRecode(bxid);
         if (bxid < arguments->minimum_bxid) continue;
         if (bxid > arguments->maximum_bxid) continue;
         for (channel = 0; channel < 36; ++channel) {
            if ((arguments->channel != -1) && (channel != arguments->channel)) continue;/*ship data from unwanted channel*/

            tdc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1)]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8);
            adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8);
            adc_hit = (adc & 0x1000) ? 1 : 0;
            adc_gain = (adc & 0x2000) ? 1 : 0;
            tdc = tdc & 0x0fff;
            adc = adc & 0x0fff;

            
            /*here is the main correlation loop*/
            if ((arguments->require_hitbit == 1) && (adc_hit == 0)) continue;/*skip data without hitbit, if required*/
            if (bif_iterator >= C_MAX_BIF_EVENTS) break;
            if (ROcycle >= C_ROC_READ_LIMIT) /*for debugging: if we do not want to read the while file. */
               break;
            int bxid_prev = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell )] | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell) + 1] << 8);
            int bxid_next = memcell==(memcell_filled-1)?-1: buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 2)] | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 2) + 1] << 8);

//				if ((memcell == 0) && (channel == 0))
//				printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", ROcycle, bxid, asic, memcell, channel, tdc, adc, hit, gain);
            ext_search = (0 - arguments->extended_search);
            if (ext_search + bxid < 0) /*fix the beginning of the runs*/
            ext_search = 0;
            for (; ext_search < (1 + arguments->extended_search); ++ext_search) {
               if (first_bif_iterator < 0) {
                  continue;/*nothing found*/
               }
//						printf("notning found\n");
               int same_bif_bxid_index = 0; /*counts the possible particles in the same bxid*/
               u_int64_t previous_bif_bxid = -1;
               //					int bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
               for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
                  bif_roc = bif_data[bif_iterator].ro_cycle - arguments->start_position;
                  if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
                  if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/

                  bif_bxid = ((int) bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
                  if (bif_bxid + ext_search > bxid) break; /*we jumped to another bxid with the bif_iterator*/
                  if (bif_bxid + ext_search < bxid) continue;/*not yet in the correct bxid*/
                  same_bif_bxid_index = (bif_bxid == previous_bif_bxid) ? same_bif_bxid_index + 1 : 0;
                  previous_bif_bxid = bif_bxid;

                  bif_tdc = (bif_data[bif_iterator].tdc - arguments->correlation_shift) % arguments->bxid_length;
                  matches++;
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
                  printf("%llu\t", (long long unsigned int) bif_tdc); //				printf("%llu\t", bif_data[bif_iterator].tdc);
                  printf("%d\t", ((int) bif_bxid) - ((int) bxid));
                  printf("%d\t", same_bif_bxid_index);
                  printf("%d\t", ROCLength);
                  printf("%d\t", memcell_filled);
                  if (arguments->debug_constant == 1) {
                     printf("%d\t", extra_stats[ROcycle].cell_modules[lda_port]); //printf("#Extra 15: memcells filled in the module\n");
                     printf("%d\t", extra_stats[ROcycle].cell_detector);          //printf("#Extra 16: memcells filled in the whole detectors\n");
                     printf("%d\t", max_asic_bxid);                               //printf("#Extra 17: last bxid in the asic\n");
                     printf("%d\t", extra_stats[ROcycle].bxid_modules[lda_port]); //printf("#Extra 18: last bxid in the module\n");
                     printf("%d\t", extra_stats[ROcycle].bxid_detector);          //printf("#Extra 19: last bxid in the whole detector\n");
                     printf("%llu\t", (long long unsigned int) extra_stats[ROcycle].acq_length); //printf("#Extra 20: length in BIF tics\n");
                     printf("%d\t",bxid_prev);
                     printf("%d\t",bxid_next);
                     
                  }
                  printf("\n");
               }
            }

         }
      }

//		printf("\n");
   }
   fclose(fp);
   return 0;
}

int analyze_memcell_ocupancy(const struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   printf("DEBUG: Analyzing memory cells and bxid first\n");
   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }
   /*spiroc datafile iteration*/
//   int bxid = 0;
   int asic = 0;
//   int memcell = 0;
//   int channel = 0;
//   int tdc, adc, adc_hit, adc_gain = 0;
//   u_int32_t ROCLength = 0;

   /*BIF iteration variables*/
//   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
//   u_int32_t bif_roc = 0;
//   int bif_bxid = 0;
//   u_int64_t bif_tdc = 0;
//   u_int32_t matches = 0;
//   int ext_search = 0;
   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   /* int roc_prev = -1; */
   u_int32_t ROcycle = -1;

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
      int lda_port = (headinfo >> 8) & 0xFF;
      if (lda_port >= C_MAX_PORTS) {
         printf("#ERROR: wrong LDA port: %d\n", lda_port);
         continue;         //wrong port number
      }
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 0x80);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
//       printf("no spiroc data packet!\n");
         continue;
      }
      if ((arguments->lda_port >= 0) && (lda_port != arguments->lda_port)) continue;
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      if (((headlen & 0x0fff) - 12) % 146) { //if the length of the packet does not align with the number of memory cells
         printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle, asic);
      }
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
      if ((ROcycle >= 0) && (ROcycle < C_ROC_READ_LIMIT)) {
         if (memcell_filled > extra_stats[ROcycle].cell_detector) extra_stats[ROcycle].cell_detector = memcell_filled;
         if (memcell_filled > extra_stats[ROcycle].cell_modules[lda_port]) extra_stats[ROcycle].cell_modules[lda_port] = memcell_filled;
         int maxbxid = buf[8 + 36 * 4 * memcell_filled] | (buf[8 + 36 * 4 * memcell_filled + 1] << 8);
         if (maxbxid > extra_stats[ROcycle].bxid_detector) extra_stats[ROcycle].bxid_detector = maxbxid;
         if (maxbxid > extra_stats[ROcycle].bxid_modules[lda_port]) extra_stats[ROcycle].bxid_modules[lda_port] = maxbxid;
      }
   }
   fclose(fp);
   return 0;
}

int scan_from_raw_channelwise(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   arguments->correlation_shift = 0;
   int max_correlation = arguments->shift_scan;
   int scan[max_correlation];
   int i = 0;
   for (; i < max_correlation; i++) {
      scan[i] = 0;
   }

   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }

   /*spiroc datafile iteration*/
   int bxid = 0;
   int asic = 0;
   int memcell = 0;
   int channel = 0;
   int tdc = 0;
   int adc = 0;
   int adc_hit = 0;
//   int adc_gain = 0;

   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   int bif_roc = 0;
   int bif_bxid = 0;

   int ext_search = 0;

   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   /* int roc_prev = -1; */
   u_int32_t ROcycle = -1;
   printf("# bxid(BIF-DIF): -1 means, that DIF bxid is higher than it should be.\n");
   printf("#1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\n");
   printf("#ROC\tbxid\tasic\tmcell\tchan\ttdc\tadc\thitb\tgainb\tBIF_TDC\tbxid(BIF-DIF)\tintra_bxid_event\tROCLen\n");

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
         printf("#wrong header length: %d", headlen & 0xffff);
         continue;
      }
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 2);
      /* roc_prev = ROcycle; */
      /* if (((headlen >> 16) & 0xFF) != roc_prev) { */
      /*          roc_prev = ((headlen >> 16) & 0xFF); */
      /*          ++ROcycle; */
      /* //       cycles[row_index] = roc_prev; */
      /*       } */
      if (ROcycle >= C_ROC_READ_LIMIT) break;/*for debugging: if we do not want to read the while file. */
//    printf("%05d\t", row_index);
//    printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
//       printf("no spiroc data packet!\n");
         continue;
      }
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//    printf("#memory cells: %d\n", memcell_filled);
      int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
      for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) {
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         bxid = grayRecode(bxid);
         if (memcell > arguments->maximum_memcell) continue;
         for (channel = 0; channel < 36; ++channel) {
            if ((arguments->channel != -1) && (channel != arguments->channel)) continue; /*ship data from unwanted channel*/

            tdc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1)]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8);
            adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8);
            adc_hit = (adc & 0x1000) ? 1 : 0;
//            adc_gain = (adc & 0x2000) ? 1 : 0;
            tdc = tdc & 0x0fff;
            adc = adc & 0x0fff;
            /*here is the main correlation loop*/
            if ((arguments->require_hitbit == 1) && (adc_hit == 0)) continue; /*skip data without hitbit, if required*/
            if (bif_iterator >= C_MAX_BIF_EVENTS) break;

//          if ((memcell == 0) && (channel == 0))
//          printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", ROcycle, bxid, asic, memcell, channel, tdc, adc, hit, gain);
            ext_search = (0 - arguments->extended_search);
            if (ext_search + bxid < 0) { /*fix the beginning of the runs*/
               ext_search = 0;
            }
            for (; ext_search < (1 + arguments->extended_search); ++ext_search) {
//               int first_bif_iterator = 1; //get_first_iterator(arguments, bif_data, ROcycle, bxid - ext_search-3);

               if (first_bif_iterator < 0) {
                  continue;/*nothing found*/
               }
//                printf("notning found\n");
               //             int bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
               for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
                  bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
                  if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
                  if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/
                  bif_bxid = (bif_data[bif_iterator].tdc - 0) / arguments->bxid_length;
                  if ((bif_bxid + ext_search) > (bxid + 1 + max_correlation / arguments->bxid_length)) break; /*we jumped to another bxid with the bif_iterator*/
                  if ((bif_bxid + ext_search) < bxid) continue;
                  int shift = 0;
                  int startindex = arguments->bxid_length * (bif_bxid + ext_search - bxid - 1) + bif_data[bif_iterator].tdc % arguments->bxid_length + 1;
                  int endindex = startindex + arguments->bxid_length;
                  if (startindex < 0) startindex = 0;
                  if (endindex >= max_correlation) endindex = max_correlation;
//                  printf("start: %d\tend: %d\n", startindex, endindex);
                  for (shift = startindex; shift < endindex; shift++) {
                     scan[shift]++;
                  }
//                  for (; shift < max_correlation; shift++) {
//                     bif_bxid = ((int) bif_data[bif_iterator].tdc - shift) / arguments->bxid_length;
////                     same_bif_bxid_index = (bif_bxid == previous_bif_bxid) ? same_bif_bxid_index + 1 : 0;
////                     previous_bif_bxid = bif_bxid;
//                     if (bif_bxid == bxid) {
//                        scan[shift]++;
//                     }
//                  }
               }
            }

         }
      }

//    printf("\n");
   }
   fclose(fp);
   int maxval = -1;
   int maxindex = -1;
   for (i = 0; i < max_correlation; i++) {
      if (scan[i] > maxval) {
         maxindex = i;
         maxval = scan[i];
      }
   }
   printf("#maximum correlation at: %d\thits:%d", maxindex, maxval);
   printf("#correlation scan\n#shift\thits\tnormalized\n");
   for (i = 0; i < max_correlation; i++) {
      printf("%d\t%d\t%f\n", i, scan[i], scan[i] / (1.0 * maxval));
   }
   return 0;
}

int scan_from_raw_bxidwise(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   fprintf(stdout, "#in scan_from_raw_bxidwise\n");
   arguments->correlation_shift = 0;
   int max_correlation = arguments->shift_scan;
   int scan[max_correlation];
   int i = 0;
   for (; i < max_correlation; i++) {
      scan[i] = 0;
   }
   unsigned char BXIDs[4096];
   for (i = 0; i < 4096; i++) {
      BXIDs[i] = 0;
   }

   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }
   fseek(fp, 0, SEEK_SET); //skip timestamp packets

   /*spiroc datafile iteration*/
   int bxid = 0;
   int asic = 0;
   int memcell = 0;

   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   int bif_roc = 0;
   int bif_bxid = 0;

   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   int roc_prev = 0; //-1;
   u_int32_t ROcycle = 0; //-1;

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
         printf("#wrong header length: %d", headlen & 0xffff);
         continue;
      }
      if ((headlen & 0xFFFF) == 0x10) {
         fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip timestamp packets
         //fprintf(stdout, "#TS packet\n");
         continue;
      }
      int lda_port = (headinfo >> 8) & 0xFF;
      if (lda_port >= C_MAX_PORTS) {
         printf("#ERROR: wrong LDA port: %d\n", lda_port);
         continue;         //wrong port number
      }
      //printf("#DEBUG ROC: %d\n",ROcycle);
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
      if (ROcycle != roc_prev) {
         if (roc_prev >= 0) {
            //get correlations for  what is in the BXIDs array
            int first_bif_iterator = get_first_iterator2(arguments, bif_data, roc_prev);
            //fprintf(stdout, ".");
            for (bxid = arguments->minimum_bxid; bxid <= arguments->maximum_bxid; bxid++) {
               if (BXIDs[bxid]) {
                  for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
                     bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
                     if (bif_roc > roc_prev) break; /*we jumped to another readout cycle with the bif_iterator*/
                     if (bif_roc < roc_prev) {
                        /*not yet in the correct readout cycle*/
                        first_bif_iterator = bif_iterator;
                        continue;
                     }
                     bif_bxid = (bif_data[bif_iterator].tdc - 0) / arguments->bxid_length;
                     if ((bif_bxid) > (bxid + 1 + max_correlation / arguments->bxid_length)) break; /*we jumped to another bxid with the bif_iterator*/
                     if ((bif_bxid) < bxid) {
                        /*too old bif events*/
                        first_bif_iterator = bif_iterator;
                        continue;
                     }
                     int startindex = arguments->bxid_length * (bif_bxid - bxid - 1) + bif_data[bif_iterator].tdc % arguments->bxid_length + 1;
                     int endindex = startindex + arguments->bxid_length;
                     if (startindex < 0) startindex = 0;
                     if (endindex >= max_correlation) endindex = max_correlation;
//                  printf("start: %d\tend: %d\n", startindex, endindex);
                     /* int shift = 0; */
                     /* for (shift = startindex; shift < endindex; shift++) { */
                     /*    scan[shift]++; */
                     /* } */
                     /* just marking where the scan data should increase by 1 and decrease by 1 again*/
                     if (startindex<max_correlation) scan[startindex]++;
                     scan[endindex]--;
                  }
               }
               BXIDs[bxid] = 0;
            }
         }

         roc_prev = ROcycle;         //((headlen >> 16) & 0xFF);
         /* ++ROcycle; */
//       cycles[row_index] = roc_prev;
      }
      if (ROcycle >= C_ROC_READ_LIMIT) {
         fprintf(stdout, "#limit C_ROC_READ_LIMIT exceede in ROC %d", ROcycle);
         break;/*for debugging: if we do not want to read the while file. */
      }
//    printf("%05d\t", row_index);
//    printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
//       printf("no spiroc data packet!\n");
         continue;
      }
      if ((arguments->lda_port >= 0) && (lda_port != arguments->lda_port)) continue;
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//    printf("#memory cells: %d\n", memcell_filled);
      for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) {
         if (memcell > arguments->maximum_memcell) continue;
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         bxid = grayRecode(bxid);
         BXIDs[bxid & 0x0FFF] = 1;
      }

//    printf("\n");
   }
   printf("#Final ROC: %d:\n", ROcycle);
   fclose(fp);
   int running_sum=0;
   for (i=0 ; i<max_correlation ; i++){//getting the number of correlations
      running_sum += scan[i];
      scan[i]=running_sum;
   }
   fprintf(stdout, "#maximum ROCycle: %d", ROcycle);
   int maxval = -1;
   int maxindex = -1;
   for (i = 0; i < max_correlation; i++) {
      if (scan[i] > maxval) {
         maxindex = i;
         maxval = scan[i];
      }
   }
   printf("#maximum correlation at: %d\thits:%d\n", maxindex, maxval);
   printf("#correlation scan\n#shift\thits\n");
   for (i = 0; i < max_correlation; i++) {
      printf("%d\t%d\t%f\n", i, scan[i], (1.0 * scan[i]) / maxval);
   }
   return 0;
}

int scan_from_raw_asicwise(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   arguments->correlation_shift = 0;
   int max_correlation = arguments->shift_scan;
   int scan[max_correlation];
   int i = 0;
   for (; i < max_correlation; i++) {
      scan[i] = 0;
   }
   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }
   /*spiroc datafile iteration*/
   int bxid = 0;
   int asic = 0;
   int memcell = 0;
   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   int bif_roc = 0;
   int bif_bxid = 0;
   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
//   int roc_prev = -1;
   int32_t ROcycle = 0;
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
         printf("#wrong header length: %d", headlen & 0xffff);
         continue;
      }
      int lda_port = (headinfo >> 8) & 0xFF;
      if (lda_port >= C_MAX_PORTS) {
         printf("#ERROR: wrong LDA port: %d\n", lda_port);
         continue;         //wrong port number
      }
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
      /* printf("#DEBUG ROC: %d\n",ROcycle); */
      if (ROcycle >= C_ROC_READ_LIMIT) break;/*for debugging: if we do not want to read the while file. */
//    printf("%05d\t", row_index);
//    printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((arguments->lda_port >= 0) && (lda_port != arguments->lda_port)) continue;
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
//       printf("no spiroc data packet!\n");
         continue;
      }
      unsigned int dif_id=((unsigned int)buf[6]) | (((unsigned int)buf[7])<<8);
      if ((arguments->module>=0) && (arguments->module != dif_id)) continue;
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//    printf("#memory cells: %d\n", memcell_filled);
      int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
      for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) {
         if (memcell > arguments->maximum_memcell) continue;
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         bxid = grayRecode(bxid);
         for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
            bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
            if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
            if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/
            bif_bxid = (bif_data[bif_iterator].tdc - 0) / arguments->bxid_length;
            if ((bif_bxid) > (bxid + 1 + max_correlation / arguments->bxid_length)) break; /*we jumped to another bxid with the bif_iterator*/
            if ((bif_bxid) < bxid) continue;
            int startindex = arguments->bxid_length * (bif_bxid - bxid - 1) + bif_data[bif_iterator].tdc % arguments->bxid_length + 1;
            int endindex = startindex + arguments->bxid_length;
            if (startindex < 0) startindex = 0;
            if (endindex >= max_correlation) endindex = max_correlation;
//                  printf("start: %d\tend: %d\n", startindex, endindex);
            /* int shift = 0; */
            /* for (shift = startindex; shift < endindex; shift++) { */
            /*    scan[shift]++; */
            /* } */
            if (startindex<max_correlation) scan[startindex]++;
            scan[endindex]--;
         }
      }

//    printf("\n");
   }
   fclose(fp);   
   int running_sum=0;
    for (i=0 ; i<max_correlation ; i++){//getting the number of correlations
      running_sum += scan[i];
      scan[i]=running_sum;
   }
   int maxval = -1;
   int maxindex = -1;
   for (i = 0; i < max_correlation; i++) {
      if (scan[i] > maxval) {
         maxindex = i;
         maxval = scan[i];
      }
   }
   printf("#maximum correlation at: %d\thits:%d\n", maxindex, maxval);
   printf("#correlation scan\n#shift\thits\tnormalized\n");
   for (i = 0; i < max_correlation; i++) {
      printf("%d\t%d\t%f\n", i, scan[i], scan[i] / (1.0 * maxval));
   }
   return 0;
}

int trigger_spacing_from_raw_bif(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
//   int max_spacing = arguments->bxid_length*arguments->trigger_spacing;

   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   int bif_roc = 0;
   int roc_prev = -1;
   int last_bxid = 0;

   u_int64_t last_TS = 0;
   u_int64_t new_TS = 0;

   printf("#trigger distances\n");
   printf("#bxid_distance\tTS_distance\n");
   int globaldistance = 1;
   for (bif_iterator = 0; bif_iterator <= bif_last_record; ++bif_iterator) {
      if (globaldistance) {
         new_TS = bif_data[bif_iterator].tdc;
         if ((new_TS - last_TS) < 400000000LLU * 32) //if distance from last trigger is less than 10s
         printf("%llu\n", (long long unsigned) (new_TS - last_TS));
         //fixed already in bif data parse. if ((new_TS - last_TS)< 600LLU ) continue ;//to fix the retriggering TLU issue. does not store the old TS
      } else {
         bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
         if (bif_roc != roc_prev) {
            last_bxid = 0;
            last_TS = bif_data[bif_iterator].tdc - arguments->correlation_shift; //0;
            roc_prev = bif_roc;
         }
         if (bif_data[bif_iterator].tdc < (arguments->correlation_shift + arguments->minimum_bxid * arguments->bxid_length)) continue;
         new_TS = bif_data[bif_iterator].tdc - arguments->correlation_shift;
         int new_bxid = new_TS / arguments->bxid_length;
         if ((new_bxid - last_bxid) < 4000) {
            printf("%d\t%llu\n", new_bxid - last_bxid, (long long unsigned) (new_TS - last_TS));
         }
         last_bxid = new_bxid;
      }
      last_TS = new_TS;
   }
   return 0;
}

int ahcal_bxid_spacing_scan(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   int i = 0;
   unsigned char BXIDs[4096];
   for (i = 0; i < 4096; i++) {
      BXIDs[i] = 0;
   }

   FILE *fp;
   if (!(fp = fopen(arguments->spiroc_raw_filename, "r"))) {
      perror("Unable to open the spiroc raw file\n");
      return -1;
   }

   /*spiroc datafile iteration*/
   int bxid = 0;
   int asic = 0;
   int memcell = 0;

   /*BIF iteration variables*/
//   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
//   int bif_roc = 0;
//   int bif_bxid = 0;
   unsigned int headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   int roc_prev = -1;
   u_int32_t ROcycle = -1;

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
         printf("#wrong header length: %d", headlen & 0xffff);
         continue;
      }

      if (((headlen >> 16) & 0xFF) != roc_prev) {
         if (ROcycle >= 0) {
            //get correlations for  what is in the BXIDs array
            int lastbxid = 0; //dummy trigger
            for (bxid = arguments->minimum_bxid; bxid <= arguments->maximum_bxid; bxid++) {
               if (BXIDs[bxid]) {
                  BXIDs[bxid] = 0;
                  if (arguments->bxid_spacing == 2) {/*in mode 1 the AHCAL BXIDs have to pass the BIF correlation criteria*/
                     int correlated = 0;
                     int bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
                     for (; bif_iterator <= bif_last_record; ++bif_iterator) {
                        int bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
                        if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
                        if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/
                        int bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
                        if (bif_bxid > bxid) break; /*we jumped to another bxid with the bif_iterator*/
                        if (bif_bxid < bxid) continue; /*too old bif events*/
                        correlated = 1;
                        break;
                     }
                     if (!correlated) continue;
                  }
                  if (arguments->bxid_spacing == 3) { //we want to print only distance from last correlated hit
                     int correlated = 0;
                     int bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
                     for (; bif_iterator <= bif_last_record; ++bif_iterator) {
                        int bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
                        if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
                        if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/
                        int bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length;
                        if (bif_bxid > bxid) break; /*we jumped to another bxid with the bif_iterator*/
                        if (bif_bxid < bxid) continue; /*too old bif events*/
                        correlated = 1;
                        break;
                     }
                     if (correlated) {
                        lastbxid = bxid;
                        continue;
                     } else {
                        if (lastbxid == 0) continue; //do not print distance to first uncorrelated trigger.
                        printf("%d\n", bxid - lastbxid);
                        continue;
                     }
                  }
                  printf("%d\n", bxid - lastbxid);
                  lastbxid = bxid;
               }
            }
         }

         roc_prev = ((headlen >> 16) & 0xFF);
         ++ROcycle;
//       cycles[row_index] = roc_prev;
      }
      if (ROcycle >= C_ROC_READ_LIMIT) break;/*for debugging: if we do not want to read the while file. */
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#ERROR unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
         /*most probably temperature readout packet*/
         //printf("#ERROR not a spiroc data packet!\n");
         continue;
      }
      asic = buf[(headlen & 0x0FFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet
      if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /*skip data from unwanted asic*/
      if (((headlen & 0x0FFF) - 12) % 146) {
         printf("#ERROR in ROC %05d, asic %03d - wrong length: %d. difference modulo 146: %d\n", ROcycle, asic, (headlen & 0x0FFF),
               ((headlen & 0x0FFF) - 12) % 146);
         printf("0\n");
         continue;
      }
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//    printf("#memory cells: %d\n", memcell_filled);
      printf("#cells:%d\tROC:%d\trawROC:%02X\tbxids: ", memcell_filled, ROcycle, (headlen >> 16) & 0xFF);
      for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) {
         if (memcell > arguments->maximum_memcell) continue;
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         bxid = grayRecode(bxid);
         if ((bxid == 0) || (bxid > 4096)) {
            printf("\n# ERROR! BXID %d (0x%04X) found in ROC %05d asic %03d memcell %02d (of %d). Header: 0x%04x%04x\n", bxid, bxid, ROcycle, asic, memcell,
                  memcell_filled - 1, headinfo, headlen);
            printf("# ERROR raw out:        ");
            int debugi = 0;
            for (debugi = 0; debugi < (headlen & 0xFFF); debugi = debugi + 2) {
               printf(" %04x", buf[debugi] | (buf[debugi + 1] << 8));
            }
            printf("\n");
         }
         BXIDs[bxid & 0x0FFF] = 1;
         printf("#%d\t", bxid);
      }
      printf("\n");

//    printf("\n");
   }
   fclose(fp);
   return 0;
}

void print_bif_phases() {
   int i = 0;
   printf("#phase\tcounts\n");
   for (; i < C_START_PHASES_LENGTH; i++) {
      printf("%d\t%d\n", i, bif_start_phases[i]);
   };
}

int main(int argc, char **argv) {
   /* Default values. */
   arguments_init(&arguments);

   bif_data = (BIF_record_t*) malloc(sizeof(BIF_record_t) * C_MAX_BIF_EVENTS);
   bif_start_phases = (u_int32_t*) malloc(sizeof(int) * C_START_PHASES_LENGTH);
   extra_stats = (extra_stats_t*) malloc(sizeof(extra_stats_t) * C_ROC_READ_LIMIT);

   /* Parse our arguments; every option seen by parse_opt will
    be reflected in arguments. */
//error_t parseret =
   argp_parse(&argp, argc, argv, 0, 0, &arguments);
   if (arguments.output_file != NULL) {
      of = fopen(arguments.output_file, arguments.forced ? "w" : "wx");
   } else {
      of = stdout;
   }
//   fprintf(of, "hi\n");
   arguments_print(&arguments);

   if (arguments.trig_data_from_spiroc_raw == 1) {
      load_timestamps_from_ahcal_raw(&arguments, bif_data, &bif_last_record);
   } else {
      load_bif_data(&arguments, bif_data, &bif_last_record);
   }
   if (arguments.shift_scan > 0) {
      printf("#correlation scan from raw\n");
      switch (arguments.shift_scan_method) {
         case 0:
            scan_from_raw_bxidwise(&arguments, bif_data, bif_last_record);
            break;
         case 1:
            scan_from_raw_asicwise(&arguments, bif_data, bif_last_record);
            break;
         case 2:
            scan_from_raw_channelwise(&arguments, bif_data, bif_last_record);
            break;
         default:
            break;
      }
      goto final;
   }
   if (arguments.bif_trigger_spacing > 0) {
      trigger_spacing_from_raw_bif(&arguments, bif_data, bif_last_record);
      goto final;
   }
   if (arguments.bxid_spacing > 0) {
      ahcal_bxid_spacing_scan(&arguments, bif_data, bif_last_record);
      goto final;
   }
   if (arguments.print_bif_start_phases) {
      print_bif_phases();
      goto final;
   }
   if (arguments.spiroc_raw_filename != NULL) {
      if (arguments.debug_constant == 1) analyze_memcell_ocupancy(&arguments, bif_data, bif_last_record);
      correlate_from_raw(&arguments, bif_data, bif_last_record);
      goto final;
   }
   if (arguments.spiroc_txt_filename != NULL) {
      correlate_from_txt(&arguments, bif_data, bif_last_record);
      goto final;
   }

//	printf("ARG1 = %s\nARG2 = %s\nOUTPUT_FILE = %s\n"
//			"VERBOSE = %s\nSILENT = %s\n",
//	        arguments.args[0], arguments.args[1],
//	        arguments.output_file,
//	        arguments.verbose ? "yes" : "no",
//	        arguments.silent ? "yes" : "no");
   final:
   free(bif_data);
   free(bif_start_phases);
   free(extra_stats);
   exit(0);
}
