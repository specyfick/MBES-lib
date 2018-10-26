#ifndef KONGSBERG_CPP
#define KONGSBERG_CPP


#include <string>
#include <cstdio>
#include <iostream>
#include <cmath>

#include "../MbesParser.hpp"
#include "../../utils/NmeaUtils.hpp"
#include "../../utils/TimeUtils.hpp"

#define STX 0x02
#define ETX 0x03https://blog.github.com/2016-02-01-working-with-submodules/

#pragma pack(1)
typedef struct{
    uint32_t        size; //Size is computed starting from STX, so it excludes this one
	unsigned char	stx;
	unsigned char	type;
    uint16_t        modelNumber;
    uint32_t        date;
    uint32_t        time;
    uint16_t        counter;
    uint16_t        serialNumber;
} KongsbergHeader;
#pragma pack()


#pragma pack(1)
typedef struct{
    uint16_t NumEntries;
    uint16_t deltaTime; //time in milliseconds since record start
    int16_t  roll;    //in 0.01 degrees
    int16_t  pitch;   //in 0.01 degrees
    int16_t  heave;   //in cm
    int16_t  heading; //in 0.01 degrees
} KongsbergAttitudeEntry;
#pragma pack()

#pragma pack(1)
typedef struct{
    int32_t  lattitude;          // decimal degrees * 20,000,000
    int32_t  longitude;          // decimal degrees * 20,000,000
    uint16_t fixQuality;        // in cm
    uint16_t speedOverGround;   // in cm/s
    uint16_t courseOverGround;  // in 0.01deg
    uint16_t headingOverGround; // in 0.01deg
    uint8_t  positionSystemDescriptor;
    uint8_t  inputDatagramBytes; // Number of bytes in input datagram
    char     inputDatagram[];

    //(...)  input datagram etc. we don't need that much

} KongsbergPositionDatagram;
#pragma pack()



class KongsbergParser : public MbesParser{
        public:
        KongsbergParser();
        ~KongsbergParser();

        //interface methods
        void parse(std::string filename);
        void processAttitude(uint64_t microEpoch,double heading,double pitch,double roll);
        void processPosition(uint64_t microEpoch,double longitude,double latitude,double height);

        private:
        void processDatagram(KongsbergHeader & hdr,unsigned char * datagram);
        void processDepth(KongsbergHeader & hdr,unsigned char * datagram);
        void processWaterHeight(KongsbergHeader & hdr,unsigned char * datagram);
        void processAttitudeDatagram(KongsbergHeader & hdr,unsigned char * datagram);
        void processPositionDatagram(KongsbergHeader & hdr,unsigned char * datagram);
        void processQualityFactor(KongsbergHeader & hdr,unsigned char * datagram);
        void processSeabedImageData(KongsbergHeader & hdr,unsigned char * datagram);

        long convertTime(long datagramDate,long datagramTime);
};

KongsbergParser::KongsbergParser(){

}

KongsbergParser::~KongsbergParser(){

}

void KongsbergParser::parse(std::string filename){
	FILE * file = fopen(filename.c_str(),"rb");

	if(file){
		while(!feof(file)){
			//Lire datagramHeader
	        	KongsbergHeader hdr;
			int elementsRead = fread (&hdr,sizeof(KongsbergHeader),1,file);

			if(elementsRead == 1){
				//Verifier la presence du caractere de debut de trame
				if(hdr.stx==STX){
					//Allouer l'espace requis pour le contenu du datagramme
			                unsigned char * buffer = (unsigned char*)malloc(hdr.size-sizeof(KongsbergHeader)+sizeof(uint32_t));

                			elementsRead = fread(buffer,hdr.size-sizeof(KongsbergHeader)+sizeof(uint32_t),1,file);

					processDatagram(hdr,buffer);

					free(buffer);
				}
				else{
					printf("%02x",hdr.size);
					throw "Bad datagram";
					//TODO: rejct bad datagram, maybe log it
				}
            }
		}

		fclose(file);
	}
	else{
		throw "Couldn't open file " + filename;
	}
}

void KongsbergParser::processDatagram(KongsbergHeader & hdr,unsigned char * datagram){

	printf("-------------------------------------\n");
	printf("Datagram has %d bytes\n",hdr.size);
        printf("Datagram type: %c\n",hdr.type);
        printf("EM Model number: %d\n",hdr.modelNumber);
        printf("Date: %d\n",hdr.date);
        printf("Seconds since midnight: %d\n",hdr.time);
        printf("Counter: %d\n",hdr.counter);
        printf("Serial number: %d\n",hdr.serialNumber);

	switch(hdr.type){
		case 'A':
        		processAttitudeDatagram(hdr,datagram);
        	break;

		case 'D':
			processDepth(hdr,datagram);
        	break;

        case 'E':
            //process echosounder data
            //processDepth(hdr,datagram);
        break;

        case 'O':
            //processQualityFactor(hdr,datagram);
        break;

        case 'P':
            processPositionDatagram(hdr,datagram);
        break;

		case 'h':
            //processWaterHeight(hdr,datagram);
		break;

		case 'Y':
            //processSeabedImageData(hdr,datagram);
		break;

		default:
			printf("Unknown type %c\n",hdr.type);
		break;
	}

    printf("-------------------------------------\n");
}

void KongsbergParser::processDepth(KongsbergHeader & hdr,unsigned char * datagram){
	//printf("TODO: parse depth data\n");
}

void KongsbergParser::processWaterHeight(KongsbergHeader & hdr,unsigned char * datagram){
    //printf("TODO: parse height data\n");
}

void KongsbergParser::processAttitudeDatagram(KongsbergHeader & hdr,unsigned char * datagram){
    uint16_t nEntries = ((uint16_t*)datagram)[0];

    KongsbergAttitudeEntry * p = (KongsbergAttitudeEntry*)datagram + 1;

    uint64_t microEpoch = convertTime(hdr.date,hdr.time);

    for(unsigned int i = 0;i<nEntries;i++){
        processAttitude(microEpoch + p[i].deltaTime * 1000,(double)p[i].heading/(double)100,(double)p[i].pitch/(double)100,(double)p[i].roll/(double)100);
    }
}

long KongsbergParser::convertTime(long datagramDate,long datagramTime){
    int year = datagramDate / 10000;
    int month = (datagramDate - (datagramDate / 10000))/100;
    int day = datagramDate - ((datagramDate - (datagramDate / 10000))/100);

    return build_time(year,month,day,datagramTime);
}

void KongsbergParser::processAttitude(uint64_t microEpoch, double heading, double pitch, double roll){
    printf("A %lu %lf %lf %lf\n",microEpoch,heading,pitch,roll);
}



void KongsbergParser::processPositionDatagram(KongsbergHeader & hdr,unsigned char * datagram){
        KongsbergPositionDatagram * p = (KongsbergPositionDatagram*) datagram;

        uint64_t microEpoch = convertTime(hdr.date,hdr.time);

        //printf("%s",p->inputDatagram);

        double longitude = (double)p->longitude/(double)20000000;
        double latitude  = (double)p->lattitude/(double)20000000;

        std::string inputDatagram(p->inputDatagram);

        std::cout << inputDatagram << std::endl;

        double height = std::numeric_limits<double>::quiet_NaN();

        //Extract ellipsoidal height from input datagram
        if(inputDatagram.find("GGK") != std::string::npos){
            height = NmeaUtils::extractHeightFromGGK(inputDatagram);

        }
        else if(inputDatagram.find("GGA") != std::string::npos){
            height = NmeaUtils::extractHeightFromGGA(inputDatagram);
        }
        else{
            //NO POSITION, whine about this
            std::cerr << "No ellipsoidal height found in input datagram: " << inputDatagram << std::endl;
        }

        if(!std::isnan(height)){
            processPosition(microEpoch,longitude,latitude,height);
        }
}

void KongsbergParser::processQualityFactor(KongsbergHeader & hdr,unsigned char * datagram){
        //printf("TODO: parse quality factor data\n");
}

void KongsbergParser::processSeabedImageData(KongsbergHeader & hdr,unsigned char * datagram){
	//printf("TODO: parse Seabed Image Data\n");
}

void KongsbergParser::processPosition(uint64_t microEpoch,double longitude,double latitude,double height){

}


#endif