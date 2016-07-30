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
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <signal.h>
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

int32_t nUnixSocketConn(const char *pNRF905ServerName)
{
	int32_t nConnectSocketFd;
	int32_t nLen;
	struct sockaddr_un tSocketAddrUn;

	if ((nConnectSocketFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){     /* create a UNIX domain stream socket */
		return(-1);
	}
	memset(&tSocketAddrUn, 0, sizeof(tSocketAddrUn));            /* fill socket address structure with our address */
	tSocketAddrUn.sun_family = AF_UNIX;
	sprintf(tSocketAddrUn.sun_path, "scktmp%05d", getpid());
	nLen = offsetof(struct sockaddr_un, sun_path) + strlen(tSocketAddrUn.sun_path);
	unlink(tSocketAddrUn.sun_path);               /* in case it already exists */
	if (bind(nConnectSocketFd, (struct sockaddr *)&tSocketAddrUn, nLen) < 0) {
		close(nConnectSocketFd);
		NRF905D_LOG_ERR("Bind socket failed during client connect with error %d.", errno);
		return (-1);
	}else{
		/* fill socket address structure with server's address */
		memset(&tSocketAddrUn, 0, sizeof(tSocketAddrUn));
		tSocketAddrUn.sun_family = AF_UNIX;
		strcpy(tSocketAddrUn.sun_path, pNRF905ServerName);
		nLen = offsetof(struct sockaddr_un, sun_path) + strlen(pNRF905ServerName);
		if (connect(nConnectSocketFd, (struct sockaddr *)&tSocketAddrUn, nLen) < 0)   {
			close(nConnectSocketFd);
			NRF905D_LOG_ERR("Connect server failed with error %d.", errno);
			return (-1);
		}else{
			return (nConnectSocketFd);
		}
	}
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

    tSignalINT_Action.sa_sigaction = sigINT_Handler;
    tSignalINT_Action.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &tSignalINT_Action, NULL);

	nConnectedSocketFd = nUnixSocketConn(NRF905_SERVER_NAME);
	if(nConnectedSocketFd < 0)
	{
		printf("Error[%d] when connecting...", errno);
		return (-1);
	}

	tNRF905CommTask.tCommType = NRF905_COMM_TYPE_TX_PKG;
	tNRF905CommTask.unCommByteNum = NRF905_TX_PAYLOAD_LEN;


	while (NRF905_FALSE == unNeedtoClose){
		if (nSendDataToNRF905Socket(nConnectedSocketFd, &tNRF905CommTask, unACK_TX_Payload) < 0 ){
			printf("Write task communication payload to pipe error with code:%d.", errno);
			NRF905D_LOG_ERR("Write task communication payload to pipe error with code:%d.", errno);
		} else {
			if ((nReceivedCNT = recv(nConnectedSocketFd, unACK_RX_Payload, NRF905_RX_PAYLOAD_LEN, 0)) < 0) {
				printf("Receive from unix domain socket error with code:%d.", errno);
				NRF905D_LOG_ERR("Receive from unix domain socket error with code:%d.", errno);
			} else {
				memcpy(&tSensorData, unACK_RX_Payload + 1, sizeof(tSensorData));
				/* get a curl handle */
				pCurl = curl_easy_init();
				if(pCurl) {
					/* First set the URL that is about to receive our POST. This URL can
					just as well be a https:// URL if that is what should receive the
					data. */
					curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1L);	// Show debug info
					curl_easy_setopt(pCurl, CURLOPT_URL, DEVICE_API_URL);
					pHttpHeaders = NULL;
					pHttpHeaders = curl_slist_append(pHttpHeaders, "Content-Type: application/json");
					pHttpHeaders = curl_slist_append(pHttpHeaders, "api-key:PPsmfd=F0gDlwdZLgGsrVEAsIeY=");
					curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHttpHeaders);

					sprintf(cPostData, "{\"Temperature\":%.01f,\"Humidity\":%.01f,\"AirQuality\":%u}",
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
					/* always cleanup */
					curl_easy_cleanup(pCurl);
				}
			}
		}

		sleep(2);
	}

	close(nConnectedSocketFd);
	return EXIT_SUCCESS;
}
