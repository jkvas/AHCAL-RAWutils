//============================================================================
// Name        : AhacalPedestalExtract.cpp
// Author      : 
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include <getopt.h>
#include <map>
#include <vector>
#include <cmath>
#include <stdexcept>      // std::out_of_range

//#include "KLauS_ROPackets.h"
//#include "KLauS_Data.h"

using namespace std;

const char *const short_opts = "w:vr:k:l:d:bhe:p:a:c:m:s:";
const struct option long_opts[] = {
      { "spiroc_raw_filenames", required_argument, nullptr, 'w' },
      { "reject_validated", no_argument, nullptr, 'v' },
      { "correlation_shift", required_argument, nullptr, 'r' },
      { "klaus_bxid0_offset", required_argument, nullptr, 'k' }, //in ns
      { "bxid_length", required_argument, nullptr, 'l' },
      //      { "run_number", required_argument, nullptr, 'u' },
      //     { "max_rocs", required_argument, nullptr, 'm' },
//      { "adc_cut", required_argument, nullptr, 'c' },
      { "dummy_triggers", required_argument, nullptr, 'd' },
      { "debug", no_argument, nullptr, 'b' },
      { "help", no_argument, nullptr, 'h' },
      //      { "print_hit_multiplicity", no_argument, nullptr, 't' },

      { "hits", no_argument, nullptr, 1005 },
      { "min_hits_in_memcell", required_argument, nullptr, 'f' },
      { "max_hits_in_memcell", required_argument, nullptr, 'e' },
      { "port", required_argument, nullptr, 'p' },
      { "asic", required_argument, nullptr, 'a' },
      { "channel", required_argument, nullptr, 'c' },
      { "memcell", required_argument, nullptr, 'm' },
      { "reset_on_start", required_argument, nullptr, 1000 },
      { "reset_on_gaps_ms", required_argument, nullptr, 1001 },
      { "reset_on_treadout", required_argument, nullptr, 1002 },
      { "prev_min_ms", required_argument, nullptr, 1003 },
      { "prev_max_ms", required_argument, nullptr, 1004 },
      { "skip_firstfill", required_argument, nullptr, 's' },      //1=skip after a reset
      { "only_firstfill", required_argument, nullptr, 1006 },      //1=reset on run, 2=reset after a delay>1s
      { "print_histograms", no_argument, nullptr, 1007 },
      { nullptr, 0, nullptr, 0 } };

struct arguments_t {
      std::vector<char*> spiroc_raw_filenames;
      int adc_cut;
      bool reject_validated;
      int correlation_shift;
      int klaus_bxid0_offset;
      int bxid_length;
      int run_number;
      int max_rocs;
      int port;
      int asic;
      int channel;
      int memcell;
      int dummy_triggers;
      bool debug;
      //   bool print_hit_multiplicity;
      int max_hits_in_memcell;
      int min_hits_in_memcell;
      int reset_on_start;
      int reset_on_gaps_ms;
      int reset_on_treadout;
      int prev_min_ms;
      int prev_max_ms;
      int hits;
      int skip_firstfill;
      int only_firstfill;
      bool print_histograms;
};

struct arguments_t arguments;

void argumentsInit(struct arguments_t &arguments) {
   //   arguments.spiroc_raw_filename = nullptr;
   arguments.adc_cut = 600;
   arguments.reject_validated = false;
   arguments.correlation_shift = 2113;
   arguments.bxid_length = 160;
   arguments.run_number = 0;
   arguments.max_rocs = 0;
   arguments.dummy_triggers = 0;
   arguments.debug = false;
   arguments.port = -1;
   arguments.asic = -1;
   arguments.channel = -1;
   arguments.memcell = -1;
//   arguments.print_hit_multiplicity = false;
   arguments.klaus_bxid0_offset = 8750;
   arguments.max_hits_in_memcell = 37;
   arguments.skip_firstfill = 0;
   arguments.only_firstfill = 0;
   arguments.min_hits_in_memcell = 1;
   arguments.reset_on_start = 1;
   arguments.reset_on_gaps_ms = 0;
   arguments.reset_on_treadout = 0;
   arguments.prev_min_ms = 0;
   arguments.prev_max_ms = 0;
   arguments.hits = 0;
   arguments.print_histograms = false;
}

void argumentsPrint(const struct arguments_t &arguments) {
   for (auto spiroc_raw_filename : arguments.spiroc_raw_filenames)
      std::cout << "#spiroc_raw_filename=" << spiroc_raw_filename << std::endl;
   std::cout << "#adc_cut=" << arguments.adc_cut << std::endl;
   std::cout << "#reject_validated=" << arguments.reject_validated << std::endl;
   std::cout << "#correlation_shift=" << arguments.correlation_shift << std::endl;
   std::cout << "#klaus_bxid0_offset=" << arguments.klaus_bxid0_offset << std::endl;
   std::cout << "#bxid_length=" << arguments.bxid_length << std::endl;
   std::cout << "#run_number=" << arguments.run_number << std::endl;
   std::cout << "#max_rocs=" << arguments.max_rocs << std::endl;
   std::cout << "#dummy_triggers=" << arguments.dummy_triggers << std::endl;
   std::cout << "#debug=" << arguments.debug << std::endl;
   std::cout << "#port=" << arguments.port << std::endl;
   std::cout << "#asic=" << arguments.asic << std::endl;
   std::cout << "#channel=" << arguments.channel << std::endl;
   std::cout << "#memcell=" << arguments.memcell << std::endl;
//   std::cout << "#print_hit_multiplicity=" << arguments.print_hit_multiplicity << std::endl;
   std::cout << "#max_hits_in_memcell=" << arguments.max_hits_in_memcell << std::endl;
   std::cout << "#skip_firstfill=" << arguments.skip_firstfill << std::endl;
   std::cout << "#only_firstfill=" << arguments.only_firstfill << std::endl;
   std::cout << "#min_hits_in_memcell=" << arguments.min_hits_in_memcell << std::endl;
   std::cout << "#reset_on_start=" << arguments.reset_on_start << std::endl;
   std::cout << "#reset_on_gaps_ms=" << arguments.reset_on_gaps_ms << std::endl;
   std::cout << "#reset_on_treadout=" << arguments.reset_on_treadout << std::endl;
   std::cout << "#prev_min_ms=" << arguments.prev_min_ms << std::endl;
   std::cout << "#prev_max_ms=" << arguments.prev_max_ms << std::endl;
   std::cout << "#hits=" << arguments.hits << std::endl;
   std::cout << "#print_histograms=" << arguments.print_histograms << std::endl;
   if (arguments.debug) {
      std::cout
            << "#length_to_last_bxid\tROCLength\tlast_bxid[25nstics]\tlast_bxid\tROcycle\tasic\tport\tfilled_mems\tRoc_Max_memcel\tbusy-start\tstop-busy\t#DEBUG length difference"
            << std::endl;
   }
}

void PrintHelp() {
   std::cout << "prints the noise statistics of ahcal raw file dumped by eudaq" << std::endl;
   std::cout << "Options: " << std::endl;
   std::cout << "   -w, --spiroc_raw_filenames   (multiple arguments possible)" << std::endl;
   std::cout << "   -r, --correlation_shift" << std::endl;
   std::cout << "   -k, --klaus_bxid0_offset" << std::endl;
   std::cout << "   -l, --bxid_length" << std::endl;
   std::cout << "   -u, --run_number" << std::endl;
   std::cout << "   -v, --reject_validated" << std::endl;
   std::cout << "   -m, --max_roc" << std::endl;
   std::cout << "   -c, --adc_cut" << std::endl;
   std::cout << "   -d, --dummy_triggers" << std::endl;
   std::cout << "   -h, --help" << std::endl;
   std::cout << "   -e, --max_hits_in_memcell" << std::endl;
   std::cout << "   -f, --min_hits_in_memcell" << std::endl;
   std::cout << "   -p, --port" << std::endl;
   std::cout << "   -a, --asic" << std::endl;
   std::cout << "   -c, --channel" << std::endl;
   std::cout << "   -m, --memcell" << std::endl;
   std::cout << "       --reset_on_start (1=reset on new run)" << std::endl;
   std::cout << "       --reset_on_gaps_ms (greater gaps issue a reset)" << std::endl;
   std::cout << "       --reset_on_treadout (1=reset on temperature readout)" << std::endl;
   std::cout << "       --prev_min_ms (minimum distance from last cellfill, in milliseconds)" << std::endl;
   std::cout << "       --prev_max_ms (maximum distance from last cellfill - good data expected)" << std::endl;
   std::cout << "    -s --skip_firstfill (1=skip after a reset (defined above))" << std::endl;
   std::cout << "       --only_firstfill (1=use only data after a reset (defined above))" << std::endl;
   std::cout << "       --hits (look into hitbit==1 instead of hitbit==0)" << std::endl;
   std::cout << "       --reset_on_start (1=reset on new run)" << std::endl;
   std::cout << "       --print_histograms" << std::endl;
//   std::cout << "   -t, --print_hit_multiplicity" << std::endl;
   exit(1);
}

void ProcessArgs(int argc, char **argv) {
   while (true) {
      const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
//      std::cout << "DEBUG argc=" << argc << std::endl;
//      for (int i = 0; i < argc; i++) {
//	 std::cout << "DEBUG i=" << i << ", argv[i]=" << argv[i] << std::endl;
//      }
      if (-1 == opt)
         break;
//      std::cout << "DEBUGopt=" << opt << ", optind=" << optind << " Optarg=" << optarg << std::endl << std::flush;
      switch (opt) {
         case 'w':
            optind--;
            for (; optind < argc && *argv[optind] != '-'; optind++) {
               arguments.spiroc_raw_filenames.push_back(argv[optind]);
//	       std::cout << "new -w: " << argv[optind] << std::endl;
//	       std::cout << "DEBUG optind=" << optind << std::endl << std::flush;
            }
            //optind++;
            break;
         case 'v':
            arguments.reject_validated = true;
            break;
         case 'r':
            arguments.correlation_shift = std::stoi(optarg);
            break;
         case 'k':
            arguments.klaus_bxid0_offset = std::atoi(optarg);
            break;
         case 'u':
            arguments.run_number = std::stoi(optarg);
            break;
         case 'l':
            arguments.bxid_length = std::atoi(optarg);
            break;
         case 'm':
            arguments.memcell = std::atoi(optarg);
            break;
         case 'p':
            arguments.port = std::atoi(optarg);
            break;
         case 'a':
            arguments.asic = std::atoi(optarg);
            break;
         case 'c':
            arguments.channel = std::atoi(optarg);
            break;
         case 'd':
            arguments.dummy_triggers = std::atoi(optarg);
            break;
         case 'b':
            arguments.debug = true;
            break;
         case 'e':
            arguments.max_hits_in_memcell = std::atoi(optarg);
            break;
         case 'f':
            arguments.min_hits_in_memcell = std::atoi(optarg);
            break;
         case 's':
            arguments.skip_firstfill = std::atoi(optarg);
            break;
         case 1006:
            arguments.only_firstfill = std::atoi(optarg);
            break;
         case 1005:
            arguments.hits = 1;
            break;
         case 1000:
            arguments.reset_on_start = std::atoi(optarg);
            break;
         case 1001:
            arguments.reset_on_gaps_ms = std::atoi(optarg);
            break;
         case 1002:
            arguments.reset_on_treadout = std::atoi(optarg);
            break;
         case 1003:
            arguments.prev_min_ms = std::atoi(optarg);
            break;
         case 1004:
            arguments.prev_max_ms = std::atoi(optarg);
            break;
         case 1007:
            arguments.print_histograms = true;
            break;
            //	 case 't':
//	    arguments.print_hit_multiplicity = true;
//	    break;
         case 'h': // -h or --help
         case '?': // Unrecognized option
         default:
            PrintHelp();
            break;
      }
   }
}

int update_counter_modulo(unsigned int oldvalue, unsigned int newvalue_modulo, unsigned int modulo, unsigned int max_backwards) {
   unsigned int newvalue = oldvalue - max_backwards;
   unsigned int mask = modulo - 1;
   if ((newvalue & mask) > (newvalue_modulo & mask)) {
      newvalue += modulo;
   }
   newvalue = (newvalue & (~mask)) | (newvalue_modulo & mask);
   return newvalue;
}

unsigned int getPedestal(const int lda, const int port, const int chip, const int channel, const int cell) {
   return (unsigned int) 300; //TODO
}

unsigned int getMipCut(const double mips, const int lda, const int port, const int chip, const int channel, const int cell) {
   return arguments.adc_cut; //TODO channel+memcell-wise pedestal estimator
}

void prefetch_information(char *filename, const struct arguments_t &arguments, std::map<uint32_t, std::map<uint16_t, bool> > &ROCtriggers,
      std::map<int, u_int64_t> &startTSs,
      std::map<int, u_int64_t> &stopTSs,
      std::map<int, u_int64_t> &busyTSs, std::map<int, int> &memcells)
      {
   unsigned char buf[4096];
   FILE *fp;
   if (!(fp = fopen(filename, "r"))) {
      perror("#Unable to open the spiroc raw file\n");
      return;
   }

//   uint8_t lda = 0;
   uint8_t port = 0;
   u_int32_t headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   u_int32_t ROcycle = -1;
   u_int64_t TS = 0;
//   u_int64_t lastTS = 0;
   u_int64_t lastStartTS = 0;
//   u_int64_t lastStopTS = 0;
   int within_ROC = 0;
   while (1) {
      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         printf("#unable to read / EOF\n");
         break;
      }
      if (b != 0xCD)
         continue;/*try to look for first 0xCD. restart if not found*/

      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         perror("#unable to read / EOF\n");
         break;
      }
      if (b != 0xCD) {
         //printf("!");
         continue;/*try to look for second 0xCD. restart if not found*/
      }

      freadret = fread(&headlen, sizeof(headlen), 1, fp);
      freadret = fread(&headinfo, sizeof(headinfo), 1, fp);
//      lda = headinfo & 0xFF;
      port = (headinfo >> 8) & 0xFF;
      //unsigned int errors = (headinfo >> 16) & 0xFF;
      unsigned int status = (headinfo >> 24) & 0xFF;
      // skip unwanted packets:
      if (((port == 128) && ((headlen & 0xFFFF) == 8)) ||
            //            ((port==160) && ((headlen & 0xFFFF)==16)) || we want timestamp
            ((status == 0xa0) && ((headlen & 0xFFFF) == 16)) || //temp
            //     ((headlen & 0xFFFF) == 16) || //timestamp
            ((status == 0x20) && ((headlen & 0xFFFF) == 12)) //EOR packet
            ) {
         fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip those packets
         continue;
      }
      if (((headlen & 0xFFFF) > 4095) || ((headlen & 0xFFFF) < 4)) {
         continue;
      }
      if ((headlen & 0xFFFF) == 0x10) {
         //////////////////////////////////////////////////////////////////////////////
         // timestamp
         //////////////////////////////////////////////////////////////////////////////
         if (fread(buf, 8, 1, fp) <= 0) {
            std::cout << "error read 2" << std::endl;
            continue;
         }
         if ((buf[0] != 0x45) || (buf[1] != 0x4D) || (buf[2] != 0x49) || (buf[3] != 0x54)) {         //TIME header
            fseek(fp, 8, SEEK_CUR);
            continue;
         }
         int newROC = (headlen >> 16) & 0xFF;
         ROcycle = update_counter_modulo(ROcycle, newROC, 256, 100);
         if (arguments.max_rocs && ((int) ROcycle > arguments.max_rocs)) {
            std::cout << "#Maximum ROC number reached" << std::endl;
            break;
         }
         int type = buf[4];
         // int trigid = ((int) buf[6]) + (((int) buf[7]) << 8); //possible trigid
         if (fread(buf, 8, 1, fp) <= 0) {
            std::cout << "error read 3" << std::endl;
            continue;
         }
         if ((buf[6] != 0xAB) || (buf[7] != 0xAB)) {
            std::cout << "error read 4: packet not finished with abab" << std::endl;
            continue;
         }
         TS = (u_int64_t) buf[0] + ((u_int64_t) buf[1] << 8) + ((u_int64_t) buf[2] << 16) + ((u_int64_t) buf[3] << 24) + ((u_int64_t) buf[4] << 32)
               + ((u_int64_t) buf[5] << 40);
         switch (type) {
            case 0x01: //start_acq
               within_ROC = 1;
               startTSs[ROcycle] = TS;
               lastStartTS = TS;
               break;
            case 0x02:               //stop_acq
               stopTSs[ROcycle] = TS;
               within_ROC = 0;
//               lastStopTS = TS;
               break;
            case 0x10: {               //trig
               // std::cout << "#Trigger TS packet ROC=" << ROC << std::endl;
               uint16_t trigBXID = ((TS - lastStartTS) - arguments.correlation_shift) / arguments.bxid_length;
               ROCtriggers[ROcycle][trigBXID] = true;
            }
               break;
            case 0x21:               //busy - not interresting here
               busyTSs[ROcycle - (1 - within_ROC)] = TS;
               break;
            default:
               break;
         }
//         lastTS = TS;
         continue;
         //////////////////////////////////////////////////////////////////////////////
         // end timestamp
         //////////////////////////////////////////////////////////////////////////////
      }
      //data packet
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//      std::cout << "#d" << memcell_filled << std::endl;
      if (memcells.count(ROcycle)) {
         if (memcell_filled > memcells[ROcycle])
            memcells[ROcycle] = memcell_filled;
      }
      else {
         memcells[ROcycle] = memcell_filled;
      }
      fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip those packets
   }
   fclose(fp);
   std::cout << "#Trigger read finished. at ROC=" << ROcycle << std::endl;
}

inline unsigned short grayToBinary(unsigned short num) {
   unsigned short mask;
   for (mask = num >> 1; mask != 0; mask = mask >> 1)
      num = num ^ mask;
   return num;
}

int analyze_pedestal(const struct arguments_t &arguments) {
   unsigned char buf[4096];
   std::map<uint32_t, unsigned int[4096]> histograms; //maps port.chip.channel.cell -> histogram, will allocate ~6GB for whole AHCAL
//   u_int64_t lastRunTs = 0LLU;
   std::map<uint32_t, uint64_t> FillingTS; //port<<16 + asic<<8 + cell
   for (char *filename : arguments.spiroc_raw_filenames) {
      uint32_t last_reset_on_gaps_roc = 0;
      uint32_t last_reset_on_treadout_roc = 0;
      std::map<uint32_t, std::map<uint16_t, bool>> ROCtriggers; // map of map of triggers on ROC
      std::map<int, u_int64_t> startTSs; //maps start TS to ROC
      std::map<int, u_int64_t> stopTSs; //maps stop TS to ROC
      std::map<int, u_int64_t> busyTSs; //maps busy TS to ROC
      std::map<int, int> memcells; //maps maximum memcell filled in the ROC to ROC
      if (arguments.reset_on_start) {
         std::cout << "INFO: reseting on start of the run" << std::endl;
         FillingTS.clear();
      }
      prefetch_information(filename, arguments, ROCtriggers, startTSs, stopTSs, busyTSs, memcells);
//   std::vector<u_int64_t> startTSs; //maps start TS to ROC
//   std::vector<u_int64_t> stopTSs; //maps start TS to ROC
//   std::map<uint32_t, int> acquisitions; //maps (LDA.Port.chip) to the number of acquisitions
      // double busy; //conversion + readout time
//      double blocked; //not going to acquisition, although the busy is down (temperature, HGCAL...)
      std::map<uint64_t, uint8_t> HitMultiplicity; //how many hits in ROC(47..16).bxid(15..0)
      std::cout << "#DEBUG prefetch filename: " << filename << std::endl << std::flush;
      //calculate the global duration
      double globalLength = 0.0;
      std::map<uint32_t, uint64_t> ADCsum; //maps LDA.Port.Chip.Channel to the ADC summary
      std::map<uint32_t, int> ASIChits; //maps LDA.Port.Chip to number of hits without bxid0
      std::map<uint32_t, int> hits; //maps LDA.Port.Chip.Channel to number of hits without bxid0
      std::map<uint32_t, int> hitsAfterAdcCut; //maps LDA.Port.Chip.Channel to number of hits without bxid0 after the ADC cut
      std::map<uint32_t, double> acqLens; //maps (LDA.Port.chip) to length of acq.
      for (const auto &it : startTSs) {
         if (stopTSs.count(it.first)) {
            uint64_t start = it.second;
            uint64_t stop = stopTSs.at(it.first);
            if ((start == 0) || (stop == 0)) {
               std::cout << "#No start or stop TS in roc=" << it.first << " start=" << start << " stop=" << stop << std::endl;
               continue;
            }
            uint64_t duration = stop - start - arguments.correlation_shift;
            if (duration > 4000000) { //(4000000 = 100 ms)
               std::cout << "#Run length invalid in roc=" << it.first << " length=" << duration << " (" << ((25E-9) * duration) << " s)" << std::endl;
               continue;
            }
            globalLength += ((25.0E-9) * duration);
         }
      }
      FILE *fp;
      if (!(fp = fopen(filename, "r"))) {
         perror("#Unable to open the spiroc raw file\n");
         return -1;
      }
      /*spiroc datafile iteration*/
      uint8_t lda = 0;
      uint8_t port = 0;
      int bxid = 0;
      int asic = 0;
      int memcell = 0;
      int channel = 0;
      int tdc, adc = 0;
      int adc_hit = 0;
      int adc_gain = 0;
      int tdc_hit = 0;
//      int tdc_gain = 0;
      int mismatches_hit = 0;
      int mismatches_gain = 0;
      int mismatches_length = 0;
      uint64_t ROCLength = 0;
      u_int32_t headlen, headinfo;
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
         if (b != 0xCD)
            continue;/*try to look for first 0xCD. restart if not found*/

         freadret = fread(&b, sizeof(b), 1, fp);
         if (!freadret) {
            perror("#unable to read / EOF\n");
            break;
         }
         if (b != 0xCD) {
            //printf(".");
            continue;/*try to look for second 0xCD. restart if not found*/
         }

         freadret = fread(&headlen, sizeof(headlen), 1, fp);
         freadret = fread(&headinfo, sizeof(headinfo), 1, fp);
         lda = headinfo & 0xFF;
         port = (headinfo >> 8) & 0xFF;
//         unsigned int errors = (headinfo >> 16) & 0xFF;
         unsigned int status = (headinfo >> 24) & 0xFF;
         // skip unwanted packets:
         if ((status == 0xa0) && ((headlen & 0xFFFF) == 16)) { //temp
            fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip those packets
            if (last_reset_on_treadout_roc != ROcycle) {
               std::cout << "#DEBUG Temp readout ROC " << ROcycle << std::endl;
               if (arguments.reset_on_treadout) {
                  FillingTS.clear();
               }
               last_reset_on_treadout_roc = ROcycle;
            }
            continue;
         }
         if (((port == 128) && ((headlen & 0xFFFF) == 8)) ||
               //            ((port==160) && ((headlen & 0xFFFF)==16)) || we want timestamp
               ((headlen & 0xFFFF) == 16) || //timestamp
               ((status == 0x20) && ((headlen & 0xFFFF) == 12)) //EOR packet
               ) {
            fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip those packets
            continue;
         }
         if (((headlen & 0xFFFF) > 4095) || ((headlen & 0xFFFF) < 4)) {
            printf("#Wrong header length: %d\n", headlen & 0xffff);
            printf("#head=0x%08x %08x\n", headinfo, headlen);
            continue;
         }
         ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
         if (arguments.reset_on_gaps_ms) {
            try {
               if (ROcycle != last_reset_on_gaps_roc)
                  if ((startTSs.at(ROcycle) - startTSs.at(ROcycle - 1)) > 40LLU * 1000 * arguments.reset_on_gaps_ms) {
                     std::cout << "#INFO: reseting first occurances in memory cells. ROC=" << ROcycle << ", DIFF="
                           << (startTSs.at(ROcycle) - startTSs.at(ROcycle - 1)) << "("
                           << 1.0 * (startTSs.at(ROcycle) - startTSs.at(ROcycle - 1)) / 40000000 << " s)" << std::endl;
                     FillingTS.clear();
                     last_reset_on_gaps_roc = ROcycle;
                  }
            }
            catch (const std::out_of_range &oor) {
               if (arguments.debug) std::cerr << "#DEBUG:" << ROcycle << "Out of Range error: " << oor.what() << std::endl;
            }
         }
         freadret = fread(buf, headlen & 0xFFF, 1, fp);
         if (!freadret) {
            printf("#unable to read the complete packet / EOF\n");
            break;
         }
         if ((arguments.port != -1) && (arguments.port != port)) continue;
         bool spirocPacket = false;
         bool klausPacket = false;
         if ((buf[0] == 0x41) && (buf[1] == 0x43) && (!((buf[2] == 0x48) && (buf[3] == 0x41))) && (buf[5] == 0x41)
               && ((buf[4] == 0x49) || (buf[4] == 0x50) || (buf[4] == 0x51))) {
            //printf("#Klaus packet! size=%03d\n", headlen & 0xFFFF);
            asic = buf[8] + 1;
//            int ro_chain = buf[9];
//            int dif_id = buf[12] + (buf[13] << 8);
            for (unsigned int hitindex = 0; 22 + 6 * hitindex < (headlen & 0xFFFF); hitindex++) {
               unsigned char channelID = (buf[14 + 6 * hitindex + 1] >> 0) & 0x000f;
               unsigned char groupID = (buf[14 + 6 * hitindex + 1] >> 4) & 0x0003;
               int channel = 0xFF;
               if (groupID == 3 && channelID == 0)
                  channel = 36; // T0 channel
               else if (channelID < 12)
                  channel = 12 * groupID + (int) channelID;
               adc_gain = ((buf[14 + 6 * hitindex + 0] >> 7) & 0x0001); //compatible
               adc_gain = 1 - adc_gain; //to be compatible with AHCAL
               //TODO gainsel busy
               unsigned int ADC_10b = 1023 - (((buf[14 + 6 * hitindex + 0] << 4) & 0x03f0) | ((buf[14 + 6 * hitindex + 3] >> 4) & 0x000f));
               unsigned int ADC_6b = 63 - ((buf[14 + 6 * hitindex + 0] >> 0) & 0x003f);
               unsigned int ADC_PIPE = 255 - ((buf[14 + 6 * hitindex + 3] >> 0) & 0x00ff);
               unsigned int adc = (ADC_6b << 8) + ADC_PIPE; //TODO does not work?
               adc = ADC_10b;
               unsigned int tdc = grayToBinary(((buf[14 + 6 * hitindex + 2] << 8) & 0xff00) | ((buf[14 + 6 * hitindex + 5] << 0) & 0x00ff));
               bxid = (tdc * 25 * 4 - arguments.klaus_bxid0_offset) / (arguments.bxid_length * 25);

               ASIChits[((lda << 16) | (port << 8) | (asic & 0xFF))]++;
               //fill-in channel-wise information
               hits[((lda << 24) | (port << 16) | ((asic & 0xFF) << 8) | (channel))]++;
               ADCsum[((lda << 24) | (port << 16) | ((asic & 0xFF) << 8) | (channel))] += adc;
               uint64_t MultiplicityIndex = (((uint64_t) ROcycle) << 32) | (uint64_t) bxid;
               HitMultiplicity[MultiplicityIndex]++;
               if (HitMultiplicity[MultiplicityIndex] == 0)
                  HitMultiplicity[MultiplicityIndex]--; //prevent overflows
               if (adc_gain && (adc > getMipCut(0.5, lda, port, asic, channel, 0))) {
                  hitsAfterAdcCut[((lda << 24) | (port << 16) | ((asic & 0xFF) << 8) | (channel))]++;
               }
            }
            acqLens[((lda << 16) | (port << 8) | (asic & 0xFF))] += 0;
            klausPacket = true;
         }

         if ((buf[0] == 0x41) && (buf[1] == 0x43) && (buf[2] == 0x48) && (buf[3] == 0x41)) { //SPIROC
            if (((headlen & 0x0fff) - 12) % 146) { //if the length of the packet does not align with the number of memory cells
               mismatches_length++;
               printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d, port %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle,
                     asic, port);
               continue;
            }
            spirocPacket = true;
            asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8);            //extract the chipID from the packet
            if ((arguments.asic != -1) && (arguments.asic != asic)) continue;
            int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
            acqLens[((lda << 16) | (port << 8) | (asic & 0xFF))] += 0;
            ROCLength = stopTSs[ROcycle] - startTSs[ROcycle] - arguments.correlation_shift;
            ASIChits[((lda << 16) | (port << 8) | (asic & 0xFF))] += memcell_filled - arguments.dummy_triggers;         // - 1;
            if (ROCLength > (65536LLU * arguments.bxid_length + (uint64_t) arguments.correlation_shift)) {
               std::cout << "#Readout cycle length=" << ROCLength << " too long. ROC=" << ROcycle << " Startroc=" << startTSs[ROcycle] << " StopROC="
                     << stopTSs[ROcycle] << " ROCLength=" << ROCLength
                     << " limit=" << (65536 * arguments.bxid_length + arguments.correlation_shift) << std::endl;
            }
            for (memcell = arguments.dummy_triggers; memcell < memcell_filled; ++memcell) {
               bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
                     | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
               if (arguments.reject_validated && (ROCtriggers[ROcycle][bxid])) {
                  //                  std::cout << "trigger match !!!!!!!!!!!!!!!!!!!!!!!!!!!! ROC=" << ROcycle << " BXID=" << bxid << std::endl;
                  ASIChits[((lda << 16) | (port << 8) | (asic & 0xFF))]--;
                  continue;
               }

               uint64_t timeFromLastMs = 0LLU;
               try {
                  timeFromLastMs = startTSs.at(ROcycle) + bxid * arguments.bxid_length - FillingTS[(port << 16) | (asic << 8) | (memcell)];
                  //(startTSs.at(ROcycle) - startTSs.at(ROcycle - 1))
               } catch (const std::out_of_range &oor) {
                  if (arguments.debug) std::cerr << "#DEBUG:" << ROcycle << "Out of Range error: " << oor.what() << std::endl;
               }
               if ((arguments.prev_max_ms != 0) && (timeFromLastMs > (25LLU * 1000 * arguments.prev_max_ms))) continue;
               if ((arguments.prev_min_ms != 0) && (timeFromLastMs < (25LLU * 1000 * arguments.prev_min_ms))) continue;
               if (arguments.skip_firstfill)
                  if (FillingTS[(port << 16) | (asic << 8) | (memcell)] == 0) {
                     std::cout << "#DEBUG skipped because first ocurance, A=" << asic << ", P=" << (int) port << ", M=" << memcell << std::endl;
                     continue;
                  }
               if (arguments.only_firstfill)
                  if (FillingTS[(port << 16) | (asic << 8) | (memcell)] != 0) {
                     std::cout << "#DEBUG skipped because NOT first ocurance, A=" << asic << ", P=" << (int) port << ", M=" << memcell << std::endl;
                     continue;
                  }

               int numOfHits = 0;
               for (channel = 0; channel < 36; ++channel) {
                  adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2]
                        | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8);
                  adc_hit = (adc & 0x1000) ? 1 : 0;
                  if (adc_hit)
                     numOfHits++;
               }
               if ((arguments.max_hits_in_memcell > 0) && (numOfHits > arguments.max_hits_in_memcell)) continue;
               if ((arguments.min_hits_in_memcell > 0) && (numOfHits < arguments.min_hits_in_memcell)) continue;
               for (channel = 0; channel < 36; ++channel) {
                  //  if ((arguments.channel != -1) && (channel != arguments.channel)) continue;/*ship data from unwanted channel*/
                  tdc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1)]
                        | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8);
                  adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2]
                        | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8);
                  adc_hit = (adc & 0x1000) ? 1 : 0;
//               adc_gain = (adc & 0x2000) ? 1 : 0;
                  tdc_hit = (tdc & 0x1000) ? 1 : 0;
//                  tdc_gain = (tdc & 0x2000) ? 1 : 0;
//               tdc = tdc & 0x0fff;
                  adc = adc & 0x0fff;
//               if (adc_hit != tdc_hit) mismatches_hit++;
//               if (adc_gain != tdc_gain) mismatches_gain++;

                  if ((arguments.channel != -1) && (arguments.channel != channel)) continue;
                  if ((arguments.memcell != -1) && (arguments.memcell != memcell)) continue;
                  if (adc_hit || tdc_hit) continue;
                  histograms[(port << 24) | ((asic & 0xFF) << 16) | (channel << 8) | memcell][adc]++;
               }
            }
            for (memcell = 0; memcell < memcell_filled; ++memcell) { //update the statistics
               bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
                     | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
               FillingTS[(port << 16) | (asic << 8) | (memcell)] = startTSs.at(ROcycle) + bxid * arguments.bxid_length;
            }
         }
         if ((klausPacket == false) && (spirocPacket == false)) {
            if (arguments.debug)
               printf("#no spiroc data packet! #head=0x%08x %08x\n", headinfo, headlen);
            continue;
         }
      }
      fclose(fp);
      std::cout << "#Mismatched hit bits: " << mismatches_hit << std::endl;
      std::cout << "#Mismatched gain bits: " << mismatches_gain << std::endl;
   }
   std::cout << "#Port\tAsic\tChan\tCell\tMax\tMaxAt\tMean10\tStddev10\tcount10\tMean100\tStddev100\tcount100\tMean1000\tStddev1000\tcount1000\t#pedstats"
         << std::endl;
   for (const auto &it : histograms) {
      std::cout << ((it.first >> 24) & 0xFF) << "\t" << std::flush;
      std::cout << ((it.first >> 16) & 0xFF) << "\t" << std::flush;
      std::cout << ((it.first >> 8) & 0xFF) << "\t" << std::flush;
      std::cout << ((it.first >> 0) & 0xFF) << "\t" << std::flush;
      uint64_t hist_count = 0;
      uint64_t hist_sum = 0;
      uint64_t hist_max = 0;
      uint64_t hist_max_at = -1;
      for (int i = 0; i < 4096; i++) {
         if (it.second[i] > hist_max) {
            hist_max = it.second[i];
            hist_max_at = i;
         }
      }
      std::cout << hist_max << "\t";
      std::cout << hist_max_at << "\t";
      for (uint64_t factor : { hist_max / 10, hist_max / 100, hist_max / 1000 }) {
         hist_count = 0;
         hist_sum = 0;
         for (int i = 0; i < 4096; i++) {
            if (it.second[i] >= factor) {
               hist_count += it.second[i];
               hist_sum += it.second[i] * i;
            }
         }
         double hist_mean = 1.0L * hist_sum / hist_count;
         double hist_stddev = 0.0L;
         for (int i = 0; i < 4096; i++) {
            if (it.second[i] >= factor)
               hist_stddev += it.second[i] * (i - hist_mean) * (i - hist_mean);
         }
         if (hist_count == 1)
            hist_stddev = 0;
         else
            hist_stddev = sqrt(hist_stddev / (hist_count - 1));
         std::cout << hist_mean << "\t";
         std::cout << hist_stddev << "\t";
         std::cout << hist_count << "\t";
      }
      std::cout << "# pedstats" << std::endl;
      if (arguments.print_histograms) {
         for (int i = 0; i < 4096; i++) {
            std::cout << i << "\t" << it.second[i] << std::endl;
         }
         std::cout << std::endl << std::endl;
      }
   }
   return 0;
}

int main(int argc, char **argv) {
   argumentsInit(arguments);
   ProcessArgs(argc, argv);
   argumentsPrint(arguments);
   analyze_pedestal(arguments);
//   cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
   return 0;
}
