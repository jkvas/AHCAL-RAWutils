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

const char* const short_opts = "w:vrluhm";
const struct option long_opts[] = {
      { "spiroc_raw_filename", required_argument, nullptr, 'w' },
      { "reject_validated", no_argument, nullptr, 'v' },
      { "correlation_shift", required_argument, nullptr, 'r' },
      { "bxid_length", required_argument, nullptr, 'l' },
      { "run_number", required_argument, nullptr, 'u' },
      { "max_rocs", required_argument, nullptr, 'm' },
      { "adc_cut", required_argument, nullptr, 'c' },
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
}

void argumentsPrint(const struct arguments_t & arguments) {
   std::cout << "#spiroc_raw_filename=" << ((arguments.spiroc_raw_filename == nullptr) ? "(no)" : arguments.spiroc_raw_filename) << std::endl;
   std::cout << "#adc_cut=" << arguments.adc_cut << std::endl;
   std::cout << "#reject_validated=" << arguments.reject_validated << std::endl;
   std::cout << "#correlation_shift=" << arguments.correlation_shift << std::endl;
   std::cout << "#bxid_length=" << arguments.bxid_length << std::endl;
   std::cout << "#run_number=" << arguments.run_number << std::endl;
   std::cout << "#max_rocs=" << arguments.max_rocs << std::endl;
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
   std::cout << "   -h, --help" << std::endl;
   exit(1);
}

void ProcessArgs(int argc, char** argv) {
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

int analyze_noise(const struct arguments_t & arguments) {
   unsigned char buf[4096];
   std::map<int, u_int64_t> startTSs; //maps start TS to ROC
   std::map<int, u_int64_t> stopTSs; //maps start TS to ROC
//   std::vector<u_int64_t> startTSs; //maps start TS to ROC
//   std::vector<u_int64_t> stopTSs; //maps start TS to ROC
   std::map<uint32_t, double> acqLens; //maps (LDA.Port.chip) to length of acq.
   std::map<uint32_t, int> acquisitions; //maps (LDA.Port.chip) to the number of acquisitions
   // double busy; //conversion + readout time
   double blocked; //not going to acquisition, although the busy is down (temperature, HGCAL...)
   std::map<uint32_t, int> ASIChits; //maps LDA.Port.Chip to number of hits without bxid0
   std::map<uint32_t, int> hits; //maps LDA.Port.Chip.Channel to number of hits without bxid0
   std::map<uint32_t, int> hitsAfterAdcCut; //maps LDA.Port.Chip.Channel to number of hits without bxid0 after the ADC cut
   std::map<uint32_t, std::map<uint16_t, bool>> ROCtriggers; // map of map of triggers on ROC
   std::map<uint32_t, uint64_t> ADCsum; //maps LDA.Port.Chip.Channel to the ADC summary
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
         printf(".");
         continue;/*try to look for second 0xCD. restart if not found*/
      }
      
      freadret = fread(&headlen, sizeof(headlen), 1, fp);
      freadret = fread(&headinfo, sizeof(headinfo), 1, fp);
      lda = headinfo & 0xFF; 
      port = (headinfo >> 8) & 0xFF;
      unsigned int errors = (headinfo >> 16) & 0xFF;
      unsigned int status = (headinfo >> 24) & 0xFF;
      // skip unwanted packets:
      if ( ((port==128) && ((headlen & 0xFFFF)==8)) ||
           //            ((port==160) && ((headlen & 0xFFFF)==16)) || we want timestamp
           ((status==0xa0) && ((headlen & 0xFFFF)==16)) || //temp
           ((status==0x20) && ((headlen & 0xFFFF)==12))//EOR packet
         ) {
         fseek(fp,headlen & 0xFFFF, SEEK_CUR);//skip those packets
         continue;
      }
      if (((headlen & 0xFFFF) > 4095) || ((headlen & 0xFFFF) < 4)) {
         printf("#Wrong header length: %d\n", headlen & 0xffff);
         printf("#head=0x%08x %08x\n",headinfo,headlen);
         continue;
      }
      // if ((((headlen & 0xFFFF) - 12) % 146) && (port<0x60)) {
      //    printf("#Wrong header length: %d in port %d\n",(headlen & 0xFFFF),port);
      //    printf("#head=0x%08x %08x\n",headinfo,headlen);
      //    fseek(fp,headlen & 0xFFFF, SEEK_CUR);//skip those packets
      //    continue;
      // }
      if ((headlen & 0xFFFF) == 0x10) {
         //////////////////////////////////////////////////////////////////////////////
         // timestamp
         //////////////////////////////////////////////////////////////////////////////
         if (fread(buf, 8, 1, fp) <= 0) {
            std::cout << "error read 2" << std::endl;
            continue;
         }
         if ((buf[0] != 0x45) || (buf[1] != 0x4D) || (buf[2] != 0x49) || (buf[3] != 0x54)) {
            fseek(fp, 8, SEEK_CUR);
            continue;
         }
         int newROC = (headlen >> 16) & 0xFF;
         ROC = update_counter_modulo(ROC, newROC, 256, 100);
	 if (arguments.max_rocs && (ROC>arguments.max_rocs)) {
	    std::cout<<"#Maximum ROC number reached"<<std::endl;
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
         if (type != 0x10) { //not trigger
            if (type == 0x01) {      //start acq
               // within_ROC = 1;
//               ROC = update_counter_modulo(ROC, newROC, 256, 1);
//               std::cout << "#Start_ACQ ROC=" << ROC << std::endl;
               startTSs[ROC] = TS;
               lastStartTS = TS;
            }
            if (type == 0x02) {      //stop_acq
//               ROC = update_counter_modulo(ROC, newROC, 256, 1);
//               std::cout << "#Stop_ACQ ROC=" << ROC << std::endl;
               stopTSs[ROC] = TS;
               // within_ROC = 0;
               lastStopTS = TS;
            }
            // if (type == 0x20) within_ROC = 2; //busy raised, but did not yet received stop acq
            //         fseek(fp, 8, SEEK_CUR);
//            int increment = (newROC - ROC) & 0xFF;
//            if ((increment > 10) || (increment < -2)) {
//               std::cout << "#ERROR wrong increment of ROC: " << increment << " . ROC=" << ROC << ", newROC=" << newROC << std::endl;
//               continue;
//            }
//            //      fprintf(stdout, "old ROC=%d, increment=%d\n", ROC, increment);
//            ROC = ROC + increment;
            //      fprintf(stdout, "%05d\t%05d\t%llu\t%d\t%d\t%lli\t#trigid,roc,TS,withinROC,ROCincrement,triggerTSinc\n", trigid, ROC, TS, within_ROC, increment, TS - lastTS);
            //TODO triggers
//            if ((bif_data_index < C_MAX_BIF_EVENTS) && (within_ROC == 1)) {/*if the data index is in range*/
//               bif_data[bif_data_index].ro_cycle = (u_int32_t) ROC; // - first_shutter;
//               bif_data[bif_data_index].tdc = (TS - lastStartTS); // << 5;
//               bif_data[bif_data_index++].trig_count = trigid;
//            }
            lastTS = TS;
            continue;
         } else {
//            std::cout << "#Trigger TS packet ROC=" << ROC << std::endl;
            uint16_t trigBXID = ((TS - lastStartTS) - arguments.correlation_shift) / arguments.bxid_length;
            ROCtriggers[ROC][trigBXID] = true;
         }
         //////////////////////////////////////////////////////////////////////////////
         // end timestamp
         //////////////////////////////////////////////////////////////////////////////
//         fseek(fp, headlen & 0xFFFF, SEEK_CUR);         //skip timestamp packets
         continue;
      }
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] != 0x41) || (buf[1] != 0x43) || (buf[2] != 0x48) || (buf[3] != 0x41)) {
         printf("no spiroc data packet! #head=0x%08x %08x\n",headinfo,headlen);
         continue;
      }
//      fprintf(stdout,"#ROC: %d\n",ROcycle);
      asic = buf[(headlen & 0xFFF) - 1 - 3] | ((buf[(headlen & 0xFFF) - 1 - 2]) << 8);            //extract the chipID from the packet
      if ((port==9) && (asic>0xFF)) { //DEBUG
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
         printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d, port %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle, asic, port);
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
//      if (startTSs[ROcycle] && stopTSs[ROcycle]) {
//         std::cout << "timestamp for ROC " << ROcycle << "exists" << std::endl;
      } else {
//         std::cout << "timestamp for ROC " << ROcycle << " does not exist" << std::endl;
         continue;
      }
      u_int64_t ROCLength = stopTSs[ROcycle] - startTSs[ROcycle] - arguments.correlation_shift;
      if (memcell_filled == 16) ROCLength = (buf[8 + 36 * 4 * 16] | (buf[8 + 36 * 4 * 16 + 1] << 8)) * arguments.bxid_length;
      if (ROCLength > 4096 * arguments.bxid_length) {
         std::cout << "#Readout cycle length=" << ROCLength << " too long. ROC=" << ROcycle
               << " Startroc=" << startTSs[ROcycle] << " StopROC=" << stopTSs[ROcycle] << std::endl;
      }

//      int first_bif_iterator = get_first_iterator2(arguments, bif_data, ROcycle);

      //fill the asic-wise information
      acquisitions[((lda << 16) | (port << 8) | (asic & 0xFF))]++;
      acqLens[((lda << 16) | (port << 8) | (asic & 0xFF))] += ((double) 0.000000025) * ROCLength;
      ASIChits[((lda << 16) | (port << 8) | (asic & 0xFF))] += memcell_filled - 1;

      for (memcell = 1; memcell < memcell_filled; ++memcell) {
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
      std::cout << "#LDA\tport\tasic\tchannel\ttotal_lenth[s]\thits\thit_cut\tasicHit\tacqs\tfreq\tavglen\tavAdc\tADCsum" << std::endl;
      std::cout << "#1\t2\t3\t4\t5\t\t6\t7\t8\t9\t10\t11\t12\t13" << std::endl;
      uint32_t index2 = it.first << 8;
      for (int ch = 0; ch < 36; ch++) {
         std::cout << ((it.first >> 16) & 0xFF) << "\t" << std::flush;
         std::cout << ((it.first >> 8) & 0xFF) << "\t" << std::flush;
         std::cout << ((it.first >> 0) & 0xFF) << "\t" << std::flush;
         std::cout << ch << "\t" << std::flush;
         printf("%f\t", it.second);
         std::cout << hits[index2 | ch] << "\t" << std::flush;
         std::cout << hitsAfterAdcCut[index2 | ch] << "\t" << std::flush;
         std::cout << ASIChits[it.first] << "\t" << std::flush;
         std::cout << acquisitions[it.first] << "\t" << std::flush;
         if (it.second > 0.0001) {
            printf("%.1f\t", (((double) 1.0) * (hits[index2 | ch] - hitsAfterAdcCut[index2 | ch]) / it.second));
         } else {
            printf("NaN\t");
         }
         std::cout << std::flush;
         printf("%.2f\t", 1000.0 * it.second / acquisitions[it.first]);
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
