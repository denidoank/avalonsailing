/*
 *  Decoder for aiscat.
 *
 *  Heavily cannibalized from gpsd's gps.h
 *  Original copyright notice follows.  the gpsd project rocks.
 *  We'd use that if it didn't mean as much work to convert the json
 *  output to our internal formats.
 */

/* gps.h -- interface of the libgps library */
/*
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */
/* For completenes, that COPYING in the distribution GPSD root notice says:

			COPYRIGHTS

Compilation copyright is held by the GPSD project.  All rights reserved.

GPSD project copyrights are assigned to the project lead, currently
Eric S. Raymond. Other portions of the GPSD code are Copyright (c)
1997, 1998, 1999, 2000, 2001, 2002 by Remco Treffkorn, and others
Copyright (c) 2005 by Eric S. Raymond.  For other copyrights, see
individual files.

			BSD LICENSE

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:<P>

Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.<P>

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.<P>

Neither name of the GPSD project nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef _IO_AIS_H_
#define _IO_AIS_H_

#include <stdint.h>

#define NMEA_MAX	91		/* max length of NMEA sentence */

/* 
 * Is an MMSI number that of an auxiliary associated with a mother ship?
 * We need to be able to test this for decoding AIS Type 24 messages.
 * According to <http://www.navcen.uscg.gov/marcomms/gmdss/mmsi.htm#format>,
 * auxiliary-craft MMSIs have the form 98MIDXXXX, where MID is a country 
 * code and XXXX the vessel ID.
 */
#define AIS_AUXILIARY_MMSI(n)	((n) / 10000000 == 98)

/* N/A values and scaling constant for 25/24 bit lon/lat pairs */
#define AIS_LON3_NOT_AVAILABLE	181000
#define AIS_LAT3_NOT_AVAILABLE	91000
#define AIS_LATLON3_SCALE	60000.0

/* N/A values and scaling constant for 28/27 bit lon/lat pairs */
#define AIS_LON4_NOT_AVAILABLE	1810000
#define AIS_LAT4_NOT_AVAILABLE	910000
#define AIS_LATLON4_SCALE	600000.0

#define AIVDM_CHANNELS	2		/* A, B */
#define AIS_SHIPNAME_MAXLEN	20

/* hold context for decoding AIDVM packet sequences */
struct aivdm_context_t {
    int decoded_frags;          /* for tracking AIDVM parts in a multipart sequence */
    unsigned char bits[2048];
    size_t bitlen; /* how many valid bits */
    unsigned int mmsi24; /* type 24 specific */
    char shipname24[AIS_SHIPNAME_MAXLEN+1]; /* type 24 specific */
};

typedef uint8_t bool;

#define AIS_TYPE26_BINARY_MAX	1004	/* Up to 128 bits */
#define AIS_LONGRANGE_LATLON_SCALE	600.0
#define AIS_LONGRANGE_LON_NOT_AVAILABLE	0x1a838
#define AIS_LONGRANGE_LAT_NOT_AVAILABLE	0xd548
#define AIS_LONGRANGE_SPEED_NOT_AVAILABLE 63
#define AIS_LONGRANGE_COURSE_NOT_AVAILABLE 511
#define AIS_TURN_HARD_LEFT	-127
#define AIS_TURN_HARD_RIGHT	127
#define AIS_TURN_NOT_AVAILABLE	-128
#define AIS_SPEED_NOT_AVAILABLE	1023
#define AIS_SPEED_FAST_MOVER	1022		/* >= 102.2 knots */
#define AIS_LATLON_SCALE	600000.0
#define AIS_LON_NOT_AVAILABLE	0x6791AC0
#define AIS_LAT_NOT_AVAILABLE	0x3412140
#define AIS_COURSE_NOT_AVAILABLE	3600
#define AIS_HEADING_NOT_AVAILABLE	511
#define AIS_SEC_NOT_AVAILABLE	60
#define AIS_SEC_MANUAL		61
#define AIS_SEC_ESTIMATED	62
#define AIS_SEC_INOPERATIVE	63
#define AIS_YEAR_NOT_AVAILABLE	0
#define AIS_MONTH_NOT_AVAILABLE	0
#define AIS_DAY_NOT_AVAILABLE	0
#define AIS_HOUR_NOT_AVAILABLE	24
#define AIS_MINUTE_NOT_AVAILABLE	60
#define AIS_SECOND_NOT_AVAILABLE	60
#define AIS_SHIPNAME_MAXLEN	20
#define AIS_TYPE6_BINARY_MAX	920	/* 920 bits */
#define AIS_DAC1FID30_TEXT_MAX	154	/* 920 bits of six-bit, plus NUL */
#define DAC1FID32_CDIR_NOT_AVAILABLE		360
#define DAC1FID32_CSPEED_NOT_AVAILABLE		127
#define AIS_TYPE8_BINARY_MAX	952	/* 952 bits */
#define AIS_DAC1FID13_RADIUS_NOT_AVAILABLE 10001
#define AIS_DAC1FID13_EXTUNIT_NOT_AVAILABLE 0
#define DAC1FID17_IDTYPE_MMSI		0
#define DAC1FID17_IDTYPE_IMO		1
#define DAC1FID17_IDTYPE_CALLSIGN	2
#define DAC1FID17_IDTYPE_OTHER		3
#define DAC1FID17_ID_LENGTH		7
#define DAC1FID17_COURSE_NOT_AVAILABLE		360
#define DAC1FID17_SPEED_NOT_AVAILABLE		255
#define AIS_DAC1FID29_TEXT_MAX	162	/* 920 bits of six-bit, plus NUL */
#define DAC1FID31_LATLON_SCALE	1000
#define DAC1FID31_LON_NOT_AVAILABLE	(181*60*DAC1FID31_LATLON_SCALE)
#define DAC1FID31_LAT_NOT_AVAILABLE	(91*60*DAC1FID31_LATLON_SCALE)
#define DAC1FID31_WIND_HIGH			126
#define DAC1FID31_WIND_NOT_AVAILABLE		127
#define DAC1FID31_DIR_NOT_AVAILABLE		360
#define DAC1FID31_AIRTEMP_NOT_AVAILABLE		-1084
#define DAC1FID31_HUMIDITY_NOT_AVAILABLE	101
#define DAC1FID31_DEWPOINT_NOT_AVAILABLE	501
#define DAC1FID31_PRESSURE_NOT_AVAILABLE	511
#define DAC1FID31_PRESSURE_HIGH			402
#define DAC1FID31_PRESSURETREND_NOT_AVAILABLE	3
#define DAC1FID31_VISIBILITY_NOT_AVAILABLE	127
#define DAC1FID11_WATERLEVEL_NOT_AVAILABLE	4001
#define DAC1FID31_WATERLEVEL_NOT_AVAILABLE	40001
#define DAC1FID31_LEVELTREND_NOT_AVAILABLE	3
#define DAC1FID31_CSPEED_NOT_AVAILABLE		255
#define DAC1FID31_CDEPTH_NOT_AVAILABLE		301
#define DAC1FID31_HEIGHT_NOT_AVAILABLE		31
#define DAC1FID31_PERIOD_NOT_AVAILABLE		63
#define DAC1FID31_SEASTATE_NOT_AVAILABLE	15
#define DAC1FID31_PRECIPTYPE_NOT_AVAILABLE	7
#define DAC1FID31_SALINITY_NOT_AVAILABLE	510
#define AIS_ALT_NOT_AVAILABLE	4095
#define AIS_ALT_HIGH    	4094	/* 4094 meters or higher */
#define AIS_SAR_SPEED_NOT_AVAILABLE	1023
#define AIS_SAR_FAST_MOVER  	1022
#define AIS_TYPE12_TEXT_MAX	157	/* 936 bits of six-bit, plus NUL */
#define AIS_TYPE14_TEXT_MAX	161	/* 952 bits of six-bit, plus NUL */
#define AIS_GNSS_LATLON_SCALE	600.0
#define AIS_TYPE17_BINARY_MAX	736	/* 920 bits */
#define AIS_GNS_LON_NOT_AVAILABLE	0x1a838
#define AIS_GNS_LAT_NOT_AVAILABLE	0xd548
#define AIS_CHANNEL_LATLON_SCALE	600.0
#define AIS_TYPE25_BINARY_MAX	128	/* Up to 128 bits */

struct route_info {
    unsigned int linkage;	/* Message Linkage ID */
    unsigned int sender;	/* Sender Class */
    unsigned int rtype;		/* Route Type */
    unsigned int month;		/* Start month */
    unsigned int day;		/* Start day */
    unsigned int hour;		/* Start hour */
    unsigned int minute;	/* Start minute */
    unsigned int duration;	/* Duration */
    int waycount;		/* Waypoint count */
    struct waypoint_t {
	signed int lon;		/* Longitude */
	signed int lat;		/* Latitude */
    } waypoints[16];
};

struct ais_t
{
    unsigned int	type;		/* message type */
    unsigned int    	repeat;		/* Repeat indicator */
    unsigned int	mmsi;		/* MMSI */
    union {

	/* Types 1-3 Common navigation info */
	struct {
	    unsigned int status;		/* navigation status */
	    signed turn;			/* rate of turn */
	    unsigned int speed;			/* speed over ground in deciknots */
	    bool accuracy;			/* position accuracy */
	    int lon;				/* longitude */
	    int lat;				/* latitude */
	    unsigned int course;		/* course over ground */
	    unsigned int heading;		/* true heading */
	    unsigned int second;		/* seconds of UTC timestamp */
	    unsigned int maneuver;	/* maneuver indicator */
	    //unsigned int spare;	spare bits */
	    bool raim;			/* RAIM flag */
	    unsigned int radio;		/* radio status bits */
	} type1;

	/* Type 4 - Base Station Report & Type 11 - UTC and Date Response */
	struct {
	    unsigned int year;			/* UTC year */
	    unsigned int month;			/* UTC month */
	    unsigned int day;			/* UTC day */
	    unsigned int hour;			/* UTC hour */
	    unsigned int minute;		/* UTC minute */
	    unsigned int second;		/* UTC second */
	    bool accuracy;		/* fix quality */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    unsigned int epfd;		/* type of position fix device */
	    //unsigned int spare;	spare bits */
	    bool raim;			/* RAIM flag */
	    unsigned int radio;		/* radio status bits */
	} type4;

	/* Type 5 - Ship static and voyage related data */
	struct {
	    unsigned int ais_version;	/* AIS version level */
	    unsigned int imo;		/* IMO identification */
	    char callsign[7+1];		/* callsign */ 
	    char shipname[AIS_SHIPNAME_MAXLEN+1];	/* vessel name */
	    unsigned int shiptype;	/* ship type code */
	    unsigned int to_bow;	/* dimension to bow */
	    unsigned int to_stern;	/* dimension to stern */
	    unsigned int to_port;	/* dimension to port */
	    unsigned int to_starboard;	/* dimension to starboard */
	    unsigned int epfd;		/* type of position fix deviuce */
	    unsigned int month;		/* UTC month */
	    unsigned int day;		/* UTC day */
	    unsigned int hour;		/* UTC hour */
	    unsigned int minute;	/* UTC minute */
	    unsigned int draught;	/* draft in meters */
	    char destination[20+1];	/* ship destination */
	    unsigned int dte;		/* data terminal enable */
	    //unsigned int spare;	spare bits */
	} type5;

	/* Type 6 - Addressed Binary Message */
	struct {
	    unsigned int seqno;		/* sequence number */
	    unsigned int dest_mmsi;	/* destination MMSI */
	    bool retransmit;		/* retransmit flag */
	    //unsigned int spare;	spare bit(s) */
	    unsigned int dac;           /* Application ID */
	    unsigned int fid;           /* Functional ID */
	    size_t bitcount;		/* bit count of the data */
	    union {
		char bitdata[(AIS_TYPE6_BINARY_MAX + 7) / 8];
		/* IMO236 - Dangerous Cargo Indication */
		struct {
		    char lastport[5+1];		/* Last Port Of Call */
		    unsigned int lmonth;	/* ETA month */
		    unsigned int lday;		/* ETA day */
		    unsigned int lhour;		/* ETA hour */
		    unsigned int lminute;	/* ETA minute */
		    char nextport[5+1];		/* Next Port Of Call */
		    unsigned int nmonth;	/* ETA month */
		    unsigned int nday;		/* ETA day */
		    unsigned int nhour;		/* ETA hour */
		    unsigned int nminute;	/* ETA minute */
		    char dangerous[20+1];	/* Main Dangerous Good */
		    char imdcat[4+1];		/* IMD Category */
		    unsigned int unid;		/* UN Number */
		    unsigned int amount;	/* Amount of Cargo */
		    unsigned int unit;		/* Unit of Quantity */
		} dac1fid12;
		/* IMO236 - Extended Ship Static and Voyage Related Data */
		struct {
		    unsigned int airdraught;	/* Air Draught */
		} dac1fid15;
		/* IMO236 - Number of Persons on board */
		struct {
		    unsigned persons;	/* number of persons */
		} dac1fid16;
		/* IMO289 - Clearance Time To Enter Port */
		struct {
		    unsigned int linkage;	/* Message Linkage ID */
		    unsigned int month;	/* Month (UTC) */
		    unsigned int day;	/* Day (UTC) */
		    unsigned int hour;	/* Hour (UTC) */
		    unsigned int minute;	/* Minute (UTC) */
		    char portname[20+1];	/* Name of Port & Berth */
		    char destination[5+1];	/* Destination */
		    signed int lon;	/* Longitude */
		    signed int lat;	/* Latitude */
		} dac1fid18;
		/* IMO289 - Berthing Data (addressed) */
		struct {
		    unsigned int linkage;	/* Message Linkage ID */
		    unsigned int berth_length;	/* Berth length */
		    unsigned int berth_depth;	/* Berth Water Depth */
		    unsigned int position;	/* Mooring Position */
		    unsigned int month;	/* Month (UTC) */
		    unsigned int day;	/* Day (UTC) */
		    unsigned int hour;	/* Hour (UTC) */
		    unsigned int minute;	/* Minute (UTC) */
		    unsigned int availability;	/* Services Availability */
		    unsigned int agent;	/* Agent */
		    unsigned int fuel;	/* Bunker/fuel */
		    unsigned int chandler;	/* Chandler */
		    unsigned int stevedore;	/* Stevedore */
		    unsigned int electrical;	/* Electrical */
		    unsigned int water;	/* Potable water */
		    unsigned int customs;	/* Customs house */
		    unsigned int cartage;	/* Cartage */
		    unsigned int crane;	/* Crane(s) */
		    unsigned int lift;	/* Lift(s) */
		    unsigned int medical;	/* Medical facilities */
		    unsigned int navrepair;	/* Navigation repair */
		    unsigned int provisions;	/* Provisions */
		    unsigned int shiprepair;	/* Ship repair */
		    unsigned int surveyor;	/* Surveyor */
		    unsigned int steam;	/* Steam */
		    unsigned int tugs;	/* Tugs */
		    unsigned int solidwaste;	/* Waste disposal (solid) */
		    unsigned int liquidwaste;	/* Waste disposal (liquid) */
		    unsigned int hazardouswaste;	/* Waste disposal (hazardous) */
		    unsigned int ballast;	/* Reserved ballast exchange */
		    unsigned int additional;	/* Additional services */
		    unsigned int regional1;	/* Regional reserved 1 */
		    unsigned int regional2;	/* Regional reserved 2 */
		    unsigned int future1;	/* Reserved for future */
		    unsigned int future2;	/* Reserved for future */
		    char berth_name[20+1];	/* Name of Berth */
		    signed int berth_lon;	/* Longitude */
		    signed int berth_lat;	/* Latitude */
		} dac1fid20;
		/* IMO289 - Dangerous Cargo Indication */
		struct {
		    unsigned int unit;	/* Unit of Quantity */
		    unsigned int amount;	/* Amount of Cargo */
		    int ncargos;
		    struct cargo_t {
			unsigned int code;	/* Cargo code */
			unsigned int subtype;	/* Cargo subtype */
		    } cargos[28];
		} dac1fid25;
		/* IMO289 - Route info (addressed) */
		struct route_info dac1fid28;
		/* IMO289 - Text message (addressed) */
		struct {
		    unsigned int linkage;
		    char text[AIS_DAC1FID30_TEXT_MAX];
		} dac1fid30;
		/* IMO289 & IMO236 - Tidal Window */
		struct {
		    unsigned int type;	/* Message Type */
		    unsigned int repeat;	/* Repeat Indicator */
		    unsigned int mmsi;	/* Source MMSI */
		    unsigned int seqno;	/* Sequence Number */
		    unsigned int dest_mmsi;	/* Destination MMSI */
		    signed int retransmit;	/* Retransmit flag */
		    unsigned int dac;	/* DAC */
		    unsigned int fid;	/* FID */
		    unsigned int month;	/* Month */
		    unsigned int day;	/* Day */
		    signed int ntidals;
		    struct tidal_t {
			signed int lon;	/* Longitude */
			signed int lat;	/* Latitude */
			unsigned int from_hour;	/* From UTC Hour */
			unsigned int from_min;	/* From UTC Minute */
			unsigned int to_hour;	/* To UTC Hour */
			unsigned int to_min;	/* To UTC Minute */
			unsigned int cdir;	/* Current Dir. Predicted */
			unsigned int cspeed;	/* Current Speed Predicted */
		    } tidals[3];
		} dac1fid32;
	    };
	} type6;

	/* Type 7 - Binary Acknowledge */
	struct {
	    unsigned int mmsi1;
	    unsigned int mmsi2;
	    unsigned int mmsi3;
	    unsigned int mmsi4;
	    /* spares ignored, they're only padding here */
	} type7;

	/* Type 8 - Broadcast Binary Message */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    unsigned int dac;       	/* Designated Area Code */
	    unsigned int fid;       	/* Functional ID */
	    size_t bitcount;		/* bit count of the data */
	    union {
		char bitdata[(AIS_TYPE8_BINARY_MAX + 7) / 8];
		/* IMO236 - Fairway Closed */
		struct {
		    char reason[20+1];	/* Reason For Closing */
		    char closefrom[20+1];	/* Location Of Closing From */
		    char closeto[20+1];	/* Location of Closing To */
		    unsigned int radius;	/* Radius extension */
		    unsigned int extunit;	/* Unit of extension */
		    unsigned int fday;	/* From day (UTC) */
		    unsigned int fmonth;	/* From month (UTC) */
		    unsigned int fhour;	/* From hour (UTC) */
		    unsigned int fminute;	/* From minute (UTC) */
		    unsigned int tday;	/* To day (UTC) */
		    unsigned int tmonth;	/* To month (UTC) */
		    unsigned int thour;	/* To hour (UTC) */
		    unsigned int tminute;	/* To minute (UTC) */
		} dac1fid13;
	        /* IMO236 - Extended ship and voyage data */
		struct {
		    unsigned int airdraught;	/* Air Draught */
		} dac1fid15;
		/* IMO289 - VTS-generated/Synthetic Targets */
		struct {
		    signed int ntargets;
		    struct target_t {
			unsigned int idtype;	/* Identifier type */
			union target_id {	/* Target identifier */
			    unsigned int mmsi;
			    unsigned int imo;
			    char callsign[DAC1FID17_ID_LENGTH+1];
			    char other[DAC1FID17_ID_LENGTH+1];
			} id;
			signed int lat;		/* Latitude */
			signed int lon;		/* Longitude */
			unsigned int course;	/* Course Over Ground */
			unsigned int second;	/* Time Stamp */
			unsigned int speed;	/* Speed Over Ground */
		    } targets[4];
		} dac1fid17;
		/* IMO 289 - Marine Traffic Signal */
		struct {
		    unsigned int linkage;	/* Message Linkage ID */
		    char station[20+1];		/* Name of Signal Station */
		    signed int lon;		/* Longitude */
		    signed int lat;		/* Latitude */
		    unsigned int status;	/* Status of Signal */
		    unsigned int signal;	/* Signal In Service */
		    unsigned int hour;		/* UTC hour */
		    unsigned int minute;	/* UTC minute */
		    unsigned int nextsignal;	/* Expected Next Signal */
		} dac1fid19;
		/* IMO289 - Route info (broadcast) */
		struct route_info dac1fid27;
		/* IMO289 - Text message (broadcast) */
		struct {
		    unsigned int linkage;
		    char text[AIS_DAC1FID29_TEXT_MAX];
		} dac1fid29;
		/* IMO236 & IMO289 - Meteorological-Hydrological data */
		struct {
		    bool accuracy;	/* position accuracy, <10m if true */
		    int lon;		/* longitude in minutes * .001 */
		    int lat;		/* longitude in minutes * .001 */
		    unsigned int day;		/* UTC day */
		    unsigned int hour;		/* UTC hour */
		    unsigned int minute;	/* UTC minute */
		    unsigned int wspeed;	/* average wind speed */
		    unsigned int wgust;		/* wind gust */
		    unsigned int wdir;		/* wind direction */
		    unsigned int wgustdir;	/* wind gust direction */
		    int airtemp;		/* temperature, units 0.1C */
		    unsigned int humidity;	/* relative humidity, % */
		    int dewpoint;		/* dew point, units 0.1C */
		    unsigned int pressure;	/* air pressure, hpa */
		    unsigned int pressuretend;	/* tendency */
		    bool visgreater;            /* visibility greater than */
		    unsigned int visibility;	/* units 0.1 nautical miles */
		    int waterlevel;		/* decimeters or cm */
		    unsigned int leveltrend;	/* water level trend code */
		    unsigned int cspeed;	/* current speed in deciknots */
		    unsigned int cdir;		/* current dir., degrees */
		    unsigned int cspeed2;	/* current speed in deciknots */
		    unsigned int cdir2;		/* current dir., degrees */
		    unsigned int cdepth2;	/* measurement depth, 0.1m */
		    unsigned int cspeed3;	/* current speed in deciknots */
		    unsigned int cdir3;		/* current dir., degrees */
		    unsigned int cdepth3;	/* measurement depth, 0.1m */
		    unsigned int waveheight;	/* in decimeters */
		    unsigned int waveperiod;	/* in seconds */
		    unsigned int wavedir;	/* direction in degrees */
		    unsigned int swellheight;	/* in decimeters */
		    unsigned int swellperiod;	/* in seconds */
		    unsigned int swelldir;	/* direction in degrees */
		    unsigned int seastate;	/* Beaufort scale, 0-12 */
		    int watertemp;		/* units 0.1deg Celsius */
		    unsigned int preciptype;	/* 0-7, enumerated */
		    unsigned int salinity;	/* units of 0.1% */
		    bool ice;			/* is there sea ice? */
		} dac1fid31;
	    };
	} type8;

	/* Type 9 - Standard SAR Aircraft Position Report */
	struct {
	    unsigned int alt;		/* altitude in meters */
	    unsigned int speed;		/* speed over ground in deciknots */
	    bool accuracy;		/* position accuracy */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    unsigned int course;	/* course over ground */
	    unsigned int second;	/* seconds of UTC timestamp */
	    unsigned int regional;	/* regional reserved */
	    unsigned int dte;		/* data terminal enable */
	    //unsigned int spare;	spare bits */
	    bool assigned;		/* assigned-mode flag */
	    bool raim;			/* RAIM flag */
	    unsigned int radio;		/* radio status bits */
	} type9;

	/* Type 10 - UTC/Date Inquiry */
	struct {
	    //unsigned int spare;
	    unsigned int dest_mmsi;	/* destination MMSI */
	    //unsigned int spare2;
	} type10;

	/* Type 12 - Safety-Related Message */
	struct {
	    unsigned int seqno;		/* sequence number */
	    unsigned int dest_mmsi;	/* destination MMSI */
	    bool retransmit;		/* retransmit flag */
	    //unsigned int spare;	spare bit(s) */
	    char text[AIS_TYPE12_TEXT_MAX];
	} type12;

	/* Type 14 - Safety-Related Broadcast Message */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    char text[AIS_TYPE14_TEXT_MAX];
	} type14;

	/* Type 15 - Interrogation */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    unsigned int mmsi1;
	    unsigned int type1_1;
	    unsigned int offset1_1;
	    //unsigned int spare2;	spare bit(s) */
	    unsigned int type1_2;
	    unsigned int offset1_2;
	    //unsigned int spare3;	spare bit(s) */
	    unsigned int mmsi2;
	    unsigned int type2_1;
	    unsigned int offset2_1;
	    //unsigned int spare4;	spare bit(s) */
	} type15;

	/* Type 16 - Assigned Mode Command */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    unsigned int mmsi1;
	    unsigned int offset1;
	    unsigned int increment1;
	    unsigned int mmsi2;
	    unsigned int offset2;
	    unsigned int increment2;
	} type16;

	/* Type 17 - GNSS Broadcast Binary Message */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    //unsigned int spare2;	spare bit(s) */
	    size_t bitcount;		/* bit count of the data */
	    char bitdata[(AIS_TYPE17_BINARY_MAX + 7) / 8];
	} type17;

	/* Type 18 - Standard Class B CS Position Report */
	struct {
	    unsigned int reserved;	/* altitude in meters */
	    unsigned int speed;		/* speed over ground in deciknots */
	    bool accuracy;		/* position accuracy */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    unsigned int course;	/* course over ground */
	    unsigned int heading;	/* true heading */
	    unsigned int second;	/* seconds of UTC timestamp */
	    unsigned int regional;	/* regional reserved */
	    bool cs;     		/* carrier sense unit flag */
	    bool display;		/* unit has attached display? */
	    bool dsc;   		/* unit attached to radio with DSC? */
	    bool band;   		/* unit can switch frequency bands? */
	    bool msg22;	        	/* can accept Message 22 management? */
	    bool assigned;		/* assigned-mode flag */
	    bool raim;			/* RAIM flag */
	    unsigned int radio;		/* radio status bits */
	} type18;

	/* Type 19 - Extended Class B CS Position Report */
	struct {
	    unsigned int reserved;	/* altitude in meters */
	    unsigned int speed;		/* speed over ground in deciknots */
	    bool accuracy;		/* position accuracy */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    unsigned int course;	/* course over ground */
	    unsigned int heading;	/* true heading */
	    unsigned int second;	/* seconds of UTC timestamp */
	    unsigned int regional;	/* regional reserved */
	    char shipname[AIS_SHIPNAME_MAXLEN+1];		/* ship name */
	    unsigned int shiptype;	/* ship type code */
	    unsigned int to_bow;	/* dimension to bow */
	    unsigned int to_stern;	/* dimension to stern */
	    unsigned int to_port;	/* dimension to port */
	    unsigned int to_starboard;	/* dimension to starboard */
	    unsigned int epfd;		/* type of position fix deviuce */
	    bool raim;			/* RAIM flag */
	    unsigned int dte;    	/* date terminal enable */
	    bool assigned;		/* assigned-mode flag */
	    //unsigned int spare;	spare bits */
	} type19;

	/* Type 20 - Data Link Management Message */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    unsigned int offset1;	/* TDMA slot offset */
	    unsigned int number1;	/* number of xlots to allocate */
	    unsigned int timeout1;	/* allocation timeout */
	    unsigned int increment1;	/* repeat increment */
	    unsigned int offset2;	/* TDMA slot offset */
	    unsigned int number2;	/* number of xlots to allocate */
	    unsigned int timeout2;	/* allocation timeout */
	    unsigned int increment2;	/* repeat increment */
	    unsigned int offset3;	/* TDMA slot offset */
	    unsigned int number3;	/* number of xlots to allocate */
	    unsigned int timeout3;	/* allocation timeout */
	    unsigned int increment3;	/* repeat increment */
	    unsigned int offset4;	/* TDMA slot offset */
	    unsigned int number4;	/* number of xlots to allocate */
	    unsigned int timeout4;	/* allocation timeout */
	    unsigned int increment4;	/* repeat increment */
	} type20;

	/* Type 21 - Aids to Navigation Report */
	struct {
	    unsigned int aid_type;	/* aid type */
	    char name[35];		/* name of aid to navigation */
	    bool accuracy;		/* position accuracy */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    unsigned int to_bow;	/* dimension to bow */
	    unsigned int to_stern;	/* dimension to stern */
	    unsigned int to_port;	/* dimension to port */
	    unsigned int to_starboard;	/* dimension to starboard */
	    unsigned int epfd;		/* type of EPFD */
	    unsigned int second;	/* second of UTC timestamp */
	    bool off_position;		/* off-position indicator */
	    unsigned int regional;	/* regional reserved field */
	    bool raim;			/* RAIM flag */
	    bool virtual_aid;		/* is virtual station? */
	    bool assigned;		/* assigned-mode flag */
	    //unsigned int spare;	unused */
	} type21;

	/* Type 22 - Channel Management */
	struct {
	    //unsigned int spare;	spare bit(s) */
	    unsigned int channel_a;	/* Channel A number */
	    unsigned int channel_b;	/* Channel B number */
	    unsigned int txrx;		/* transmit/receive mode */
	    bool power;			/* high-power flag */
	    union {
		struct {
		    int ne_lon;		/* NE corner longitude */
		    int ne_lat;		/* NE corner latitude */
		    int sw_lon;		/* SW corner longitude */
		    int sw_lat;		/* SW corner latitude */
		} area;
		struct {
		    unsigned int dest1;	/* addressed station MMSI 1 */
		    unsigned int dest2;	/* addressed station MMSI 2 */
		} mmsi;
	    };
	    bool addressed;		/* addressed vs. broadast flag */
	    bool band_a;		/* fix 1.5kHz band for channel A */
	    bool band_b;		/* fix 1.5kHz band for channel B */
	    unsigned int zonesize;	/* size of transitional zone */
	} type22;

	/* Type 23 - Group Assignment Command */
	struct {
	    int ne_lon;			/* NE corner longitude */
	    int ne_lat;			/* NE corner latitude */
	    int sw_lon;			/* SW corner longitude */
	    int sw_lat;			/* SW corner latitude */
	    //unsigned int spare;	spare bit(s) */
	    unsigned int stationtype;	/* station type code */
	    unsigned int shiptype;	/* ship type code */
	    //unsigned int spare2;	spare bit(s) */
	    unsigned int txrx;		/* transmit-enable code */
	    unsigned int interval;	/* report interval */
	    unsigned int quiet;		/* quiet time */
	    //unsigned int spare3;	spare bit(s) */
	} type23;

	/* Type 24 - Class B CS Static Data Report */
	struct {
	    char shipname[AIS_SHIPNAME_MAXLEN+1];	/* vessel name */
	    unsigned int shiptype;	/* ship type code */
	    char vendorid[8];		/* vendor ID */
	    char callsign[8];		/* callsign */
	    struct {
		unsigned int mothership_mmsi;	/* MMSI of main vessel */
		struct {
		    unsigned int to_bow;	/* dimension to bow */
		    unsigned int to_stern;	/* dimension to stern */
		    unsigned int to_port;	/* dimension to port */
		    unsigned int to_starboard;	/* dimension to starboard */
		} dim;
	    };
	} type24;

	/* Type 25 - Addressed Binary Message */
	struct {
	    bool addressed;		/* addressed-vs.broadcast flag */
	    bool structured;		/* structured-binary flag */
	    unsigned int dest_mmsi;	/* destination MMSI */
	    unsigned int app_id;        /* Application ID */
	    size_t bitcount;		/* bit count of the data */
	    char bitdata[(AIS_TYPE25_BINARY_MAX + 7) / 8];
	} type25;

	/* Type 26 - Addressed Binary Message */
	struct {
	    bool addressed;		/* addressed-vs.broadcast flag */
	    bool structured;		/* structured-binary flag */
	    unsigned int dest_mmsi;	/* destination MMSI */
	    unsigned int app_id;        /* Application ID */
	    size_t bitcount;		/* bit count of the data */
	    char bitdata[(AIS_TYPE26_BINARY_MAX + 7) / 8];
	    unsigned int radio;		/* radio status bits */
	} type26;

	/* Type 27 - Long Range AIS Broadcast message */
	struct {
	    bool accuracy;		/* position accuracy */
	    bool raim;			/* RAIM flag */
	    unsigned int status;	/* navigation status */
	    int lon;			/* longitude */
	    int lat;			/* latitude */
	    unsigned int speed;		/* speed over ground in deciknots */
	    unsigned int course;	/* course over ground */

	    bool gnss;			/* are we reporting GNSS position? */
	} type27;
    };

};

int
aivdm_decode(const char *buf, size_t buflen,
	     struct aivdm_context_t ais_contexts[AIVDM_CHANNELS],
	     struct ais_t *ais);



#endif /* _GPSD_GPS_H_ */
