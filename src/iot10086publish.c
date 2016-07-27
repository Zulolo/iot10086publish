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
#include <curl/curl.h>

#define __USED_IOT10086PUBLISH_INTERNAL__
#include "iot10086publish.h"

int main(void) {
	puts("!!!Publish to 10086 cloud start!!!"); /* prints !!!Publish to 10086 cloud start!!! */
	CURL *pCurl;
	CURLcode tCurlRslt;
	struct curl_slist *pHttpHeaders;
	int32_t nIndex;
	char cPostData[128];

	for (nIndex = 0; nIndex < 100; nIndex++){
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

			sprintf(cPostData, "{\"Current\":%.02f}", ((double)(rand()%10000))/100);
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
		sleep(2);
	}
	return EXIT_SUCCESS;
}
