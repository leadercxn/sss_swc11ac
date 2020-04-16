/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: info.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

#include <string.h>

#include "../nmea/info.h"

void nmea_zero_INFO(nmeaINFO *info)
{
    memset(info, 0, sizeof(nmeaINFO));
    nmea_time_now(&info->utc);
    info->sig = NMEA_SIG_BAD;
    info->fix = NMEA_FIX_BAD;
}

void nmea_get_gps_coord_in_deg(nmeaINFO *info,double *buf)
{
		double degree,minute;

		degree = (int)(info->lat/100);
		//printf("deg 1 step val %f\r\n",degree);
		minute = info->lat-degree*100;
		//printf("minute 1 step val %f\r\n",minute);
		buf[0] = degree+minute/60;
		//printf("latitude val = %f\r\n",lat);
		degree = (int)(info->lon/100);
		//printf("deg 2 step val %f\r\n",degree);
		minute = info->lon - degree*100;
		//printf("minute 2 step val %f\r\n",minute);
		buf[1] = degree+minute/60;
		//printf("longitude val = %f\r\n",lon);
}
