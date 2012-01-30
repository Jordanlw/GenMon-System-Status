#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	//Find network usage
	int fd = open("/proc/net/dev",O_RDONLY);
	if(fd < 0) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	char buffer[5000];
	int exitStatus = read(fd,buffer,sizeof(buffer) - 1);
	close(fd);
	if(exitStatus <= 0) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	buffer[exitStatus] = '\0';
	char *found = strstr(buffer,"eth0:");
	long rData = 0;
	long tData = 0;
	int i;
	for(i = 0;i < 9;i++)
	{
		if(!i)
		{
			found = strtok(found," ");
			found = strtok(NULL," ");
			rData = atol(found);
			continue;
		}
		found = strtok(NULL," ");
	}
	tData = atol(found);

	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC,&tp);
	fd = open("/dev/shm/jSysStat.txt.tmp",O_RDWR | O_CREAT,0700);
	if(fd == -1) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	long prevRData,prevTData,tDelta,rDelta;
	//Find previous network data
	exitStatus = read(fd,buffer,sizeof(buffer) - 1);
	if(exitStatus)
	{
		buffer[exitStatus] = '\0';
		strtok(buffer," ");
		prevRData = atol(strtok(NULL," "));
		prevTData = atol(strtok(NULL," "));
		rDelta = rData - prevRData;
		tDelta = tData - prevTData;
	}
	//Write new network data to file
	sprintf(buffer,"NET: %ld %ld ",rData,tData);
	lseek(fd,0,SEEK_SET);
	if(write(fd,buffer,strlen(buffer)) < 0) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	//Set prefixes for stdout
	char rSize[100],tSize[100];
	if(rDelta < 1000) strcpy(rSize,"Bps");
	if(tDelta < 1000) strcpy(tSize,"Bps");
	if(rDelta >= 1000) rDelta /= 1000,strcpy(rSize,"KBps");
	if(tDelta >= 1000) tDelta /= 1000,strcpy(tSize,"KBps");
	if(rDelta >= 1000*1000) rDelta /= (1000*1000),strcpy(rSize,"MBps");
	if(tDelta >= 1000*1000) tDelta /= (1000*1000),strcpy(tSize,"MBps");
	//Find position to write CPU data to within fd
	lseek(fd,0,SEEK_SET);
	exitStatus = read(fd,buffer,sizeof(buffer) - 1);
	buffer[exitStatus] = '\0';
	found = strstr(buffer,"CPU:");
	/*
	Set cpuPos to end of buffer, also if found is set (setting done if "CPU:" is found within buffer)
	then set cpuPos to position of "CPU:"
	*/
	int cpuPos = strlen(buffer);
	if(found)
	{
		*found = '\0';
		cpuPos = strlen(buffer);
	}
	//Find combined CPU usage
	//Grab data from /proc/stat
	int statFd = open("/proc/stat",O_RDONLY);
	if(statFd == -1) puts("EXIT_FAILURE"), exit(EXIT_FAILURE);
	exitStatus = read(statFd,buffer,sizeof(buffer) - 1);
	buffer[exitStatus] = '\0';
	found = strtok(buffer," ");
	long minusIdle = 0,total = 0;
	for(i = 0;i < 4;i++)
	{
		found = strtok(NULL," ");
		if(i < 3) minusIdle += atol(found);
	}
	total += atol(found);
	total += minusIdle;
	if(close(statFd)) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	//Grab previous data from tmp file
	lseek(fd,0,SEEK_SET);
	long prevMinus = 0,prevTotal = 0;
	exitStatus = read(fd,buffer,sizeof(buffer) -1);
	if(exitStatus)
	{
		buffer[exitStatus] = '\0';
		found = strstr(buffer,"CPU:");
		if(found)
		{
			strtok(found," ");
			prevMinus = atol(strtok(NULL," "));
			prevTotal = atol(strtok(NULL," "));
		}
	}
	//Store current data as previous
	lseek(fd,cpuPos,SEEK_SET);
	int size;
	size = sprintf(buffer,"CPU: %ld %ld ",minusIdle,total);
	if(write(fd,buffer,size) < 0) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	if(close(fd)) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	minusIdle -= prevMinus;
	total -= prevTotal;
	//Find memory usage
	fd = open("/proc/meminfo",O_RDONLY);
	if(fd < 0) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	exitStatus = read(fd,buffer,sizeof(buffer) - 1);
	if(exitStatus < 0) puts("EXIT FAILURE"), exit(EXIT_FAILURE);
	buffer[exitStatus] = '\0';
	found = strstr(buffer,"MemTotal:");
	strtok(found," ");
	long memTotal = atol(strtok(NULL," "));
	found = strstr(buffer,"MemFree:");
	strtok(found," ");
	long memFree = atol(strtok(NULL," "));
	found = strstr(buffer,"Buffers:");
	strtok(found," ");
	long bufferMem = atol(strtok(NULL," "));
	found = strstr(buffer,"Cached:");
	strtok(found," ");
	long cached = atol(strtok(NULL," "));
	memFree += cached + bufferMem;
	
	printf("DOWN:%ld %s UP:%ld %s \nCPU:%ld%% MEM: %ld%%",rDelta,rSize,tDelta,tSize,
	(minusIdle * 100) / total,100 - (memFree * 100) / memTotal);
	return 0;
}
	
