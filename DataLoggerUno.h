

#include <Arduino.h>

/*--------------------------------------------------------------------*/
/* General defines                                                    */
/*--------------------------------------------------------------------*/

                                    /* return elements number for an array */
#define ARRAY_SIZE( Array )          ( sizeof( Array ) / sizeof( Array[0] ) )
                                    /* return next index value in a round buffer array */
#define NEXTIDX( Idx, ArraySize )    ( ( (Idx) < ArraySize - 1 ) ? Idx + 1 : 0 )
                                    /* test if one of bit in set <Bits> is set in <Value> */
#define ISSET( Val, Bits )       ( ( (Val) & (Bits) ) != 0 )

#define C const


typedef struct
{
   uint8_t u8Seconds ;         /* Seconds, 00 - 59 */
   uint8_t u8Minutes ;         /* Minutes, 00 - 59 */
   uint8_t u8Hours ;           /* Hours, 00 - 23 */
   uint8_t u8WeekDay ;         /* Day of the week 01 - 07 */
   uint8_t u8Days ;            /* Day, 01 - 31 */
   uint8_t u8Months ;          /* Month, 01 - 12 */
   uint8_t u8Year ;            /* 2-Digit year (16 = 2016) */
} s_DateTime ;


/*--------------------------------------------------------------------*/
/* Hardware                                                           */
/*--------------------------------------------------------------------*/

                                /* analog pin */
#define DLG_CHANNEL1_V1          A0
#define DLG_CHANNEL1_V2          A1
#define DLG_CHANNEL2_V1          A2
#define DLG_CHANNEL2_V2          A3

                                /* I/O pin */
#define DLG_CHANNEL2_RELAY        6
#define DLG_CHANNEL1_RELAY        7
#define DLG_SDCARD_CS            10



/*--------------------------------------------------------------------*/
/* Rtc                                                                */
/*--------------------------------------------------------------------*/

bool rtc_ReadCurDatetime( s_DateTime * o_pDatetime ) ;
void rtc_AdjustCurrentDatetime( s_DateTime C* i_pDatetime ) ;



/*--------------------------------------------------------------------*/
/* Basic filter (BasicFilter.cpp)                                     */
/*--------------------------------------------------------------------*/

#define BFLT_ST_FLT_VALID    0x01
#define BFLT_ST_FLT_CHG_MOY  0x02

typedef struct
{
   byte byBufSize ;
   byte byIndex ;
   byte byNbVAls ;
   uint32_t u32Total ;
   uint16_t * pu16BufValues ;
} s_BasicFilter ;


unsigned int bflt_ProcessVal( s_BasicFilter * io_pFilter, uint16_t i_u16Value, byte * o_pbyStatus ) ;

void bflt_Init( s_BasicFilter * io_pFilter, uint16_t * io_pu16RawBuf, byte i_byRawBufSize ) ;



/*--------------------------------------------------------------------*/
/* Temporisations (Tempo.cpp)                                         */
/*--------------------------------------------------------------------*/

void tmp_StartTmpMs( uint32_t * io_pu32Tempo ) ;
boolean tmp_IsEndTmpMs( uint32_t * io_pu32Tempo, uint32_t i_u32Delay ) ;



/*----------------------------------------------------------------------------*/
/* Error.c                                                                    */
/*----------------------------------------------------------------------------*/

                                       /* fatal error macro */
#define ERR_FATAL() \
   err_FatalError()
                                       /* fatal error under condition macro */
#define ERR_FATAL_IF( cond ) \
   if ( cond )               \
   {                         \
      err_FatalError() ;     \
   }


void err_FatalError( void ) ;