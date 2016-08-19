/*
 * iot10086publish.h
 *
 *  Created on: Jul 27, 2016
 *      Author: zulolo
 */

#ifndef IOT10086PUBLISH_H_
#define IOT10086PUBLISH_H_

#ifdef __USED_IOT10086PUBLISH_INTERNAL__

#define __IOT10086PUBLISH_EXTERN__
typedef struct
{
	struct
	{
		uint16_t unNULL:16;
	}SCR;												// sensor control bits
	struct
	{
		uint16_t unNULL:16;
	}SSR;												// sensor status bits
	uint16_t	unHumidity;				// Need to divided by 10 to get real one
	int16_t		nTemperature;			// Need to divided by 10 to get real one
	uint16_t	unAirQuality;			//
} SENSOR_DATA_T;

#define DEVICE_API_URL				"http://things.ubidots.com/api/v1.6/devices/smart_home_xm/"
static uint8_t unNeedtoClose = NRF905_FALSE;
#else
#define __IOT10086PUBLISH_EXTERN__			extern
#endif

#endif /* IOT10086PUBLISH_H_ */
