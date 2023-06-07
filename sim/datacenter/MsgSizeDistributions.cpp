/*
 * MsgSizeDistributions.cc
 *
 *  Created on: May 22, 2015
 *      Author: neverhood
 */

#include <iostream>
#include <fstream>
//#include <math.h>
#include <stdio.h>
#include <random>
#include <string.h>
#include "MsgSizeDistributions.h"
//#include "config.h"
MsgSizeDistributions::MsgSizeDistributions(char *workLoadType, double loadfactor)
	: msgSizeProbDistVector()
	, msgSizeDestInterarrivalQueue()
	, avgMsgSize(0.0)
	, avgInterArrivalTime(0.0)
{
// Initialize the msgSizeGenerator
    //const char* workLoadType = "DCTCP";
	MsgSizeDistributions::DistributionChoice distSelector;
    std::string distFileName;
    if (strcmp(workLoadType, "DCTCP") == 0) {
        distSelector = MsgSizeDistributions::DistributionChoice::DCTCP;
        distFileName = std::string(
                "../../../../sizeDistributions/DCTCP_MsgSizeDist.txt");
    } else if (strcmp(workLoadType, "TEST_DIST") == 0) {
        distSelector = MsgSizeDistributions::DistributionChoice::TEST_DIST;
        distFileName = std::string(
              	"../../../sizeDistributions/TestDistribution.txt");
    } else if (strcmp(workLoadType, "FACEBOOK_KEY_VALUE") == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::FACEBOOK_KEY_VALUE;
        distFileName = std::string(
                "../../../sizeDistributions/FacebookKeyValueMsgSizeDist.txt");
    } else if (strcmp(workLoadType, "FACEBOOK_WEB_SERVER_INTRACLUSTER")
            == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::
                FACEBOOK_WEB_SERVER_INTRACLUSTER;
        distFileName = std::string("../../../sizeDistributions/"
                "Facebook_WebServerDist_IntraCluster.txt");
    } else if (strcmp(workLoadType, "FACEBOOK_CACHE_FOLLOWER_INTRACLUSTER")
            == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::
                FACEBOOK_CACHE_FOLLOWER_INTRACLUSTER;
        distFileName = std::string("../../../sizeDistributions/"
                "Facebook_CacheFollowerDist_IntraCluster.txt");
    } else if (strcmp(workLoadType, "GOOGLE_ALL_RPC")
            == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::
                GOOGLE_ALL_RPC;
        distFileName = std::string("../../../sizeDistributions/"
                "Google_AllRPC.txt");
    } else if (strcmp(workLoadType, "FACEBOOK_HADOOP_ALL")
            == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::
                FACEBOOK_HADOOP_ALL;
        distFileName = std::string("../../../sizeDistributions/"
                "Facebook_HadoopDist_All.txt");
    } else if (strcmp(workLoadType, "FABRICATED_HEAVY_MIDDLE")
            == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::
                FABRICATED_HEAVY_MIDDLE;
        distFileName = std::string("../../../sizeDistributions/"
                "Fabricated_Heavy_Middle.txt");
    } else if (strcmp(workLoadType, "GOOGLE_SEARCH_RPC")
            == 0) {
        distSelector =
            MsgSizeDistributions::DistributionChoice::GOOGLE_SEARCH_RPC;
        distFileName = std::string("../../../sizeDistributions/"
                "Google_SearchRPC.txt");
    } else if (strcmp(workLoadType, "FABRICATED_HEAVY_HEAD")
            == 0) {
        distSelector =
                MsgSizeDistributions::DistributionChoice::
                FABRICATED_HEAVY_HEAD;
        distFileName = std::string("../../../sizeDistributions/"
                "Fabricated_Heavy_Head.txt");
    } else if (strcmp(workLoadType, "PRESET_IN_FILE") == 0){
        distSelector =
                MsgSizeDistributions::DistributionChoice::SIZE_IN_FILE;
        distFileName = std::string(
                "../../../sizeDistributions/HostidSizeInterarrival.txt");
    } else {
        fprintf(stderr, "'%s': Not a valie workload type.",workLoadType);
	exit(-1);    
}

    maxDataBytesPerPkt = 1500; 
	cout<< "distFileName="<<distFileName <<endl;
    MsgSizeDistributions::InterArrivalDist interArrivalDist;
        interArrivalDist = MsgSizeDistributions::InterArrivalDist::EXPONENTIAL;
/*    if (strcmp(par("interArrivalDist").stringValue(), "exponential") == 0) {
        interArrivalDist = MsgSizeDistributions::InterArrivalDist::EXPONENTIAL;
    } else if (strcmp(par("interArrivalDist").stringValue(),
            "preset_in_file") == 0) {
        interArrivalDist =
                MsgSizeDistributions::InterArrivalDist::INTERARRIVAL_IN_FILE;
    } else if (strcmp(par("interArrivalDist").stringValue(),
            "facebook_key_value") == 0){
        interArrivalDist =
                MsgSizeDistributions::InterArrivalDist::FACEBOOK_PARETO;
    } else {
        throw cRuntimeError("'%s': Not a valid Interarrival Distribution",
                par("interArrivalDist").stringValue());
    }*/


	//uint32_t nicLinkSpeed = 10;		
    double avgRate = loadfactor * speedFromMbps((uint64_t)HOST_NIC);
   	init(distFileName.c_str(),
            maxDataBytesPerPkt, interArrivalDist, distSelector, avgRate,
            0);
 
}


/**
 * avgRate must be in Gb/s defined as loadFactor*nicLinkSpeed
 */

void MsgSizeDistributions::init(const char* distFileName,
        int maxDataBytesPerPkt, InterArrivalDist interArrivalDist,
        DistributionChoice sizeDistSelector, double avgRate, int callerHostId)
{
	this->sizeDistSelector = sizeDistSelector;
	this->interArrivalDist = interArrivalDist;
	this->maxDataBytesPerPkt = maxDataBytesPerPkt;   
	
	std::ifstream distFileStream;
    distFileStream.open(distFileName);
    if (distFileStream.fail()) {
        char buf[100];
        snprintf(buf, 100, "%s: Failed to open file.", distFileName);
        throw MsgSizeDistException(buf);
        distFileStream.close();
    }

    if (sizeDistSelector >= DistributionChoice::NO_SIZE_DIST_SPECIFIED ||
            sizeDistSelector < 0) {
        throw MsgSizeDistException("Invalide MsgSize Distribution Selected.");

    }

    if (interArrivalDist >= InterArrivalDist::NO_INTERARRIAVAL_DIST_SPECIFIED ||
            interArrivalDist < 0) {
        throw MsgSizeDistException("Invalid MsgArrival Distribution Selected.");
    }

    if (sizeDistSelector == DistributionChoice::SIZE_IN_FILE){
        assert(interArrivalDist == InterArrivalDist::INTERARRIVAL_IN_FILE);
        double dt = 0.0;
        std::string hostIdSizeInterarrivalLine;
        int srcHostId;
        int destHostId;
        int msgSize;
        double deltaTime;
        while (getline(distFileStream, hostIdSizeInterarrivalLine)) {
            sscanf(hostIdSizeInterarrivalLine.c_str(), "%d %d %d %lf",
                    &srcHostId, &destHostId, &msgSize, &deltaTime);
            dt += deltaTime;
            if (srcHostId == callerHostId) {
                msgSizeDestInterarrivalQueue.push(
                    std::make_tuple(msgSize, destHostId, dt));
                dt = 0.0;
            }
        }

    } else {
        //ASSERT(interArrivalDist != InterArrivalDist::INTERARRIVAL_IN_FILE);
        std::string avgMsgSizeStr;
        std::string sizeProbStr;

        // The first line of distFileName is the average message size of the
        // distribution.
        getline(distFileStream, avgMsgSizeStr);
        sscanf(avgMsgSizeStr.c_str(), "%lf", &avgMsgSize);
        if (sizeDistSelector == DistributionChoice::DCTCP) {
            avgMsgSize *= maxDataBytesPerPkt; // AvgSize in terms of bytes
        	cout<<"sizeDistSelector=DCTCP"<<endl;
		}

        avgInterArrivalTime = 1.0* avgMsgSize * 8  / avgRate; //bps
		cout<< "avgInterArrivalTime="<< avgInterArrivalTime<<" avgRate="<<avgRate<<endl;	
        // reads msgSize<->probabilty pairs from "distFileName" file
        while(getline(distFileStream, sizeProbStr)) {
            int msgSize;
            double prob;
            sscanf(sizeProbStr.c_str(), "%d %lf",
                    &msgSize, &prob);
            msgSizeProbDistVector.push_back(std::make_pair(msgSize, prob));
        }
    }
    distFileStream.close();
}
void
MsgSizeDistributions::getSizeAndInterarrival(int &msgSize, int &destHostId,
        double &nextInterarrivalTime)
{
        // reads msgSize<->probabilty pairs from "distFileName" file
	// Default value -1 if not destHostId is determined in the files. In which
    // case, the destination will be chosen based on the config.xml file.
    destHostId = -1;
    switch(sizeDistSelector) {
        case DistributionChoice::DCTCP:
        case DistributionChoice::GOOGLE_ALL_RPC:
        case DistributionChoice::GOOGLE_SEARCH_RPC:
        case DistributionChoice::FACEBOOK_WEB_SERVER_INTRACLUSTER:
        case DistributionChoice::FACEBOOK_CACHE_FOLLOWER_INTRACLUSTER:
        case DistributionChoice::FACEBOOK_HADOOP_ALL:
        case DistributionChoice::FABRICATED_HEAVY_MIDDLE:
        case DistributionChoice::FABRICATED_HEAVY_HEAD:
        case DistributionChoice::TEST_DIST:
			getInterarrivalSizeFromVec(msgSize, nextInterarrivalTime);
            return;
        case DistributionChoice::FACEBOOK_KEY_VALUE:
            getFacebookSizeInterarrival(msgSize, nextInterarrivalTime);
            return;
        case DistributionChoice::SIZE_IN_FILE:
            getInfileSizeInterarrivalDest(msgSize, destHostId, 
                nextInterarrivalTime);
            return;
        default:
            msgSize = -1;
            nextInterarrivalTime = 0.0;
            return;
    }
}

void
MsgSizeDistributions::getInfileSizeInterarrivalDest(int &msgSize,
    int &destHostId, double &nextInterarrivalTime)
{
    //ASSERT(interArrivalDist == InterArrivalDist::INTERARRIVAL_IN_FILE);
    if (msgSizeDestInterarrivalQueue.empty()) {
        msgSize = -1;
        nextInterarrivalTime = 0.0;
        return;
    }

    auto msgTuple = msgSizeDestInterarrivalQueue.front();
    msgSize = std::get<0>(msgTuple);
    destHostId = std::get<1>(msgTuple);
    nextInterarrivalTime = std::get<2>(msgTuple);
    msgSizeDestInterarrivalQueue.pop();
    return;
}

/*
 * Internal method for DCTCP, TestDist, FACEBOOK_CACHE_FOLLOWER_INTRACLUSTER,
 * FACEBOOK_WEB_SERVER_INTRACLUSTER, FACEBOOK_HADOOP_ALL,
 * FABRICATED_HEAVY_MIDDLE, and FABRICATED_HEAVY_HEAD
 */
void
MsgSizeDistributions::getInterarrivalSizeFromVec(int &msgSize,
        double &nextInterarrivalTime)
{

    double prob = drand(); //uniform(0.0, 1.0);
    size_t mid, high, low;
    high = msgSizeProbDistVector.size() - 1;
    low = 0;
    while(low < high) {
        mid = (high + low) / 2;
        if (prob <= msgSizeProbDistVector[mid].second) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    msgSize = msgSizeProbDistVector[high].first;

    if (sizeDistSelector == DistributionChoice::DCTCP){
        msgSize *= maxDataBytesPerPkt;
    }

    //generate the interarrival time
    nextInterarrivalTime = getInterarrivalTime();
//	cout<<"msgSize=" <<msgSize <<" nextInterarrivalTime="<<nextInterarrivalTime<<endl;
	return;

}


void
MsgSizeDistributions::getFacebookSizeInterarrival(int &msgSize,
        double &nextInterarrivalTime)
{
    // Facebook KeyValue workload constants
    int sizeOffset = msgSizeProbDistVector.back().first;
    double probOffset = msgSizeProbDistVector.back().second;

    // Generalized pareto distribution parameters
    const double k = 0.348238;
    const double sigma = 214.476;

    // Maximum allowed Msg size
    const int maxSize = 0x40000000;

    double prob = drand();//uniform(0.0, 1.0);
    int size;

    if (prob <= msgSizeProbDistVector.back().second) {
        int first = 0;
        int last = msgSizeProbDistVector.size() - 1;

        while (first < last) {
            int mid = first + (last - first)/2;
            if (prob <= msgSizeProbDistVector[mid].second) {
                last = mid;
            } else {
                first = mid + 1;
            }
        }
        size = msgSizeProbDistVector[first].first;

    } else {
        double msgSizeTemp =
            round( sizeOffset +
            (pow((1-probOffset)/(1-prob), k) - 1) * sigma / k );

        size =  msgSizeTemp > maxSize ? maxSize : (int)(msgSizeTemp);
    }
    msgSize = size;

    //generate the interarrival gap
    nextInterarrivalTime = getInterarrivalTime();
    return;
}

double
MsgSizeDistributions::facebookParetoInterGap()
{
    // Generalized pareto distribution parameters
    const double k = 0.154971;
    const double sigma = 16.0292e-6;

    // Maximum interarrival gap
    const int maxGap = 1000;
    double prob = drand();//uniform(0.0, 1.0);

    double gapTime = (pow(1/(1-prob), k) - 1) * sigma / k;

    // Capping the gap value at the maxGap
    if (gapTime > maxGap) {
        gapTime = maxGap;
    }
    return gapTime;
}

double
MsgSizeDistributions::getInterarrivalTime()
{
    if (interArrivalDist == InterArrivalDist::EXPONENTIAL) {
        return exponential(1.0/avgInterArrivalTime);   //seconds
	} else if (interArrivalDist == InterArrivalDist::FACEBOOK_PARETO) {
        return facebookParetoInterGap();
    } else {
        throw MsgSizeDistException("InterArrival generator not defined.");
    }
}
