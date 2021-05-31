#ifndef MIMBROLA_CONFIG_H
#define MIMBROLA_CONFIG_H

#define __data_header(x) #x
/*
 * Change line below corresponding to your voice folder
 * for us1 full quality it will be
 * #define _data_header(x) __data_header(data/us1_full/espola##x)
 */
#define _data_header(x) __data_header(data/pl1_alaw/espola##x)
#define data_header(x) _data_header(x)

/*
 * uncomment for Arduino and internal DAC to minimize clicks
 * must be commented out for I2S external DAC!
 */ 
#define USE_ANTICLICK

#endif
