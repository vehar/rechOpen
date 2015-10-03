#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>

#include "rp.h"

#define BUFLEN 16384  //Max length of buffer // 16*1024
#define PORT 8888   //The port on which to listen for incoming data

    int tbuff = 0;
    int CheckUDP = 0;

// preparing the UDP connection

//
// ASUS : 192.168.1.47
// Port : 10000
//

void die(char *s)
{
    perror(s);
    exit(1);
}


// Pitaya code in itself
int main(int argc, char **argv){

	/* Initialize UDP */
	struct sockaddr_in si_me, si_other;
	int s, slen = sizeof(si_other);
	//char buf[BUFLEN];
	//create a UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
	die("socket");
	}
	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	//bind socket to port
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1){
		die("bind");
	}
	/* End of UDP initialization */

	/*number of acquision*/
	uint32_t N = 64;
	/*size of the acquisition buffer*/
	uint32_t buff_size = 16384;
	/*allocation of buffer size in memory*/
	float *buff = (float *)malloc(buff_size * sizeof(float));

	/*initialise to 0 the buffer*/
	for (int i=0 ; i<buff_size ; i++){buff[i]=0.0;}

	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}

	/* decimation n (=1,8,64...) : frequency = 125/n MHz*/
	rp_AcqSetDecimation(RP_DEC_1);

	/*init trigger state*/
	rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

	for (int i=0 ; i<N ; i++) {

		/*start acquisition must be set before trigger initiation*/
		rp_AcqStart();

		/*allocation of temporary buffer size in memory*/
		float *temp = (float *)malloc(buff_size * sizeof(float));

		/*trigger source*/
		rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHB_NE);

		/*level of the trigger activation in volt*/
		rp_AcqSetTriggerLevel(-0.03); 

		/*acquisition trigger delay*/
		rp_AcqSetTriggerDelay(8000);

		/*waiting for trigger*/
		while(1){
			rp_AcqGetTriggerState(&state);
			if(state == RP_TRIG_STATE_TRIGGERED){
				break;
			}
		}		

		/*putt acquisition data in the temporary buffer*/
		rp_AcqGetOldestDataV(RP_CH_2, &buff_size, temp);

		/*additionning the N signals*/
		for (int j = 0; j < buff_size; j++){
			buff[j]+=temp[j];
		}

		/*release memory*/
		free(temp);
	}

	/*open file and write the mean signal*/
	FILE * fm;
 	fm = fopen ("moy.txt", "w+");
	for (int i=0 ; i<buff_size ; i++) {fprintf(fm, "%f\n", buff[i]/N);}
	fclose(fm);



	/* sending to UDP */
	for (int i=0 ; i<buff_size ; i++) {
		char tbuff = buff[i]/N;
		CheckUDP = sendto(s,tbuff,sizeof(tbuff),0,(struct sockaddr*) &si_other,slen);
		if (CheckUDP==-1) {
         	   die("sendto()");
       		}
	}


	/* end of emission */

	/* Release rp resources */
	rp_Release();

	return 0;
}
