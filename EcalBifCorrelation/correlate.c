#include <stdlib.h>
#include <argp.h> /*parsing arguments*/
const int C_ROC_READ_LIMIT = 1000000;/*for debugging - reading only few RO cycles*/
const int C_MAX_BIF_EVENTS = 5000000;
#define C_MAX_PORTS 48
//const int C_MAX_ROCYCLES = 1000;
#define SINGLE_SKIROC_EVENT_SIZE (129*2)

const int C_MEM_CELLS = 16;

/* Program documentation. */
static char doc[] = "BIF vs SPIROC data correlation tool. For more info try --help";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options will be parsed. */
static struct argp_option options[] =
      {
            /* { "spiroc_txt_file", 's', "SPIROC_FILE", 0, "filename/path of the Spiroc TXT file produced by the labview" }, */
            { "ecal_raw_file", 'w', "ECAL_RAW_FILE", 0, "filename/path of the Spiroc RAW file saved by EUDAQ" },
            { "bif_raw_file", 'b', "BIF_FILE", 0, "filename/path of the BIF raw data file" },
            /* { "trig_data_from_spiroc_raw", 'd', 0, 0, "read the timestamps from the AHCAL raw data" }, */
            /* { "output_file", 'o', "OUTPUT_FILE", 0, "!!!NOT IMPLEMENTED!!! filename to which the data should be written" }, */
            /* { "force", 'f', 0, 0, "Force overwrite" }, */
            /* { "asic", 'a', "ASIC_NUMBER", 0, "asic number (must be < 256)" }, */
            /* { "channel", 'c', "CHANNEL_NUMBER", 0, "channel number to be correlated" }, */
            /* { "memcell", 'm', "MEMCELL_NUMBER", 0, "memory cell to only take into account (0..15). Will set minimum and maximum to this value" }, */
            /* { "minimum_memcell", 262, "MEMCELL_NUMBER", 0, "lowest memory cell to take into account (0..15)" }, */
            /* { "maximum_memcell", 263, "MEMCELL_NUMBER", 0, "highest memory cell to take into account (0..15)" }, */
            /* { "extended_search", 'x', "BXID_TOLERANCE", 0, "extended search in +-BIXID_TOLERANCE BXIDs. Useful only for investigating BXID errors. Default 0" }, */
            { "start_position", 't', "START_POSITION", 0, "Start ROC offset in the BIF data. (=-1 when BIF starts from 0 and AHCAL from 1)" },
            { "correlation_shift", 'r', "RELATIVE_TIMESTAMP", 0, "Correlation timestamp, which synchronizes BXIDs between BIF and SPIROC. Default:13448" },
            { "shift_scan_max", 'n', "SHIFT_SCAN_MAX", 0, "Do the scan for offset values between AHCAL and BIF from 0 to the given maximum. Results are calculated event-wise (every BXID is counted only once for the whole detector). Default:-1, maximum: 2000000" },
            { "shift_scan_method", 'e', "N", 0, "Which method to use for the offset scan:\n0=BXIDs-wise (same BXID from multiple asics are counted only once)\n1=ASIC-wise: each correlated events is summed up individually (same BXID in 2 chips is counted twice). Fastest.\n2=channel-wise: only specified channel is used. Default:0" },
            /* { "bif_trigger_spacing", 'g', 0, 0, "print the time distance between particles as BXID and timestamp differences. Correct offset should be given" }, */
            /* { "minimum_bxid", 257, "MIN_BXID", 0, "BXIDs smaller than MIN_BXID will be ignored. Default:1" }, */
            /* { "maximum_bxid", 258, "MAX_BXID", 0, "BXIDs greater than MAX_BXID will be ignored. Default:4095" }, */
            /* { "bxid_spacing", 'i', "MODE", 0, "print the bxid distance in AHCAL data.. Mode:\n1=print all distances\n2=print distances of correlated BXID\n3=print only uncorrelated distances (from last correlated bxid)" }, */
            { "bxid_length", 'l', "LENGTH", 0, "length of the BXID in BIF tics (to convert from ns: multiply by 1.28)" },
            { "require_hitbit", 'h', 0, 0, "Filter only the hitbit data" },
            { "aftertrigger_veto", 'v', "VETO_LENGTH", 0,"Period in ns after the last trigger, where another triggers are vetoed. Useful to filter glitches on trigger line" },
            /* { "report_gaps_ms", 'q', "LENGTH_MS", 0, "Report readout cycles, that have larger gaps before start (to detect temperature readouts and starts)" }, */
            /* { "realign_bif_starts", 'z', "CLOCK_PHASE", 0,"Realign the clock phase in the BIF data to desired phase (0..7). Use value <-99 to skip the realignment (or don't use this parameter at all)" }, */
            /* { "print_bif_start_phases", 'p', 0, 0, "simply print histogram of last 3 bits of the start acquisition commands seen by BIF" }, */
            { "run_number", 'u', "RUN_NUMBER", 0, "Run number used for the prints" },
            /* { "debug_constant", 'k', "VALUE", 0, "Debug constant for various purposes." }, */
            /* { "print_triggers", 'y', 0, 0, "prints trigger details." }, */
            { "trigger_input", 259, "NUMBER",0,"use this BIF trigger number. Default:3 (close to RJ45)" },
            /* { "module", 260, "NUMBER",0,"use only this module number. NOT IMPLEMENTED" }, */
            /* { "lda_port", 261, "NUMBER",0,"use only this port number." }, */
            /* { "print_maxoffset", 264, 0, 0, "prints the offset scan maxvalue instead of full scan" }, */
	    /* { "add_trigger_numbers", 265, 0, 0, "adds run_number and trigger_number to the data" }, */
	    //			{ "module_positions", 266, "NUMBER", 0, "calculates position within module. 1=1HBU, 2=2x2HBUs" },
            { 0 } };

/* Used by main to communicate with parse_opt. */
struct arguments_t {
   /* char *spiroc_txt_filename; */
   char *ecal_raw_filename;
   char *bif_filename;
   /* char *output_file; */
   /* int forced; /\*forced file overwrite*\/ */
   /* int asic; */
   /* int channel; */
   /* int memcell; */
   /* int extended_search; */
   int start_position;
   int correlation_shift;
   int shift_scan;
   int shift_scan_method;
   /* int bif_trigger_spacing; */
   /* int minimum_bxid; */
   /* int maximum_bxid; */
   /* int bxid_spacing; */
   int bxid_length;
   int require_hitbit;
   /* int trig_data_from_spiroc_raw; */
   int aftertrigger_veto;
   /* int report_gaps_ms; */
   /* int print_bif_start_phases; */
   /* int realign_bif_starts; */
   int run_number;
   /* int debug_constant; */
   /* int print_triggers; */
   int trigger_input;
   /* int module; */
   /* int lda_port; */
   /* int minimum_memcell; */
   /* int maximum_memcell; */
   /* int print_maxoffset; */
   /* int add_trigger_numbers; */
   /* int module_positions; */
};
struct arguments_t arguments;

void arguments_init(struct arguments_t* arguments) {
   /* Default values. */
   /* arguments->spiroc_txt_filename = NULL; */
   arguments->ecal_raw_filename = NULL;
   arguments->bif_filename = NULL;
   /* arguments->output_file = NULL; */
   /* arguments->forced = 0; */
   /* arguments->asic = -1; */
   /* arguments->channel = -1; */
   //arguments->memcell = -1;
   /* arguments->extended_search = 0; */
   arguments->start_position = 0;
   arguments->correlation_shift = 13448;
   arguments->shift_scan = -1;
   arguments->shift_scan_method = 1;
   arguments->require_hitbit = 0;
   /* arguments->minimum_bxid = 1;/\*by default ship BXID 0*\/ */
   /* arguments->maximum_bxid = 4095; */
   /* arguments->bif_trigger_spacing = -1; */
   arguments->bxid_length = 256;
   /* arguments->bxid_spacing = -1; */
   /* arguments->trig_data_from_spiroc_raw = 0; */
   arguments->aftertrigger_veto = 0;
   /* arguments->report_gaps_ms = 500; */
   /* arguments->print_bif_start_phases = 0; */
   /* arguments->realign_bif_starts = -100; */
   arguments->run_number = -1;
   /* arguments->debug_constant = -1; */
   /* arguments->print_triggers = 0; */
   arguments->trigger_input = 0;
   /* arguments->module = -1; */
   /* arguments->lda_port = -1; */
   /* arguments->minimum_memcell = 0; */
   /* arguments->maximum_memcell = 15; */
   /* arguments->print_maxoffset = 0; */
   /* arguments->add_trigger_numbers = 0; */
   /* arguments->module_positions = 0; */
}

void arguments_print(struct arguments_t* arguments) {
   printf("#Run number=%d\n", arguments->run_number);
   /* printf("#SPIROC_TXT_data_file=\"%s\"\n", arguments->spiroc_txt_filename); */
   printf("#ECAL_RAW_data_file=\"%s\"\n", arguments->ecal_raw_filename);
   printf("#BIF_data_file=\"%s\"\n", arguments->bif_filename);
   /* printf("#Output_file=\"%s\"\n", arguments->output_file); */
   /* printf("#Forced_overwrite=\"%s\"\n", arguments->forced ? "yes" : "no"); */
   /* printf("#ASIC_number=%d\n", arguments->asic); */
   /* printf("#Channel_number=%d\n", arguments->channel); */
//   printf("#memory_cell=%d\n", arguments->memcell);
   /* printf("#minimum_memcell=%d\n", arguments->minimum_memcell); */
   /* printf("#maximum_memcell=%d\n", arguments->maximum_memcell); */
   /* printf("#Extended_search=%d\n", arguments->extended_search); */
   printf("#BIF_start_position=%d\n", arguments->start_position);
   printf("#Correlation_shift=%d\n", arguments->correlation_shift);
   printf("#Shift_scan_max=%d\n", arguments->shift_scan);
   printf("#Shift_scan_method=%d\n", arguments->shift_scan_method);
   printf("#REQUIRE_Hitbit=%s\n", arguments->require_hitbit ? "yes" : "no");
   /* printf("#Trigger_spacing=%d\n", arguments->bif_trigger_spacing); */
   /* printf("#BXID_spacing=%d\n", arguments->bxid_spacing); */
   printf("#BXID_length=%d\n", arguments->bxid_length);
   /* printf("#Minimum BXID=%d\n", arguments->minimum_bxid); */
   /* printf("#Maximum BXID=%d\n", arguments->maximum_bxid); */
   /* printf("#trig_data_from_spiroc_raw=%d\n", arguments->trig_data_from_spiroc_raw); */
   printf("#aftertrigger_veto=%d\n", arguments->aftertrigger_veto);
   /* printf("#report_gaps_ms=%d\n", arguments->report_gaps_ms); */
   /* printf("#print_bif_start_phases=%d\n", arguments->print_bif_start_phases); */
   /* printf("#realign_bif_starts=%d\n", arguments->realign_bif_starts); */
   /* printf("#Debug_constant=%d\n", arguments->debug_constant); */
   /* printf("#Print_triggers=%d\n", arguments->print_triggers); */
   printf("#trigger_input=%d\n", arguments->trigger_input);
   /* printf("#module=%d\n", arguments->module); */
   /* printf("#lda_port=%d\n", arguments->lda_port); */
   /* printf("#print_maxoffset=%d\n", arguments->print_maxoffset); */
   /* printf("#add_trigger_numbers=%d\n", arguments->add_trigger_numbers); */
   /* printf("#module_positions=%d\n", arguments->module_positions); */
}

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
   /* Get the input argument from argp_parse, which we
    know is a pointer to our arguments structure. */
   struct arguments_t *arguments = state->input;

   switch (key) {
      /* case 'a': */
      /*    arguments->asic = atoi(arg); */
      /*    break; */
      case 'b':
         arguments->bif_filename = arg;
         break;
      /* case 'c': */
      /*    arguments->channel = atoi(arg); */
      /*    break; */
      /* case 'd': */
      /*    arguments->trig_data_from_spiroc_raw = 1; */
      /*    break; */
      case 'e':
         arguments->shift_scan_method = atoi(arg);
         break;
      /* case 'f': */
      /*    arguments->forced = 1; */
      /*    break; */
      /* case 'g': */
      /*    arguments->bif_trigger_spacing = 1; */
      /*    break; */
      case 'h':
         arguments->require_hitbit = 1;
         break;
      /* case 'i': */
      /*    arguments->bxid_spacing = atoi(arg); */
      /*    break; */
      /* case 257: */
      /*    arguments->minimum_bxid = atoi(arg); */
      /*    break; */
      /* case 258: */
      /*    arguments->maximum_bxid = atoi(arg); */
      /*    break; */
      case 259:
         arguments->trigger_input = atoi(arg);
         break;
      /* case 260: */
      /*    arguments->module = atoi(arg); */
      /*    break; */
      /* case 261: */
      /*    arguments->lda_port = atoi(arg); */
      /*    break; */
      /* case 262: */
      /*    arguments->minimum_memcell = atoi(arg); */
      /*    break; */
      /* case 263: */
      /*    arguments->maximum_memcell = atoi(arg); */
      /*    break; */
      /* case 264: */
      /*    arguments->print_maxoffset = 1; */
      /*    break; */
      /* case 265: */
      /*    arguments->add_trigger_numbers = 1; */
      /*    break; */
      /* case 266: */
      /* 	 arguments->module_positions = atoi(arg); */
      /* 	 break; */
      /* case 'k': */
      /*    arguments->debug_constant = atoi(arg); */
      /*    break; */
      case 'l':
         arguments->bxid_length = atoi(arg);
         break;
      /* case 'm': */
      /*    arguments->minimum_memcell = atoi(arg); */
      /*    arguments->maximum_memcell = arguments->minimum_memcell; */
      /*    break; */
      case 'n':
         arguments->shift_scan = atoi(arg);
         break;
      /* case 'o': */
      /*    arguments->output_file = arg; */
      /*    break; */
      /* case 'p': */
      /*    arguments->print_bif_start_phases = 1; */
      /*    break; */
      /* case 'q': */
      /*    arguments->report_gaps_ms = atoi(arg); */
      /*    break; */
      case 'r':
         arguments->correlation_shift = atoi(arg);
         break;
      /* case 's': */
      /*    arguments->spiroc_txt_filename = arg; */
      /*    break; */
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
         arguments->ecal_raw_filename = arg;
         break;
      /* case 'x': */
      /*    arguments->extended_search = atoi(arg); */
      /*    break; */
      /* case 'y': */
      /*    arguments->print_triggers = 1; */
      /*    break; */
      /* case 'z': */
      /*    arguments->realign_bif_starts = atoi(arg); */
      /*    break; */
      case ARGP_KEY_END:
         if (arguments->ecal_raw_filename == NULL) {
            argp_error(state, "missing ECAL data filename!\n");
         }
         if (arguments->bif_filename == NULL) {
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

/* static const int C_START_PHASES_LENGTH = 8; //must be power of 2 !!! otherwise modulo will not work */
/* u_int32_t *bif_start_phases; //[];//={0,0,0,0,0,0,0,0}; */

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

FILE *of = NULL;/*output file*/
unsigned char buf[4096];

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
   /* int within_ROC=0; */
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
            /* within_ROC = 0; */
            //stop acquisition
            oldtime_fcmd = time;
            break;
         case 3:
            /* within_ROC = 1; */
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
               fprintf(stdout, "#start=0x%x, cnt=0x%lx\n", arguments->start_position,shutter_cnt);
               fprintf(stdout, "#First start acq cycle: %llu", (long long unsigned int) shutter_cnt);
               arguments->start_position += shutter_cnt;
               fprintf(stdout, ", treating as %llu\n", (long long unsigned int) shutter_cnt - (long long unsigned int) arguments->start_position);
               fprintf(stdout, "#start=0x%x, cnt=0x%lx\n", arguments->start_position,shutter_cnt);
            }
            //start acquisition
//				printf("%llu\t%llu\t%llu\t#start acq\n", time, time - oldtime_fcmd, shutter_cnt);
            /* u_int32_t bif_start_phase = ((u_int32_t) time) & (C_START_PHASES_LENGTH - 1); */
            //printf("%d\t%d\t#Phase\n", bif_start_phase, (u_int32_t)shutter_cnt);//DEBUG
            /* if (bif_start_phase < C_START_PHASES_LENGTH) bif_start_phases[bif_start_phase]++; */
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

int cycleIDDecoding(unsigned char *buf) {
   int i = 0;
   int n = 0;
   // metadata
   int result = 0;
   for (n = 0; n < 16; n++) {
      result += ((unsigned int) (((buf[2 * n + 1 + 2] & 0xC0) >> 6) << (30 - 2 * i)));
      i++;
   }
//   std::cout << "cycleIDDecoding:" << dec << result << std::endl;
   i = 0;
   n = 0;
   return result;
}

int Convert_FromGrayToBinary(int grayValue, int nbOfBits) {
   // converts a Gray integer of nbOfBits bits to a decimal integer.
   int binary, grayBit, binBit;
   binary = 0;
   grayBit = 1 << (nbOfBits - 1);
   binary = grayValue & grayBit;
   binBit = binary;
   while (grayBit >>= 1)
   {
      binBit >>= 1;
      binary |= binBit ^ (grayValue & grayBit);
      binBit = binary & grayBit;
   }
   return binary;
}

/* int scan_from_raw_channelwise(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) { */
/*    arguments->correlation_shift = 0; */
/*    int max_correlation = arguments->shift_scan; */
/*    int scan[max_correlation]; */
/*    int i = 0; */
/*    for (; i < max_correlation; i++) { */
/*       scan[i] = 0; */
/*    } */
/*    FILE *fp; */
/*    if (!(fp = fopen(arguments->ecal_raw_filename, "r"))) { */
/*       perror("Unable to open the spiroc raw file\n"); */
/*       return -1; */
/*    } */
/*    /\*spiroc datafile iteration*\/ */
/*    int bxid = 0; */
/*    int asic = 0; */
/*    int memcell = 0; */
/*    int channel = 0; */
/*    int tdc = 0; */
/*    int adc = 0; */
/*    int adc_hit = 0; */
/* //   int adc_gain = 0; */
/*    /\*BIF iteration variables*\/ */
/*    u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger */
/*    int bif_roc = 0; */
/*    int bif_bxid = 0; */
/*    int ext_search = 0; */
/*    unsigned int headlen, headinfo; */
/*    unsigned char b; */
/*    int freadret; //return code */
/*    /\* int roc_prev = -1; *\/ */
/*    u_int32_t ROcycle = -1; */
/*    printf("# bxid(BIF-DIF): -1 means, that DIF bxid is higher than it should be.\n"); */
/*    printf("#1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\n"); */
/*    printf("#ROC\tbxid\tasic\tmcell\tchan\ttdc\tadc\thitb\tgainb\tBIF_TDC\tbxid(BIF-DIF)\tintra_bxid_event\tROCLen\n"); */
/*    while (1) { */
/*       freadret = fread(&b, sizeof(b), 1, fp); */
/*       if (!freadret) { */
/*          printf("#unable to read / EOF\n"); */
/*          break; */
/*       } */
/*       if (b != 0xCD) continue;/\*try to look for first 0xCD. restart if not found*\/ */
/*       freadret = fread(&b, sizeof(b), 1, fp); */
/*       if (!freadret) { */
/*          perror("#unable to read / EOF\n"); */
/*          break; */
/*       } */
/*       if (b != 0xCD) continue;/\*try to look for second 0xCD. restart if not found*\/ */
/*       freadret = fread(&headlen, sizeof(headlen), 1, fp); */
/*       freadret = fread(&headinfo, sizeof(headinfo), 1, fp); */
/*       if (((headlen & 0xFFFF) > 4095) || ((headlen & 0xFFFF) < 4)) { */
/*          printf("#wrong header length: %d\n", headlen & 0xffff); */
/*          continue; */
/*       } */
/*       int lda_port = (headinfo >> 8) & 0xFF; */
/*       if ((lda_port == 0xA0) || (lda_port == 0x80)) { */
/*          fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip timestamp packets */
/*          continue; */
/*       } */
/*       if (lda_port >= C_MAX_PORTS) { */
/*          printf("#ERROR: wrong LDA port: %d\n", lda_port); */
/*          continue;         //wrong port number */
/*       } */
/*       ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100); */
/*       /\* roc_prev = ROcycle; *\/ */
/*       /\* if (((headlen >> 16) & 0xFF) != roc_prev) { *\/ */
/*       /\*          roc_prev = ((headlen >> 16) & 0xFF); *\/ */
/*       /\*          ++ROcycle; *\/ */
/*       /\* //       cycles[row_index] = roc_prev; *\/ */
/*       /\*       } *\/ */
/*       if (ROcycle >= C_ROC_READ_LIMIT) break;/\*for debugging: if we do not want to read the while file. *\/ */
/* //    printf("%05d\t", row_index); */
/* //    printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF); */
/*       freadret = fread(buf, headlen & 0xFFF, 1, fp); */
/*       if (!freadret) { */
/*          printf("#unable to read the complete packet / EOF\n"); */
/*          break; */
/*       } */
/*       if ((arguments->lda_port >= 0) && (lda_port != arguments->lda_port)) continue; */
/*       if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) { */
/* //       printf("no spiroc data packet!\n"); */
/*          continue; */
/*       } */
/*       unsigned int dif_id=((unsigned int)buf[6]) | (((unsigned int)buf[7])<<8); */
/*       if ((arguments->module>=0) && (arguments->module != dif_id)) continue; */
/*       asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet */
/*       if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /\*skip data from unwanted asic*\/ */
/*       int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2); */
/* //    printf("#memory cells: %d\n", memcell_filled); */
/*       int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle); */
/*       for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) { */
/* 	 if (memcell > arguments->maximum_memcell) continue; */
/*          bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)] */
/*                | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8); */
/*          bxid = grayRecode(bxid); */
/*          for (channel = 0; channel < 36; ++channel) { */
/*             if ((arguments->channel != -1) && (channel != arguments->channel)) continue; /\*ship data from unwanted channel*\/ */
/*             tdc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1)] */
/*                   | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8); */
/*             adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2] */
/*                   | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8); */
/*             adc_hit = (adc & 0x1000) ? 1 : 0; */
/* //            adc_gain = (adc & 0x2000) ? 1 : 0; */
/*             tdc = tdc & 0x0fff; */
/*             adc = adc & 0x0fff; */
/*             /\*here is the main correlation loop*\/ */
/*             if ((arguments->require_hitbit == 1) && (adc_hit == 0)) continue; /\*skip data without hitbit, if required*\/ */
/*             if (bif_iterator >= C_MAX_BIF_EVENTS) break; */
/* //          if ((memcell == 0) && (channel == 0)) */
/* //          printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", ROcycle, bxid, asic, memcell, channel, tdc, adc, hit, gain); */
/*             ext_search = (0 - arguments->extended_search); */
/*             if (ext_search + bxid < 0) { /\*fix the beginning of the runs*\/ */
/*                ext_search = 0; */
/*             } */
/*             for (; ext_search < (1 + arguments->extended_search); ++ext_search) { */
/* //               int first_bif_iterator = 1; //get_first_iterator(arguments, bif_data, ROcycle, bxid - ext_search-3); */
/*                if (first_bif_iterator < 0) { */
/*                   continue;/\*nothing found*\/ */
/*                } */
/* //                printf("notning found\n"); */
/*                //             int bif_bxid = (bif_data[bif_iterator].tdc - arguments->correlation_shift) / arguments->bxid_length; */
/*                for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) { */
/*                   bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position; */
/*                   if (bif_roc > ROcycle) break; /\*we jumped to another readout cycle with the bif_iterator*\/ */
/*                   if (bif_roc < ROcycle) continue; /\*not yet in the correct readout cycle*\/ */
/*                   bif_bxid = (bif_data[bif_iterator].tdc - 0) / arguments->bxid_length; */
/*                   if ((bif_bxid + ext_search) > (bxid + 1 + max_correlation / arguments->bxid_length)) break; /\*we jumped to another bxid with the bif_iterator*\/ */
/*                   if ((bif_bxid + ext_search) < bxid) continue; */
/*                   /\* int shift = 0; *\/ */
/*                   int startindex = arguments->bxid_length * (bif_bxid + ext_search - bxid - 1) + bif_data[bif_iterator].tdc % arguments->bxid_length + 1; */
/*                   int endindex = startindex + arguments->bxid_length; */
/*                   if (startindex < 0) startindex = 0; */
/*                   if (endindex >= max_correlation) endindex = max_correlation; */
/* //                  printf("start: %d\tend: %d\n", startindex, endindex); */
/* 		  if (startindex<max_correlation) scan[startindex]++; */
/* 		  scan[endindex]--; */
/*                   /\* for (shift = startindex; shift < endindex; shift++) { *\/ */
/*                   /\*    scan[shift]++; *\/ */
/*                   /\* } *\/ */
/* //                  for (; shift < max_correlation; shift++) { */
/* //                     bif_bxid = ((int) bif_data[bif_iterator].tdc - shift) / arguments->bxid_length; */
/* ////                     same_bif_bxid_index = (bif_bxid == previous_bif_bxid) ? same_bif_bxid_index + 1 : 0; */
/* ////                     previous_bif_bxid = bif_bxid; */
/* //                     if (bif_bxid == bxid) { */
/* //                        scan[shift]++; */
/* //                     } */
/* //                  } */
/*                } */
/*             } */
/*          } */
/*       } */
/* //    printf("\n"); */
/*    } */
/*    fclose(fp); */
/*    int running_sum=0; */
/*    for (i=0 ; i<max_correlation ; i++){//getting the number of correlations */
/*       running_sum += scan[i]; */
/*       scan[i]=running_sum; */
/*    } */
/*    int maxval = -1; */
/*    int maxindex = -1; */
/*    for (i = 0; i < max_correlation; i++) { */
/*       if (scan[i] > maxval) { */
/*          maxindex = i; */
/*          maxval = scan[i]; */
/*       } */
/*    } */
/*    printf("#maximum correlation at: %d\thits:%d\n", maxindex, maxval); */
/*    if (arguments->print_maxoffset){ */
/*       printf("%d\t%d\t%d\t%d\t#ShortScanResult\n",arguments->run_number, arguments->shift_scan_method, maxindex, maxval); */
/*    } else { */
/*       printf("#correlation scan\n#shift\thits\tnormalized\n"); */
/*       for (i = 0; i < max_correlation; i++) { */
/* 	 printf("%d\t%d\t%f\n", i, scan[i], (1.0 * scan[i]) / maxval); */
/*       } */
/*    } */
/*    return 0; */
/* } */

/* int scan_from_raw_bxidwise(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) { */
/*    fprintf(stdout, "#in scan_from_raw_bxidwise\n"); */
/*    arguments->correlation_shift = 0; */
/*    int max_correlation = arguments->shift_scan; */
/*    int scan[max_correlation]; */
/*    int i = 0; */
/*    for (; i < max_correlation; i++) { */
/*       scan[i] = 0; */
/*    } */
/*    unsigned char BXIDs[65536]; */
/*    for (i = 0; i < 65536; i++) { */
/*       BXIDs[i] = 0; */
/*    } */
/*    FILE *fp; */
/*    if (!(fp = fopen(arguments->ecal_raw_filename, "r"))) { */
/*       perror("Unable to open the spiroc raw file\n"); */
/*       return -1; */
/*    } */
/*    fseek(fp, 0, SEEK_SET); //skip timestamp packets */
/*    /\*spiroc datafile iteration*\/ */
/*    int bxid = 0; */
/*    int asic = 0; */
/*    int memcell = 0; */
/*    /\*BIF iteration variables*\/ */
/*    u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger */
/*    int bif_roc = 0; */
/*    int bif_bxid = 0; */
/*    unsigned int headlen, headinfo; */
/*    unsigned char b; */
/*    int freadret; //return code */
/*    int roc_prev = 0; //-1; */
/*    u_int32_t ROcycle = 0; //-1; */
/*    while (1) { */
/*       freadret = fread(&b, sizeof(b), 1, fp); */
/*       if (!freadret) { */
/*          printf("#unable to read / EOF\n"); */
/*          break; */
/*       } */
/*       if (b != 0xCD) continue;/\*try to look for first 0xCD. restart if not found*\/ */
/*       freadret = fread(&b, sizeof(b), 1, fp); */
/*       if (!freadret) { */
/*          perror("#unable to read / EOF\n"); */
/*          break; */
/*       } */
/*       if (b != 0xCD) continue;/\*try to look for second 0xCD. restart if not found*\/ */
/*       freadret = fread(&headlen, sizeof(headlen), 1, fp); */
/*       freadret = fread(&headinfo, sizeof(headinfo), 1, fp); */
/*       if (((headlen & 0xFFFF) > 4095) || ((headlen & 0xFFFF) < 4)) { */
/*          printf("#wrong header length: %d\n", headlen & 0xffff); */
/*          continue; */
/*       } */
/*       if ((headlen & 0xFFFF) == 0x10) { */
/*          fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip timestamp packets */
/*          //fprintf(stdout, "#TS packet\n"); */
/*          continue; */
/*       } */
/*       int lda_port = (headinfo >> 8) & 0xFF; */
/*       if ((lda_port == 0xA0) || (lda_port == 0x80)) { */
/*          fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip timestamp packets */
/*          continue; */
/*       } */
/*       if (lda_port >= C_MAX_PORTS) { */
/*          printf("#ERROR: wrong LDA port: %d\n", lda_port); */
/*          continue;         //wrong port number */
/*       } */
/*       //printf("#DEBUG ROC: %d\n",ROcycle); */
/*       ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100); */
/*       if (ROcycle != roc_prev) { */
/*          if (roc_prev >= 0) { */
/*             //get correlations for  what is in the BXIDs array */
/*             int first_bif_iterator = get_first_iterator2(arguments, bif_data, roc_prev); */
/*             //fprintf(stdout, "."); */
/*             for (bxid = arguments->minimum_bxid; bxid <= arguments->maximum_bxid; bxid++) { */
/*                if (BXIDs[bxid]) { */
/*                   for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) { */
/*                      bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position; */
/*                      if (bif_roc > roc_prev) break; /\*we jumped to another readout cycle with the bif_iterator*\/ */
/*                      if (bif_roc < roc_prev) { */
/*                         /\*not yet in the correct readout cycle*\/ */
/*                         first_bif_iterator = bif_iterator; */
/*                         continue; */
/*                      } */
/*                      bif_bxid = (bif_data[bif_iterator].tdc - 0) / arguments->bxid_length; */
/*                      if ((bif_bxid) > (bxid + 1 + max_correlation / arguments->bxid_length)) break; /\*we jumped to another bxid with the bif_iterator*\/ */
/*                      if ((bif_bxid) < bxid) { */
/*                         /\*too old bif events*\/ */
/*                         first_bif_iterator = bif_iterator; */
/*                         continue; */
/*                      } */
/*                      int startindex = arguments->bxid_length * (bif_bxid - bxid - 1) + bif_data[bif_iterator].tdc % arguments->bxid_length + 1; */
/*                      int endindex = startindex + arguments->bxid_length; */
/*                      if (startindex < 0) startindex = 0; */
/*                      if (endindex >= max_correlation) endindex = max_correlation; */
/* //                  printf("start: %d\tend: %d\n", startindex, endindex); */
/*                      /\* int shift = 0; *\/ */
/*                      /\* for (shift = startindex; shift < endindex; shift++) { *\/ */
/*                      /\*    scan[shift]++; *\/ */
/*                      /\* } *\/ */
/*                      /\* just marking where the scan data should increase by 1 and decrease by 1 again*\/ */
/*                      if (startindex<max_correlation) scan[startindex]++; */
/*                      scan[endindex]--; */
/*                   } */
/*                } */
/*                BXIDs[bxid] = 0; */
/*             } */
/*          } */
/*          roc_prev = ROcycle;         //((headlen >> 16) & 0xFF); */
/*          /\* ++ROcycle; *\/ */
/* //       cycles[row_index] = roc_prev; */
/*       } */
/*       if (ROcycle >= C_ROC_READ_LIMIT) { */
/*          fprintf(stdout, "#limit C_ROC_READ_LIMIT exceede in ROC %d", ROcycle); */
/*          break;/\*for debugging: if we do not want to read the while file. *\/ */
/*       } */
/* //    printf("%05d\t", row_index); */
/* //    printf("%04X\t%04X\t%04X\t%04X", (headlen >> 16) & 0xFFFF, (headlen) & 0xFFFF, (headinfo >> 16) & 0xFFFF, (headinfo) & 0xFFFF); */
/*       freadret = fread(buf, headlen & 0xFFF, 1, fp); */
/*       if (!freadret) { */
/*          printf("#unable to read the complete packet / EOF\n"); */
/*          break; */
/*       } */
/*       if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) { */
/* //       printf("no spiroc data packet!\n"); */
/*          continue; */
/*       } */
/*       if ((arguments->lda_port >= 0) && (lda_port != arguments->lda_port)) continue; */
/*       asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8); //extract the chipID from the packet */
/*       if ((arguments->asic != -1) && (asic != arguments->asic)) continue; /\*skip data from unwanted asic*\/ */
/*       int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2); */
/* //    printf("#memory cells: %d\n", memcell_filled); */
/*       for (memcell = arguments->minimum_memcell; memcell < memcell_filled; ++memcell) { */
/*          if (memcell > arguments->maximum_memcell) continue; */
/*          bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)] */
/*                | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8); */
/*          bxid = grayRecode(bxid); */
/*          BXIDs[bxid] = 1; */
/*       } */
/* //    printf("\n"); */
/*    } */
/*    printf("#Final ROC: %d:\n", ROcycle); */
/*    fclose(fp); */
/*    int running_sum=0; */
/*    for (i=0 ; i<max_correlation ; i++){//getting the number of correlations */
/*       running_sum += scan[i]; */
/*       scan[i]=running_sum; */
/*    } */
/*    fprintf(stdout, "#maximum ROCycle: %d", ROcycle); */
/*    int maxval = -1; */
/*    int maxindex = -1; */
/*    for (i = 0; i < max_correlation; i++) { */
/*       if (scan[i] > maxval) { */
/*          maxindex = i; */
/*          maxval = scan[i]; */
/*       } */
/*    } */
/*    printf("#maximum correlation at: %d\thits:%d\n", maxindex, maxval); */
/*    if (arguments->print_maxoffset){ */
/*       printf("%d\t%d\t%d\t%d\t#ShortScanResult\n",arguments->run_number, arguments->shift_scan_method, maxindex, maxval); */
/*    } else { */
/*       printf("#correlation scan\n#shift\thits\tnormalized\n"); */
/*       for (i = 0; i < max_correlation; i++) { */
/* 	 printf("%d\t%d\t%f\n", i, scan[i], (1.0 * scan[i]) / maxval); */
/*       } */
/*    } */
/*    return 0; */
/* } */

int scan_from_raw_asicwise(struct arguments_t * arguments, const BIF_record_t * bif_data, const int bif_last_record) {
   arguments->correlation_shift = 0;
   int max_correlation = arguments->shift_scan;
   int scan[max_correlation];
   int i = 0;
   for (; i < max_correlation; i++) {
      scan[i] = 0;
   }
   FILE *fp;
   if (!(fp = fopen(arguments->ecal_raw_filename, "r"))) {
      perror("Unable to open the ecal raw file\n");
      return -1;
   }
   /*spiroc datafile iteration*/
   /* int bxid = 0; */
   /* int asic = 0; */
   /* int memcell = 0; */
   /*BIF iteration variables*/
   u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   int bif_roc = 0;
   int bif_bxid = 0;
   /* unsigned int headlen, headinfo; */
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
      if (b != 0xAB) {
         printf ("No 0xAB, but 0x%02x\n", b);
         continue;/*try to look for first 0xCD. restart if not found*/
      }
      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         perror("#unable to read / EOF\n");
         break;
      }
      if (b != 0xCD) {
         printf( "No 0xCD, but 0x%02x\n",b);
         continue;/*try to look for first 0xCD. restart if not found*/
      }
      freadret = fread(&buf, 6, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      int datasize = 0;
      datasize = ((unsigned int) buf[1] << 8) | buf[0];
      freadret = fread(&buf, datasize + 2, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      unsigned short trailerWord = ((unsigned short) buf[datasize + 2 - 1] << 8) + buf[datasize + 2 - 2];
      if (trailerWord == 0x9697) {
	 /*          for (int ibuf = 0; ibuf < datasize + 8; ibuf++) { */
	 /*             ucharValFrameVec.push_back(bufx[ibuf]); */
	 /* //            std::cout << datasize << " LOOP, ibuf=" << ibuf << " hex:" << hex << bufx[ibuf] << "  dec:" << dec << bufx[ibuf] << std::endl; */
	 /*          } */
         int ROC = cycleIDDecoding(buf);
	 ROcycle = update_counter_modulo(ROcycle, (ROC & 0xFFFF), 0x10000, 1000);
	 int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);
	 //printf("#DEBUG ROcycle=%d, first_bif_iterator=%d\n",ROcycle ,first_bif_iterator);
	 if (ROcycle >= C_ROC_READ_LIMIT) break;/*for debugging: if we do not want to read the while file. */
         int nbOfSingleSkirocEventsInFrame = (int) ((datasize - 2 - 2) / SINGLE_SKIROC_EVENT_SIZE);
         for (int n = 0; n < nbOfSingleSkirocEventsInFrame; n++) {
            int rawValue = (unsigned int) buf[datasize - 2 * (n + 1) - 2]
	       + (((unsigned int) buf[datasize - 2 * (n + 1) - 1] & 0x0F) << 8);
	    //            int sca_ascii = nbOfSingleSkirocEventsInFrame - n - 1;
	    //            int sca = nbOfSingleSkirocEventsInFrame - (sca_ascii + 1);
            int bxid = Convert_FromGrayToBinary(rawValue, 12);
	    for (bif_iterator = first_bif_iterator; bif_iterator <= bif_last_record; ++bif_iterator) {
               bif_roc = (int) bif_data[bif_iterator].ro_cycle - arguments->start_position;
               bif_bxid = (bif_data[bif_iterator].tdc - 0) / arguments->bxid_length;
               //fprintf(stdout, "DEBUG bif_iterator=%d, BIF_ROC=%d, bif_data[].ro_cycle=%d,ebxid=%d,bbxid=%d\n", bif_iterator, bif_roc, bif_data[bif_iterator].ro_cycle, bxid, bif_bxid);
               if (bif_roc > ROcycle) break; /*we jumped to another readout cycle with the bif_iterator*/
               if (bif_roc < ROcycle) continue; /*not yet in the correct readout cycle*/
               if ((bif_bxid) > (bxid + 1 + max_correlation / arguments->bxid_length)) break; /*we jumped to another bxid with the bif_iterator*/
               if ((bif_bxid) < bxid) continue;
               int startindex = arguments->bxid_length * (bif_bxid - bxid - 1) + bif_data[bif_iterator].tdc % arguments->bxid_length + 1;
               int endindex = startindex + arguments->bxid_length;
               if (startindex < 0) startindex = 0;
               //if (startindex >= max_correlation) break;
               if (endindex >= max_correlation) endindex = max_correlation - 1;
               //printf("#adding ROC=%d, BXID=%d, bif_roc=%d, bif_bxid=%d,start=%d, end=%d\n",ROcycle,bxid,bif_roc,bif_bxid,startindex,endindex);
	       //                  printf("start: %d\tend: %d\n", startindex, endindex);
	       /* int shift = 0; */
               /* for (shift = startindex; shift < endindex; shift++) { */
               /*    scan[shift]++; */
               /* } */
               if (startindex < max_correlation) {
                  scan[startindex]++;
                  scan[endindex]--;
               }
	    }
         };
      }
   }
   printf("#debug1\n");
   fclose(fp);   
   int running_sum = 0;
   for (i = 0; i < max_correlation; i++) {	    //getting the number of correlations
      // printf("#Debug 1a %d\n",i);
      running_sum += scan[i];
      scan[i] = running_sum;
   }
   printf("#debug2\n");
   int maxval = -1;
   int maxindex = -1;
   for (i = 0; i < max_correlation; i++) {
      if (scan[i] > maxval) {
         maxindex = i;
         maxval = scan[i];
      }
   }
   printf("#debug3\n");

   printf("#maximum correlation at: %d\thits:%d\n", maxindex, maxval);
   /* if (arguments->print_maxoffset){ */
   /*    printf("%d\t%d\t%d\t%d\t#ShortScanResult\n",arguments->run_number, arguments->shift_scan_method, maxindex, maxval); */
   /* } else { */
   printf("#correlation scan\n#shift\thits\tnormalized\n");
   for (i = 0; i < max_correlation; i++) {
      printf("%d\t%d\t%f\n", i, scan[i], (1.0 * scan[i]) / maxval);
   }
   /* } */
   return 0;
}

int main(int argc, char **argv) {
   /* Default values. */
   arguments_init(&arguments);
   bif_data = (BIF_record_t*) malloc(sizeof(BIF_record_t) * C_MAX_BIF_EVENTS);
   extra_stats = (extra_stats_t*) malloc(sizeof(extra_stats_t) * C_ROC_READ_LIMIT);
   /* Parse our arguments; every option seen by parse_opt will
    be reflected in arguments. */
//error_t parseret =
   argp_parse(&argp, argc, argv, 0, 0, &arguments);
   of = stdout;
//   fprintf(of, "hi\n");
   arguments_print(&arguments);
   load_bif_data(&arguments, bif_data, &bif_last_record);
   
   if (arguments.shift_scan > 0) {
      printf("#correlation scan from raw\n");
      switch (arguments.shift_scan_method) {
         case 0:
	    printf("scan method 0 (global bxid) not implemented\n");
            /* scan_from_raw_bxidwise(&arguments, bif_data, bif_last_record); */
            break;
         case 1:
            scan_from_raw_asicwise(&arguments, bif_data, bif_last_record);
            break;
         case 2:
	    printf("scan method 2 (channel hits) not implemented\n");
            /* scan_from_raw_channelwise(&arguments, bif_data, bif_last_record); */
            break;
         default:
            break;
      }
   }
   free(bif_data);
   free(extra_stats);
   exit(0);
}
