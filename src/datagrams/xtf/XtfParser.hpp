#ifndef XTFPARSER_HPP
#define XTFPARSER_HPP

#include <string>
#include <stdio.h>
#include <string.h>
#include <cstdio>
#include "XtfTypes.hpp"
#include "../MbesParser.hpp"
#include "../../utils/TimeUtils.hpp"

#define MAGIC_NUMBER 123
#define PACKET_MAGIC_NUMBER 0xFACE

/**
 * @author Guillaume Morissette
 *
 * Lit un fichier XTF et appelle un ensemble de callbacks pour chacun des types de datagrammes
 * Par defaut, les callbacks ne font qu'imprimer les informations de celui-ci. Une application 
 * pourra surcharger celles-ci afin d'effectuer des operations specialisees.
 */
class XtfParser : public MbesParser{
	public:
		XtfParser();
		~XtfParser();

		void parse(std::string & filename);
		int getTotalNumberOfChannels();

	        //Interface methods
	        void processPing(uint64_t microEpoch,long id, double beamAngle,double tiltAngle,double twoWayTravelTime,uint32_t quality,uint32_t intensity);
	        void processAttitude(uint64_t microEpoch,float heading,float pitch,float roll);
	        void processPosition(uint64_t microEpoch,double longitude,double latitude,double height);
	        void processSwathStart(double soundVelocity);

	protected:
		void processPacketHeader(XtfPacketHeader & hdr);
		void processPacket(XtfPacketHeader & hdr,unsigned char * packet);
	        void processPingHeader(XtfPingHeader & hdr);
	        void processFileHeader(XtfFileHeader & hdr);
	        void processChanInfo(XtfChanInfo & c);

		XtfFileHeader fileHeader;
};

XtfParser::XtfParser(){

}

XtfParser::~XtfParser(){

}

void XtfParser::parse(std::string & filename){
	FILE * file = fopen(filename.c_str(),"rb");

        if(file){
                //Lire Header
		memset(&fileHeader,0,sizeof(XtfFileHeader));
		int elementsRead = fread (&fileHeader,sizeof(XtfFileHeader),1,file);

		if(elementsRead==1){
			if(fileHeader.FileFormat == MAGIC_NUMBER){

				processFileHeader(fileHeader);

				int channels = this->getTotalNumberOfChannels();

				//Lire structs CHANINFO dans le header
				int channelsInHeader = (channels > 6)?6:channels;

				for(int i=0;i<channelsInHeader;i++){
					processChanInfo(fileHeader.Channels[i]);
				}

				//Lire les structs CHANINFO qui suivent le header
				if(channels>6){
					int channelsLeft = channels;
					XtfChanInfo buf[8];

					do{
						memset(buf,0,sizeof(XtfChanInfo)*8);
						elementsRead = fread(&buf,sizeof(XtfChanInfo),8,file);

						if(elementsRead == 8){
							for(int i=0;i<8;i++){
								if(channelsLeft > 0){
									processChanInfo(buf[i]);
									channelsLeft--;
								}
								else{
									break;
								}
							}
						}
						else{
							//TODO: whine and log error while reading
							printf("Error while reading CHANINFO\n");
						}
					}
					while(channelsLeft > 0);
				}

				//Lire packets
				while(!feof(file)){
					// parse a packet header
					XtfPacketHeader packetHeader;

					elementsRead = fread (&packetHeader,sizeof(XtfPacketHeader),1,file);

					if(elementsRead == 1){
						if (packetHeader.MagicNumber==PACKET_MAGIC_NUMBER){
							processPacketHeader(packetHeader);

							unsigned char * packet = (unsigned char*) malloc(packetHeader.NumBytesThisRecord-sizeof(XtfPacketHeader));

							elementsRead = fread (packet,packetHeader.NumBytesThisRecord-sizeof(XtfPacketHeader),1,file);

							if(elementsRead == 1){
								processPacket(packetHeader,packet);
							}
							else{
								printf("Error while reading packet\n");
							}

							free(packet);
						}
						else{
							printf("Invalid packet header\n");
						}
					}
					else{
						//TODO: whine and log error while reading
						//printf("Error while reading packet header\n");
					}
				}
			}
			else{
				fclose(file);
				throw "Invalid file format";
			}
		}
		else{
			fclose(file);
			throw "Couldn't read from file";
		}

		fclose(file);
	}
}

int XtfParser::getTotalNumberOfChannels(){
	return  this->fileHeader.NumberOfSonarChannels+
		this->fileHeader.NumberOfBathymetryChannels+
		this->fileHeader.NumberOfSnippetChannels+
		this->fileHeader.NumberOfEchoStrengthChannels+
		this->fileHeader.NumberOfInterferometryChannels;
};

void XtfParser::processFileHeader(XtfFileHeader & f){
    /*
        printf("------------\n");
        printf("FileFormat: %d\n",f.FileFormat);
        printf("SystemType: %d\n",f.SystemType);
        printf("RecordingProgramName: %s\n",f.RecordingProgramName);
        printf("RecordingProgramVersion: %s\n",f.RecordingProgramVersion);
        printf("SonarName: %s\n",f.SonarName);
        printf("sonarType: %d (%s)\n",f.SonarType,SonarTypes[f.SonarType].c_str());
        printf("NoteString: %s\n",f.NoteString);
        printf("ThisFileName: %s\n",f.ThisFileName);
        printf("NavUnits: %d\n",f.NavUnits);
        printf("NumberOfSonarChannels: %d\n",f.NumberOfSonarChannels);
        printf("NumberOfBathymetryChannels: %d\n",f.NumberOfBathymetryChannels);
        printf("NumberOfSnippetChannels: %d\n",f.NumberOfSnippetChannels);
        printf("NumberOfForwardLookArrays: %d\n",f.NumberOfForwardLookArrays);
        printf("NumberOfEchoStrengthChannels: %d\n",f.NumberOfEchoStrengthChannels);
        printf("NumberOfInterferometryChannels: %d\n",f.NumberOfInterferometryChannels);
        printf("Reserved1: %d\n",f.Reserved1);
        printf("Reserved2: %d\n",f.Reserved2);
        printf("ReferencePointHeight: %f\n",f.ReferencePointHeight);
        //TODO
        //printf("ProjectionType: ");
        //print(f.ProjectionType,12);
        //printf("\n");
        //printf("SpheriodType: ");
        //print(f.SpheriodType,10);
        //printf("\n");

        printf("NavigationLatency: %d\n",f.NavigationLatency);
        printf("OriginY: %f\n",f.OriginY);
        printf("OriginX: %f\n",f.OriginX);
        printf("NavOffsetY: %f\n",f.NavOffsetY);
        printf("NavOffsetX: %f\n",f.NavOffsetX);
        printf("NavOffsetZ: %f\n",f.NavOffsetZ);
        printf("NavOffsetYaw: %f\n",f.NavOffsetYaw);
        printf("MRUOffsetY: %f\n",f.MRUOffsetY);
        printf("MRUOffsetX: %f\n",f.MRUOffsetX);
        printf("MRUOffsetZ: %f\n",f.MRUOffsetZ);
        printf("MRUOffsetYaw: %f\n",f.MRUOffsetYaw);
        printf("MRUOffsetPitch: %f\n",f.MRUOffsetPitch);
        printf("MRUOffsetRoll: %f\n",f.MRUOffsetRoll);
        printf("------------\n");
        */
}

void XtfParser::processChanInfo(XtfChanInfo & c){
    /*
        printf("------------\n");
        printf("TypeOfChannel: %d\n",c.TypeOfChannel);
        printf("SubChannelNumber: %d\n",c.SubChannelNumber);
        printf("CorrectionFlags: %d\n",c.CorrectionFlags);
        printf("UniPolar: %d\n",c.UniPolar);
        printf("BytesPerSample: %d\n",c.BytesPerSample);
        printf("Reserved: %d\n",c.Reserved);
        printf("ChannelName: %s\n",c.ChannelName);
        printf("VoltScale: %f\n",c.VoltScale);
        printf("Frequency: %f\n",c.Frequency);
        printf("HorizBeamAngle: %f\n",c.HorizBeamAngle);
        printf("TiltAngle: %f\n",c.TiltAngle);
        printf("BeamWidth: %f\n",c.BeamWidth);
        printf("OffsetX: %f\n",c.OffsetX));
        printf("OffsetY: %f\n",c.OffsetY);
        printf("OffsetZ: %f\n",c.OffsetZ);
        printf("OffsetYaw: %f\n",c.OffsetYaw);
        printf("OffsetPitch: %f\n",c.OffsetPitch);
        printf("OffsetRoll: %f\n",c.OffsetRoll);
        printf("BeamsPerArray: %d\n",c.BeamsPerArray);
        printf("ReservedArea2: %s\n",c.ReservedArea2);
        printf("------------\n");
        */
}

void XtfParser::processPacketHeader(XtfPacketHeader & hdr){
    /*
        printf("------------\n");
        printf("MagicNumber: %d (%s)\n",hdr.MagicNumber,(hdr.MagicNumber==PACKET_MAGIC_NUMBER)?"OK":"FAIL");
        printf("HeaderType: %d\n",hdr.HeaderType);
        printf("SubChannelNumber: %d\n",hdr.SubChannelNumber);
        printf("NumChansToFollow: %d\n",hdr.NumChansToFollow);
        //printf("Reserved: %",x.Reserved);
        printf("NumBytesThisRecord: %d\n",hdr.NumBytesThisRecord);
        printf("------------\n");
*/
}

void XtfParser::processPing(uint64_t microEpoch,long id, double beamAngle,double tiltAngle,double twoWayTravelTime,uint32_t quality,uint32_t intensity){

    /*
	printf("------------\n");
	printf("PING\n");
	printf("ID: %u\n",ping.Id);
	printf("Intensity: %lf\n",ping.Intensity);
	printf("Quality: %u\n",ping.Quality);
	printf("TWTT: %lf\n",ping.TwoWayTravelTime);
	printf("DeltaTime: %lf\n",ping.DeltaTime);
	printf("BeamAngle: %lf\n",ping.BeamAngle);
	printf("TiltAngle: %lf\n",ping.TiltAngle);
	printf("------------\n");
    */
}

void XtfParser::processPingHeader(XtfPingHeader & hdr){
    processSwathStart(hdr.SoundVelocity);
}

void XtfParser::processSwathStart(double soundVelocity){

}


void XtfParser::processPacket(XtfPacketHeader & hdr,unsigned char * packet){

	if(hdr.HeaderType==XTF_HEADER_ATTITUDE){
		uint64_t microEpoch = 0;
		XtfAttitudeData* attitude = (XtfAttitudeData*)packet;

		if(attitude->SourceEpoch){
			microEpoch = attitude->SourceEpoch * 1000000 + attitude->EpochMicroseconds;
		}
		else{
			microEpoch = build_time(attitude->Year,attitude->Month,attitude->Day,attitude->Hour,attitude->Minutes,attitude->Seconds,attitude->Milliseconds,0);
		}

        	processAttitude(microEpoch,attitude->Heading,attitude->Pitch,attitude->Roll);
	}
	else if(hdr.HeaderType==XTF_HEADER_Q_MULTIBEAM){
		XtfPingHeader * pingHdr = (XtfPingHeader*) packet;

		processPingHeader(*pingHdr);

        //printf("%d Pings\n",hdr.NumChansToFollow);

		XtfQpsMbEntry * ping = (XtfQpsMbEntry*) ((uint8_t*)packet + sizeof(XtfPingHeader));

        uint64_t microEpoch = build_time(pingHdr->Year,pingHdr->Month,pingHdr->Day,pingHdr->Hour,pingHdr->Minute,pingHdr->Second,pingHdr->HSeconds * 10,0);

		for(unsigned int i = 0;i < hdr.NumChansToFollow;i++){
            processPing(microEpoch,ping[i].Id,ping[i].BeamAngle,ping[i].TiltAngle,ping[i].TwoWayTravelTime,ping[i].Quality,ping[i].Intensity);
		}
	}
	else if(hdr.HeaderType==XTF_HEADER_POSITION){
		XtfPosRawNavigation* position = (XtfPosRawNavigation*)packet;
        uint64_t microEpoch = build_time(position->Year,position->Month,position->Day,position->Hour,position->Minutes,position->Seconds,position->MicroSeconds/1000,position->MicroSeconds%1000);
        processPosition(microEpoch,position->RawXcoordinate,position->RawYcoordinate,position->RawAltitude);
	}
	else{
		printf("Unknown packet type: %d\n",hdr.HeaderType);
	}
}

void XtfParser::processAttitude(uint64_t microEpoch,float heading,float pitch,float roll){
	//printf("ATTITUDE: %lu %.04f %.04f %.04f\n",microEpoch,heading,pitch,roll);
}

void XtfParser::processPosition(uint64_t microEpoch,double longitude,double latitude,double height){
    //printf("POSITION: %lu %.08f %.08f\n",microEpoch,latitude,longitude,height);
}

#endif