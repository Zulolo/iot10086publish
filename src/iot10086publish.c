/*
 ============================================================================
 Name        : iot10086publish.c
 Author      : zulolo
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <curl/curl.h>

#define __USED_IOT10086PUBLISH_INTERNAL__
#include "nRF905_d.h"
#include "iot10086publish.h"

void sigINT_Handler(int32_t signum, siginfo_t *info, void *ptr)
{
	unNeedtoClose = NRF905_TRUE;
}

int32_t nSendDataToNRF905Socket(int32_t nConnectedSocketFD, nRF905CommTask_t* pNRF905CommTask, uint8_t* pACK_Payload)
{
	uint8_t unBuffer[sizeof(nRF905CommTask_t) + NRF905_TX_PAYLOAD_LEN];
	memcpy(unBuffer, pNRF905CommTask, sizeof(nRF905CommTask_t));
	memcpy(unBuffer + sizeof(nRF905CommTask_t), pACK_Payload, NRF905_TX_PAYLOAD_LEN);
	return send(nConnectedSocketFD, unBuffer, sizeof(nRF905CommTask_t) + NRF905_TX_PAYLOAD_LEN, 0);
}

/* Create a client endpoint32_t and connect to a server.   Returns fd if all OK, <0 on error. */
int32_t nTCPSocketConn(const char* pServerIPAddr, uint16_t unServerPort)
{
	int32_t nConnectSocket;
	struct sockaddr_in tConnSocketAddr;

    if ((nConnectSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    	NRF905D_LOG_ERR("Failed to create socket with error %d.", errno);
		return(-1);
    }

    /* Construct the server sockaddr_in structure */
    memset(&tConnSocketAddr, 0, sizeof(tConnSocketAddr));       /* Clear struct */
    tConnSocketAddr.sin_family = AF_INET;                  /* Internet/IP */
    tConnSocketAddr.sin_addr.s_addr = inet_addr(pServerIPAddr);  /* IP address */
    tConnSocketAddr.sin_port = unServerPort;       /* server port */
    /* Establish connection */
    if (connect(nConnectSocket, (struct sockaddr *) &tConnSocketAddr, sizeof(tConnSocketAddr)) < 0) {
    	close(nConnectSocket);
    	NRF905D_LOG_ERR("Failed to connect with server with error %d.", errno);
		return(-1);
    }

    return nConnectSocket;
}

int main(void) {
	puts("!!!Publish to 10086 cloud start!!!"); /* prints !!!Publish to 10086 cloud start!!! */
	CURL *pCurl;
	CURLcode tCurlRslt;
	struct curl_slist *pHttpHeaders;
	struct sigaction tSignalINT_Action;
	char cPostData[128];
	nRF905CommTask_t tNRF905CommTask;
	static uint8_t unACK_TX_Payload[NRF905_TX_PAYLOAD_LEN] = {RF_READ_SENSOR_VALUE, 0x00, };
	static uint8_t unACK_RX_Payload[NRF905_RX_PAYLOAD_LEN];
	int32_t nConnectedSocketFd;
	int32_t nReceivedCNT;
	SENSOR_DATA_T tSensorData;
//	uint32_t unIndex;

    tSignalINT_Action.sa_sigaction = sigINT_Handler;
    tSignalINT_Action.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &tSignalINT_Action, NULL);

	nConnectedSocketFd = nTCPSocketConn(LOCAL_HOST_IP, NRF905_SERVER_PORT);
	if(nConnectedSocketFd < 0)
	{
		printf("Error[%d] when connecting...", errno);
		return (-1);
	}

	tNRF905CommTask.tCommType = NRF905_COMM_TYPE_TX_PKG;
	tNRF905CommTask.unCommByteNum = NRF905_TX_PAYLOAD_LEN;

	/* get a curl handle */
	pCurl = curl_easy_init();
	if (NULL == pCurl){
		NRF905D_LOG_ERR("Initialize curl error.");
		close(nConnectedSocketFd);
		return (-1);
	}

	/* First set the URL that is about to receive our POST. This URL can
	just as well be a https:// URL if that is what should receive the
	data. */
	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1L);	// Show debug info
	curl_easy_setopt(pCurl, CURLOPT_URL, DEVICE_API_URL);
	pHttpHeaders = NULL;
	pHttpHeaders = curl_slist_append(pHttpHeaders, "Content-Type: application/json");
	pHttpHeaders = curl_slist_append(pHttpHeaders, "X-Auth-Token:ENc7C7vHZPBcSEj3L4HZ9oZd7ENGAd");
	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHttpHeaders);

	while (NRF905_FALSE == unNeedtoClose){
		if (nSendDataToNRF905Socket(nConnectedSocketFd, &tNRF905CommTask, unACK_TX_Payload) < 0 ){
			printf("Write task communication payload to pipe error with code:%d.", errno);
			NRF905D_LOG_ERR("Write task communication payload to pipe error with code:%d.", errno);
		} else {
			if ((nReceivedCNT = recv(nConnectedSocketFd, unACK_RX_Payload, NRF905_RX_PAYLOAD_LEN, 0)) < 0) {
				printf("Receive from unix domain socket error with code:%d.", errno);
				NRF905D_LOG_ERR("Receive from unix domain socket error with code:%d.", errno);
			} else {
//				printf("Socket %d received one message from nRF905 server.\n", nConnectedSocketFd);
//				for (unIndex = 0; unIndex < ARRAY_SIZE(unACK_RX_Payload); unIndex++){
//					printf("0x%02X\n", unACK_RX_Payload[unIndex]);
//				}
				memcpy(&tSensorData, unACK_RX_Payload + 1, sizeof(tSensorData));

				sprintf(cPostData, "{\"temperature\":%.01f,\"humidity\":%.01f,\"air-quality\":%u}",
						(double)(tSensorData.nTemperature) / 10,
						(double)(tSensorData.unHumidity) / 10,
						tSensorData.unAirQuality);
				puts(cPostData);

				/* Now specify the POST data */
				curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, cPostData);

				/* Perform the request, res will get the return code */
				tCurlRslt = curl_easy_perform(pCurl);
				/* Check for errors */
				if(tCurlRslt != CURLE_OK){
					fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(tCurlRslt));
				}
			}
		}

		sleep(5);
	}
	/* cleanup */
	curl_slist_free_all(pHttpHeaders);
	curl_easy_cleanup(pCurl);

	close(nConnectedSocketFd);
	return EXIT_SUCCESS;
}
