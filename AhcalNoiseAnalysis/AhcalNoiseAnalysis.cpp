//============================================================================
// Name        : AhacalNoiseAnalysis.cpp
// Author      : 
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include <getopt.h>
#include <map>
#include <vector>

using namespace std;

const char* const short_opts = "w:vrluhmd";
const struct option long_opts[] = {
      { "spiroc_raw_filename", required_argument, nullptr, 'w' },
      { "reject_validated", no_argument, nullptr, 'v' },
      { "correlation_shift", required_argument, nullptr, 'r' },
      { "bxid_length", required_argument, nullptr, 'l' },
      { "run_number", required_argument, nullptr, 'u' },
      { "max_rocs", required_argument, nullptr, 'm' },
      { "adc_cut", required_argument, nullptr, 'c' },
      { "dummy_triggers", required_argument, nullptr, 'd' },
      { "debug", no_argument, nullptr, 'b' },
      { "help", no_argument, nullptr, 'h' },
      { nullptr, 0, nullptr, 0 }
};

struct arguments_t {
      char *spiroc_raw_filename;
      int adc_cut;
      bool reject_validated;
      int correlation_shift;
      int bxid_length;
      int run_number;
      int max_rocs;
      int dummy_triggers;
      bool debug;
};

struct arguments_t arguments;

void argumentsInit(struct arguments_t & arguments) {
   arguments.spiroc_raw_filename = nullptr;
   arguments.adc_cut = 600;
   arguments.reject_validated = false;
   arguments.correlation_shift = 2113;
   arguments.bxid_length = 160;
   arguments.run_number = 0;
   arguments.max_rocs = 0;
   arguments.dummy_triggers = 0;
   arguments.debug = false;
}

void argumentsPrint(const struct arguments_t & arguments) {
   std::cout << "#spiroc_raw_filename=" << ((arguments.spiroc_raw_filename == nullptr) ? "(no)" : arguments.spiroc_raw_filename) << std::endl;
   std::cout << "#adc_cut=" << arguments.adc_cut << std::endl;
   std::cout << "#reject_validated=" << arguments.reject_validated << std::endl;
   std::cout << "#correlation_shift=" << arguments.correlation_shift << std::endl;
   std::cout << "#bxid_length=" << arguments.bxid_length << std::endl;
   std::cout << "#run_number=" << arguments.run_number << std::endl;
   std::cout << "#max_rocs=" << arguments.max_rocs << std::endl;
   std::cout << "#dummy_triggers=" << arguments.dummy_triggers << std::endl;
   std::cout << "#debug=" << arguments.debug << std::endl;
}

void PrintHelp() {
   std::cout << "prints the noise statistics of ahcal raw file dumped by eudaq" << std::endl;
   std::cout << "Options: " << std::endl;
   std::cout << "   -w, --spiroc_raw_filename" << std::endl;
   std::cout << "   -r, --correlation_shift" << std::endl;
   std::cout << "   -l, --bxid_length" << std::endl;
   std::cout << "   -u, --run_number" << std::endl;
   std::cout << "   -v, --reject_validated" << std::endl;
   std::cout << "   -m, --max_roc" << std::endl;
   std::cout << "   -c, --adc_cut" << std::endl;
   std::cout << "   -d, --dummy_triggers" << std::endl;
   std::cout << "   -h, --help" << std::endl;
   exit(1);
}

void ProcessArgs(int argc, char** argv) {
   std::cout << "#dummy_triggers=" << arguments.dummy_triggers << std::endl;

   while (true) {
      const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
      if (-1 == opt)
         break;
      switch (opt) {
         case 'w':
            arguments.spiroc_raw_filename = optarg;
            break;
         case 'v':
            arguments.reject_validated = true;
            break;
         case 'r':
            arguments.correlation_shift = std::stoi(optarg);
            break;
         case 'u':
            arguments.run_number = std::stoi(optarg);
            break;
         case 'l':
            arguments.bxid_length = std::atof(optarg);
            break;
         case 'm':
            arguments.max_rocs = std::atoi(optarg);
            break;
         case 'c':
            arguments.adc_cut = std::atoi(optarg);
            break;
         case 'd':
            arguments.dummy_triggers = std::atoi(optarg);
            break;
         case 'b':
            arguments.debug = true;
            break;
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

void prefetch_information(const struct arguments_t& arguments,
      std::map<uint32_t, std::map<uint16_t, bool> >& ROCtriggers,
      std::map<int, u_int64_t>& startTSs,
      std::map<int, u_int64_t>& stopTSs,
      std::map<int, u_int64_t>& busyTSs,
      std::map<int, int>& memcells) {
   unsigned char buf[4096];
   FILE *fp;
   if (!(fp = fopen(arguments.spiroc_raw_filename, "r"))) {
      perror("#Unable to open the spiroc raw file\n");
      return;
   }

   uint8_t lda = 0;
   uint8_t port = 0;
   u_int32_t headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   u_int32_t ROcycle = -1;
   u_int64_t TS = 0;
   u_int64_t lastTS = 0;
   u_int64_t lastStartTS = 0;
   u_int64_t lastStopTS = 0;
   int within_ROC = 0;
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
      if (b != 0xCD) {
         //printf("!");
         continue;/*try to look for second 0xCD. restart if not found*/
      }

      freadret = fread(&headlen, sizeof(headlen), 1, fp);
      freadret = fread(&headinfo, sizeof(headinfo), 1, fp);
      lda = headinfo & 0xFF;
      port = (headinfo >> 8) & 0xFF;
      unsigned int errors = (headinfo >> 16) & 0xFF;
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
         int trigid = ((int) buf[6]) + (((int) buf[7]) << 8); //possible trigid
         if (fread(buf, 8, 1, fp) <= 0) {
            std::cout << "error read 3" << std::endl;
            continue;
         }
         if ((buf[6] != 0xAB) || (buf[7] != 0xAB)) {
            std::cout << "error read 4: packet not finished with abab" << std::endl;
            continue;
         }
         TS = (u_int64_t) buf[0] +
               ((u_int64_t) buf[1] << 8) +
               ((u_int64_t) buf[2] << 16) +
               ((u_int64_t) buf[3] << 24) +
               ((u_int64_t) buf[4] << 32) +
               ((u_int64_t) buf[5] << 40);
         switch (type) {
            case 0x01: //start_acq
               within_ROC = 1;
               startTSs[ROcycle] = TS;
               lastStartTS = TS;
               break;
            case 0x02:               //stop_acq
               stopTSs[ROcycle] = TS;
               within_ROC = 0;
               lastStopTS = TS;
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
         lastTS = TS;
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
         if (memcell_filled > memcells[ROcycle]) memcells[ROcycle] = memcell_filled;
      } else {
         memcells[ROcycle] = memcell_filled;
      }
      fseek(fp, headlen & 0xFFFF, SEEK_CUR); //skip those packets
   }
   fclose(fp);
   std::cout << "#Trigger read finished. at ROC=" << ROcycle << std::endl;
}

int analyze_noise(const struct arguments_t & arguments) {
   unsigned char buf[4096];
   std::map<int, u_int64_t> startTSs; //maps start TS to ROC
   std::map<int, u_int64_t> stopTSs; //maps stop TS to ROC
   std::map<int, u_int64_t> busyTSs; //maps busy TS to ROC
   std::map<int, int> memcells; //maps maximum memcell filled in the ROC to ROC
//   std::vector<u_int64_t> startTSs; //maps start TS to ROC
//   std::vector<u_int64_t> stopTSs; //maps start TS to ROC
   std::map<uint32_t, double> acqLens; //maps (LDA.Port.chip) to length of acq.
//   std::map<uint32_t, int> acquisitions; //maps (LDA.Port.chip) to the number of acquisitions
   double globalLength = 0.0;
   // double busy; //conversion + readout time
   double blocked; //not going to acquisition, although the busy is down (temperature, HGCAL...)
   std::map<uint32_t, int> ASIChits; //maps LDA.Port.Chip to number of hits without bxid0
   std::map<uint32_t, int> hits; //maps LDA.Port.Chip.Channel to number of hits without bxid0
   std::map<uint32_t, int> hitsAfterAdcCut; //maps LDA.Port.Chip.Channel to number of hits without bxid0 after the ADC cut
   std::map<uint32_t, std::map<uint16_t, bool>> ROCtriggers; // map of map of triggers on ROC
   std::map<uint32_t, uint64_t> ADCsum; //maps LDA.Port.Chip.Channel to the ADC summary

   prefetch_information(arguments, ROCtriggers, startTSs, stopTSs, busyTSs, memcells);

   //calculate the global duration
   for (const auto& it : startTSs) {
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
   if (!(fp = fopen(arguments.spiroc_raw_filename, "r"))) {
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
   int tdc_gain = 0;
   int mismatches_hit = 0;
   int mismatches_gain = 0;
   int mismatches_length = 0;
   u_int32_t ROCLength = 0;
   /*BIF iteration variables*/
   // u_int32_t bif_iterator = 0; //the bif iterator points to the first registered trigger
   // u_int32_t bif_roc = 0;
   // int bif_bxid = 0;
//   u_int64_t bif_tdc = 0;
   // u_int32_t matches = 0;
   u_int32_t headlen, headinfo;
   unsigned char b;
   int freadret; //return code
   /* int roc_prev = -1; */
   u_int32_t ROcycle = -1;
//   printf("# bxid(BIF-DIF): -1 means, that DIF bxid is higher than it should be.\n");
//   printf("#1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\t15\n");
//   printf("#ROC\tbxid\tasic\tmcell\tchan\ttdc\tadc\thitb\tgainb\tBIF_TDC\tbxid(BIF-DIF)\tintra_bxid_event\tROCLen\tmem_filled\n");

   int ROC = 0;
   // int within_ROC = 0;
   u_int64_t TS = 0;
   u_int64_t lastTS = 0;
   u_int64_t lastStartTS = 0;
   u_int64_t lastStopTS = 0;
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
      if (b != 0xCD) {
         //printf(".");
         continue;/*try to look for second 0xCD. restart if not found*/
      }

      freadret = fread(&headlen, sizeof(headlen), 1, fp);
      freadret = fread(&headinfo, sizeof(headinfo), 1, fp);
      lda = headinfo & 0xFF;
      port = (headinfo >> 8) & 0xFF;
      unsigned int errors = (headinfo >> 16) & 0xFF;
      unsigned int status = (headinfo >> 24) & 0xFF;
      // skip unwanted packets:
      if (((port == 128) && ((headlen & 0xFFFF) == 8)) ||
            //            ((port==160) && ((headlen & 0xFFFF)==16)) || we want timestamp
            ((status == 0xa0) && ((headlen & 0xFFFF) == 16)) || //temp
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
      // if ((((headlen & 0xFFFF) - 12) % 146) && (port<0x60)) {
      //    printf("#Wrong header length: %d in port %d\n",(headlen & 0xFFFF),port);
      //    printf("#head=0x%08x %08x\n",headinfo,headlen);
      //    fseek(fp,headlen & 0xFFFF, SEEK_CUR);//skip those packets
      //    continue;
      // }
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
         printf("no spiroc data packet! #head=0x%08x %08x\n", headinfo, headlen);
         continue;
      }
//      fprintf(stdout,"#ROC: %d\n",ROcycle);
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8);            //extract the chipID from the packet
      if ((port == 9) && (asic > 0xFF)) { //DEBUG
         printf("#ERROR in chipid. length %d, modulo %d, ROC %d, ASIC %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle, asic);
         // printf("#");
         // for (int i=0; i<(headlen & 0xFFF); i++){
         //    printf("%02x",buf[i]);
         // }
         // printf("\n");
         continue;
      }
      if (((headlen & 0x0fff) - 12) % 146) { //if the length of the packet does not align with the number of memory cells
         mismatches_length++;
         printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d, port %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle, asic,
               port);
         // printf("#");
         // for (int i=0; i<(headlen & 0xFFF); i++){
         //    printf("%02x",buf[i]);
         // }
         // printf("\n");
         continue;
      }
      int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
//    printf("#memory cells: %d\n", memcell_filled);

      if (startTSs.count(ROcycle) && stopTSs.count(ROcycle)) {
         //OK, both timestamp exist
//      if (startTSs[ROcycle] && stopTSs[ROcycle]) {
//         std::cout << "timestamp for ROC " << ROcycle << "exists" << std::endl;
      } else {
//         std::cout << "timestamp for ROC " << ROcycle << " does not exist" << std::endl;
         continue;
      }
      u_int64_t ROCLength = stopTSs[ROcycle] - startTSs[ROcycle] - arguments.correlation_shift;
      acqLens[((lda << 16) | (port << 8) | (asic & 0xFF))] += 0;
      if (arguments.debug) {
         //uint64_t ROCLength2 = (buf[8 + 36 * 4 * 16] | (buf[8 + 36 * 4 * 16 + 1] << 8)) * arguments.bxid_length;
         int last_bxid = buf[8 + 36 * 4 * memcell_filled] | (buf[8 + 36 * 4 * memcell_filled + 1] << 8);
         std::cout << "#DEBUG length difference\t";
         std::cout << ((int64_t) ROCLength - (int64_t) last_bxid * arguments.bxid_length);
         std::cout << "\t" << ROCLength;
         std::cout << "\t" << last_bxid * arguments.bxid_length;
         std::cout << "\t" << last_bxid;
         std::cout << "\t" << ROcycle;
         std::cout << "\t" << asic;
         std::cout << "\t" << (unsigned int) port;
         std::cout << "\t" << memcell_filled;
         std::cout << "\t" << memcells[ROcycle];
         std::cout << "\t" << busyTSs[ROcycle] - startTSs[ROcycle] - arguments.correlation_shift;
         std::cout << "\t" << (int64_t) stopTSs[ROcycle] - (int64_t) busyTSs[ROcycle];
         std::cout << std::endl;
         //TODO calculate the shortening and store
         //TODO more asics in one layer should not shorten the cycle more!!!
         //acqLens[((lda << 16) | (port << 8) | (asic & 0xFF))] += ((double) 0.000000025) * ROCLength;
      }
      if (ROCLength > 4096 * arguments.bxid_length) {
         std::cout << "#Readout cycle length=" << ROCLength << " too long. ROC=" << ROcycle
               << " Startroc=" << startTSs[ROcycle] << " StopROC=" << stopTSs[ROcycle] << std::endl;
      }

      //fill the asic-wise information
//      acquisitions[((lda << 16) | (port << 8) | (asic & 0xFF))]++;
      ASIChits[((lda << 16) | (port << 8) | (asic & 0xFF))] += memcell_filled - arguments.dummy_triggers;         // - 1;

      for (memcell = arguments.dummy_triggers; memcell < memcell_filled; ++memcell) {
//         if ((arguments->memcell != -1) && (memcell != arguments->memcell)) continue;/*skip data from unwanted asic*/
         bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
               | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
         if (arguments.reject_validated && (ROCtriggers[ROcycle][bxid])) {
//                  std::cout << "trigger match !!!!!!!!!!!!!!!!!!!!!!!!!!!! ROC=" << ROcycle << " BXID=" << bxid << std::endl;
            ASIChits[((lda << 16) | (port << 8) | (asic & 0xFF))]--;
            continue;
         }
         for (channel = 0; channel < 36; ++channel) {
//            if ((arguments->channel != -1) && (channel != arguments->channel)) continue;/*ship data from unwanted channel*/
            tdc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1)]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 1] << 8);
            adc = buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2]
                  | (buf[8 + (35 - channel) * 2 + 36 * 4 * (memcell_filled - memcell - 1) + 36 * 2 + 1] << 8);
            adc_hit = (adc & 0x1000) ? 1 : 0;
            adc_gain = (adc & 0x2000) ? 1 : 0;
            tdc_hit = (tdc & 0x1000) ? 1 : 0;
            tdc_gain = (tdc & 0x2000) ? 1 : 0;
            tdc = tdc & 0x0fff;
            adc = adc & 0x0fff;
            if (adc_hit != tdc_hit) mismatches_hit++;
            if (adc_gain != tdc_gain) mismatches_gain++;

            //fill-in channel-wise information
            if (adc_hit) {
               hits[((lda << 24) | (port << 16) | ((asic & 0xFF) << 8) | (channel))]++;
               ADCsum[((lda << 24) | (port << 16) | ((asic & 0xFF) << 8) | (channel))] += adc;
               if (adc_gain && (adc > getMipCut(0.5, lda, port, asic, channel, memcell))) {
                  hitsAfterAdcCut[((lda << 24) | (port << 16) | ((asic & 0xFF) << 8) | (channel))]++;
               }
            }
         }
      }

   }
   fclose(fp);
   std::cout << "#Mismatched hit bits: " << mismatches_hit << std::endl;
   std::cout << "#Mismatched gain bits: " << mismatches_gain << std::endl;
   for (const auto &it : acqLens) {
      //it.first = LDA<<16 | port <<8 | chip
      std::cout << "#LDA\tport\tasic\tchannel\ttotal_lenth[s]\thits\thit_cut\tasicHit\tacqs\tfreq\tavglen\tavAdc\tADCsum" << std::endl;
      std::cout << "#1\t2\t3\t4\t5\t\t6\t7\t8\t9\t10\t11\t12\t13" << std::endl;
      uint32_t index2 = it.first << 8; //LDA<<24 | port<<16 | chip<<8 | channel
      for (int ch = 0; ch < 36; ch++) {
         std::cout << ((it.first >> 16) & 0xFF) << "\t" << std::flush;
         std::cout << ((it.first >> 8) & 0xFF) << "\t" << std::flush;
         std::cout << ((it.first >> 0) & 0xFF) << "\t" << std::flush;
         std::cout << ch << "\t" << std::flush;
         printf("%f\t", globalLength);
//         printf("%f\t", it.second);
         std::cout << hits[index2 | ch] << "\t" << std::flush;
         std::cout << hitsAfterAdcCut[index2 | ch] << "\t" << std::flush;
         std::cout << ASIChits[it.first] << "\t" << std::flush;
         std::cout << ROcycle << "\t" << std::flush;
         if (globalLength > 0.0001) {
            printf("%.1f\t", (((double) 1.0) * (hits[index2 | ch] - hitsAfterAdcCut[index2 | ch]) / globalLength));
         } else {
            printf("NaN\t");
         }
         std::cout << std::flush;
         printf("%.2f\t", 1000.0 * globalLength / ROcycle);
         std::cout << std::flush;
         if (hits[index2 | ch] != 0) {
            std::cout << ADCsum[index2 | ch] / hits[index2 | ch] << "\t" << std::flush;
         } else {
            std::cout << "NaN\t" << std::flush;
         }
         std::cout << ADCsum[index2 | ch] << "\t" << std::flush;
         std::cout << std::endl;
      }
      std::cout << "#---------------------------------------------------------------" << std::endl;
//      std::cout << "#acqlen LDA=" << ((it.first >> 16) & 0xFF)
//            << "  port=" << ((it.first >> 8) & 0xFF)
//            << "  chip=" << (it.first & 0xFF) << "  len=";
//      printf("%12f", it.second);
//      std::cout << std::endl;
   }
//   for (const auto &it : hits) {
//      std::cout << "#Hits LDA=" << ((it.first >> 24) & 0xFF)
//            << "  port=" << ((it.first >> 16) & 0xFF)
//            << "  chip=" << ((it.first >> 8) & 0xFF)
//            << "  chan=" << ((it.first) & 0xFF);
//      std::cout << "  hits=" << it.second << std::endl;
//   }
//   std::cout << "#---------------------------------------------------------------" << std::endl;
//   for (const auto &it : hitsAfterAdcCut) {
//      std::cout << "#Hits after cut: LDA=" << ((it.first >> 24) & 0xFF)
//            << "  port=" << ((it.first >> 16) & 0xFF)
//            << "  chip=" << ((it.first >> 8) & 0xFF)
//            << "  chan=" << ((it.first) & 0xFF);
//      std::cout << "  hits=" << it.second << std::endl;
//   }
//   std::cout << "#---------------------------------------------------------------" << std::endl;
//   for (const auto &it : ADCsum) {
//      std::cout << "#Hits after cut: LDA=" << ((it.first >> 24) & 0xFF)
//            << "  port=" << ((it.first >> 16) & 0xFF)
//            << "  chip=" << ((it.first >> 8) & 0xFF)
//            << "  chan=" << ((it.first) & 0xFF);
//      std::cout << "  hits=" << hits[it.first] << "  adcsum=" << it.second;
//      printf("  %12f", ((1.0f) * it.second) / hits[it.first]);
//      std::cout << "  hits=" << it.second << std::endl;
//   }
   return 0;
}

int main(int argc, char **argv) {
   argumentsInit(arguments);
   ProcessArgs(argc, argv);
   argumentsPrint(arguments);
   analyze_noise(arguments);
//   cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
   return 0;
}
