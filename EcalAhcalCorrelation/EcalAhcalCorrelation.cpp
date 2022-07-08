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
#include <sys/types.h>
//#include <cstdio>
//#include <cstdlib>
#include <iostream>
#include <string>

//#include "KLauS_ROPackets.h"
//#include "KLauS_Data.h"
#define SINGLE_SKIROC_EVENT_SIZE (129*2)

using namespace std;

const char *const short_opts = "we:celumbt";
const struct option long_opts[] = {
      { "spiroc_raw_filename", required_argument, nullptr, 'w' },
      { "spiroc_hits_cut", required_argument, nullptr, 's' },
      { "ecal_raw_filename", required_argument, nullptr, 'e' },
      { "ecal_hits_cut", required_argument, nullptr, 'c' },
      // { "correlation_shift", required_argument, nullptr, 'r' },
      { "bxid_length", required_argument, nullptr, 'l' },
      { "run_number", required_argument, nullptr, 'u' },
      { "max_rocs", required_argument, nullptr, 'm' },
      { "debug", no_argument, nullptr, 'b' },
      { "help", no_argument, nullptr, 'h' },
      { nullptr, 0, nullptr, 0 }
};

struct arguments_t {
      char *spiroc_raw_filename;
      char *ecal_raw_filename;
      int spiroc_hits_cut;
      int ecal_hits_cut;
      bool reject_validated;
      int correlation_shift;
      int bxid_length;
      int run_number;
      int max_rocs;
      int dummy_triggers;
      bool debug;
};

struct arguments_t arguments;

std::map<int, std::map<int, int>> ecalBxids; //bxids in ecal
std::map<int, std::map<int, int>> hcalBxids; //bxids in ecal

void argumentsInit(struct arguments_t &arguments) {
   arguments.spiroc_raw_filename = nullptr;
   arguments.spiroc_hits_cut = 2;
   arguments.ecal_raw_filename = nullptr;
   arguments.ecal_hits_cut = 5;
   arguments.correlation_shift = 286;
   arguments.bxid_length = 8;
   arguments.run_number = 0;
   arguments.max_rocs = 0;
   arguments.debug = false;
}

void argumentsPrint(const struct arguments_t &arguments) {
   std::cout << "#spiroc_raw_filename=" << ((arguments.spiroc_raw_filename == nullptr) ? "(no)" : arguments.spiroc_raw_filename) << std::endl;
   std::cout << "#ecal_raw_filename=" << ((arguments.ecal_raw_filename == nullptr) ? "(no)" : arguments.ecal_raw_filename) << std::endl;
   std::cout << "#correlation_shift=" << arguments.correlation_shift << std::endl;
   std::cout << "#bxid_length=" << arguments.bxid_length << std::endl;
   std::cout << "#run_number=" << arguments.run_number << std::endl;
   std::cout << "#max_rocs=" << arguments.max_rocs << std::endl;
   std::cout << "#debug=" << arguments.debug << std::endl;
   if (arguments.debug) {
      std::cout
            << "#length_to_last_bxid\tROCLength\tlast_bxid[25nstics]\tlast_bxid\tROcycle\tasic\tport\tfilled_mems\tRoc_Max_memcel\tbusy-start\tstop-busy\t#DEBUG length difference"
            << std::endl;
   }
}

void PrintHelp() {
   std::cout << "prints the noise statistics of ahcal raw file dumped by eudaq" << std::endl;
   std::cout << "Options: " << std::endl;
   std::cout << "   -w, --spiroc_raw_filename" << std::endl;
   std::cout << "   -e, --ecal_raw_filename" << std::endl;
   std::cout << "   -r, --correlation_shift" << std::endl;
   std::cout << "   -l, --bxid_length" << std::endl;
   std::cout << "   -u, --run_number" << std::endl;
   std::cout << "   -m, --max_roc" << std::endl;
   std::cout << "   -h, --help" << std::endl;
   exit(1);
}

void ProcessArgs(int argc, char **argv) {
   std::cout << "#dummy_triggers=" << arguments.dummy_triggers << std::endl;

   while (true) {
      const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
      if (-1 == opt)
         break;
      switch (opt) {
         case 'w':
            arguments.spiroc_raw_filename = optarg;
            break;
         case 's':
            arguments.spiroc_hits_cut = std::stoi(optarg);
            break;
         case 'e':
            arguments.ecal_raw_filename = optarg;
            break;
         case 'c':
            arguments.ecal_hits_cut = std::stoi(optarg);
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

void prefetch_information(const struct arguments_t &arguments, std::map<uint32_t, std::map<uint16_t, bool> > &ROCtriggers, std::map<int, u_int64_t> &startTSs,
      std::map<int, u_int64_t> &stopTSs, std::map<int, u_int64_t> &busyTSs, std::map<int, int> &memcells) {
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

inline unsigned short grayToBinary(unsigned short num) {
   unsigned short mask;
   for (mask = num >> 1; mask != 0; mask = mask >> 1)
      num = num ^ mask;
   return num;
}

int cycleIDDecoding(std::vector<unsigned char> ucharValFrameVec) {

   int i = 0;
   int n = 0;
   // metadata
   int result = 0;
   for (n = 0; n < 16; n++) {
      result += ((unsigned int) (((ucharValFrameVec.at(2 * n + 1 + 2) & 0xC0) >> 6) << (30 - 2 * i)));
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

void printEcalBxids(const struct arguments_t &arguments) {
   for (auto roc_it : ecalBxids) {
      std::cout << "ECAL ROC " << roc_it.first;
      for (auto bxid_it : roc_it.second) {
         if (bxid_it.second >= arguments.ecal_hits_cut) {
            std::cout << " " << bxid_it.first << "(" << bxid_it.second << "),";
         }
      }
      std::cout << std::endl;
   }
}

int fetchEcalBXIDs(const struct arguments_t &arguments) {
   int datasize = 0;
   unsigned char b;
   unsigned char bufx[4096];
   int freadret; //return code

   FILE *fp;
   if (!(fp = fopen(arguments.ecal_raw_filename, "r"))) {
      perror("#Unable to open the ecal raw file\n");
      return -1;
   }
   while (1) {
      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         printf("#unable to read / EOF\n");
         break;
      }
      if (b != 0xAB) {
         std::cout << "No 0xAB, but " << b << std::endl;
         continue;/*try to look for first 0xCD. restart if not found*/
      }
      freadret = fread(&b, sizeof(b), 1, fp);
      if (!freadret) {
         printf("#unable to read / EOF\n");
         break;
      }
      if (b != 0xCD) {
         std::cout << "No 0xCD, but " << b << std::endl;
         continue;/*try to look for first 0xCD. restart if not found*/
      }
      freadret = fread(&bufx, 6, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      datasize = ((unsigned int) bufx[1] << 8) | bufx[0];
      freadret = fread(&bufx, datasize + 2, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      unsigned short trailerWord = ((unsigned short) bufx[datasize + 2 - 1] << 8) + bufx[datasize + 2 - 2];
//      cout << "Trailer Word hex:" << hex << trailerWord << " dec:" << dec << trailerWord << std::endl;
      std::vector<unsigned char> ucharValFrameVec;
      if (trailerWord == 0x9697) {
         for (int ibuf = 0; ibuf < datasize + 8; ibuf++) {
            ucharValFrameVec.push_back(bufx[ibuf]);
//            std::cout << datasize << " LOOP, ibuf=" << ibuf << " hex:" << hex << bufx[ibuf] << "  dec:" << dec << bufx[ibuf] << std::endl;
         }
         int ROC = cycleIDDecoding(ucharValFrameVec);
         int nbOfSingleSkirocEventsInFrame = (int) ((datasize - 2 - 2) / SINGLE_SKIROC_EVENT_SIZE);
         for (int n = 0; n < nbOfSingleSkirocEventsInFrame; n++) {
            int rawValue = (unsigned int) ucharValFrameVec[datasize - 2 * (n + 1) - 2]
                  + (((unsigned int) ucharValFrameVec[datasize - 1 - 2 * (n + 1)] & 0x0F) << 8);
//            int sca_ascii = nbOfSingleSkirocEventsInFrame - n - 1;
//            int sca = nbOfSingleSkirocEventsInFrame - (sca_ascii + 1);
            int bxid = Convert_FromGrayToBinary(rawValue, 12);
            int BxidRocOccurences = ecalBxids[ROC][bxid];
            ecalBxids[ROC][bxid] = BxidRocOccurences + 1;
         };
      }
   }
   std::cout << "#ecal read finished" << std::endl;
   return 0;
//    }     // }//while1
}

void printHcalBxids(const struct arguments_t &arguments)
      {
   for (auto roc_it : hcalBxids) {
      std::cout << "AHCAL ROC " << roc_it.first;
      for (auto bxid_it : roc_it.second) {
         if (bxid_it.second >= arguments.spiroc_hits_cut) {
            std::cout << " " << bxid_it.first << "(" << bxid_it.second << "),";
         }
      }
      std::cout << std::endl;
   }
}

int fetchAhcalBXIDs(const struct arguments_t &arguments) {
   unsigned char buf[4096];
   FILE *fp;
   if (!(fp = fopen(arguments.spiroc_raw_filename, "r"))) {
      perror("#Unable to open the spiroc raw file\n");
      return -1;
   }
   /*spiroc datafile iteration*/
   uint8_t port = 0;
   int bxid = 0;
   int asic = 0;
   int memcell = 0;
   int mismatches_length = 0;
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

//   int ROC = 0;
//   // int within_ROC = 0;
//   u_int64_t TS = 0;
//   u_int64_t lastTS = 0;
//   u_int64_t lastStartTS = 0;
//   u_int64_t lastStopTS = 0;
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
      //lda = headinfo & 0xFF;
      //port = (headinfo >> 8) & 0xFF;
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
      ROcycle = update_counter_modulo(ROcycle, ((headlen >> 16) & 0xFF), 0x100, 100);
      freadret = fread(buf, headlen & 0xFFF, 1, fp);
      if (!freadret) {
         printf("#unable to read the complete packet / EOF\n");
         break;
      }
      if ((buf[0] == 0x41) && (buf[1] == 0x43) && (buf[2] == 0x48) && (buf[3] == 0x41)) { //SPIROC
         if (((headlen & 0x0fff) - 12) % 146) { //if the length of the packet does not align with the number of memory cells
            mismatches_length++;
            printf("#ERROR wrong AHCAL packet length %d, modulo %d, ROC %d, ASIC %d, port %d\n", headlen & 0x0fff, ((headlen & 0x0fff) - 12) % 146, ROcycle,
                  asic, port);
            continue;
         }
         int memcell_filled = ((headlen & 0xFFF) - 8 - 2 - 2) / (36 * 4 + 2);
         //=(buf[8 + 36 * 4 * 16] | (buf[8 + 36 * 4 * 16 + 1] << 8)) * arguments.bxid_length;//TODO this is wrong? Prefilled lengths should be should be
         for (memcell = arguments.dummy_triggers; memcell < memcell_filled; ++memcell) {
            //         if ((arguments->memcell != -1) && (memcell != arguments->memcell)) continue;/*skip data from unwanted asic*/
            bxid = buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1)]
                  | (buf[8 + 36 * 4 * memcell_filled + 2 * (memcell_filled - memcell - 1) + 1] << 8);
            int hits = hcalBxids[ROcycle][bxid];
            hcalBxids[ROcycle][bxid] = hits + 1;
         }
      }
   }
   return 0;
}

void makeCorrelations(const struct arguments_t &arguments) {
   std::map<int, std::map<uint8_t, uint16_t>> correlations; //<roc,<difference, counts>>
   int corr_search_min = -5;
   int corr_search_max = 5;
   for (auto hcalRocIt : hcalBxids) {
      for (auto hcalBxidIt : hcalRocIt.second) {         //hcal ROC and bxid in the memory here
         if (hcalBxidIt.second >= arguments.spiroc_hits_cut) {
            for (int diff = corr_search_min; diff <= corr_search_max; diff++) {
               auto ecalRocIt = ecalBxids.find(hcalRocIt.first+diff);
               if (ecalRocIt != ecalBxids.end()) {         //ecalroc exists in the map
                  for (auto ecalBxidIt : ecalRocIt->second) {
                     if (ecalBxidIt.second >= arguments.ecal_hits_cut) {
                        if (ecalBxidIt.first == hcalBxidIt.first) {
                           uint16_t prevCorrelations = correlations[hcalRocIt.first][diff];
                           correlations[hcalRocIt.first][diff] = prevCorrelations + 1;
                        }
                     }
                  }
               }
            }
         }
      }
   }
   std::cout << "#ROC\td-5\td-4\t-3\t-2\t-1\t0\t1\2\t\3\t4\t5\tmax"<<std::endl;
   for (auto roc : correlations) {
      std::cout << roc.first << "\t";
      int maxcorrcount=0;
      int maxcorrdiff=0;
      for (int i = corr_search_min; i <= corr_search_max; i++) {
         if (roc.second[i]>maxcorrcount){
            maxcorrcount=roc.second[i];
            maxcorrdiff=i;
         }
         std::cout << roc.second[i] << "\t";
      }
      std::cout <<maxcorrdiff<<"\t"<<maxcorrcount<<"\t"<< "#correlation";
      std::cout << std::endl;
   }
}

int main(int argc, char **argv) {
   argumentsInit(arguments);
   ProcessArgs(argc, argv);
   fetchEcalBXIDs(arguments);
   fetchAhcalBXIDs(arguments);

   argumentsPrint(arguments);
//   printEcalBxids(arguments);
   // printHcalBxids(arguments);
   makeCorrelations(arguments);
//   cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
   return 0;
}
