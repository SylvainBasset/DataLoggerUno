
#include "DataLoggerUno.h"


/*----------------------------------------------------------------------------*/
/* Definitions                                                                */
/*----------------------------------------------------------------------------*/

#define SYSTICK_FREQ    1000llu
#define TMP_START_FLAG  0x80000000


/*----------------------------------------------------------------------------*/
/* Prototypes                                                                */
/*----------------------------------------------------------------------------*/

static uint32_t tmp_ComRemainTmp( uint32_t * io_pu32Tempo, uint32_t i_u32Delay ) ;


/*----------------------------------------------------------------------------*/
/* Start a millisecond-based temporisation                                    */
/*----------------------------------------------------------------------------*/

void tmp_StartTmpMs( uint32_t * io_pu32Tempo )
{
   *io_pu32Tempo = millis() ;           /* get millisecond counter value */
   *io_pu32Tempo |= TMP_START_FLAG ;    /* add flag to indicate tempo is started */
}


/*----------------------------------------------------------------------------*/
/* Test the end of a millisecond-based temporisation                          */
/*----------------------------------------------------------------------------*/

boolean tmp_IsEndTmpMs( uint32_t * io_pu32Tempo, uint32_t i_u32Delay )
{
   boolean bRet ;

   bRet = false ;
                                       /* a started tempo is ending */
   if ( ( ISSET( *io_pu32Tempo, TMP_START_FLAG ) ) &&
        ( tmp_ComRemainTmp( io_pu32Tempo, i_u32Delay ) == 0 ) )
   {
      *io_pu32Tempo = 0 ;               /* clear value, including #TMP_START_FLAG */
      bRet = true ;                    /* tempo no longer started */
   }

   return bRet ;
}


/*----------------------------------------------------------------------------*/
/* Get remain time of a millisecond-based temporisation                       */
/*----------------------------------------------------------------------------*/

uint32_t tmp_GetRemainMsTmp( uint32_t * io_pu32Tempo, uint32_t i_u32Delay )
{
   uint32_t u32RemainTime ;
                                       /* get remain value */
   u32RemainTime = tmp_ComRemainTmp( io_pu32Tempo, i_u32Delay ) ;

   return u32RemainTime ;
}


/*====================================================================*/

/*----------------------------------------------------------------------------*/
/* Common for remain time calculation                                         */
/*----------------------------------------------------------------------------*/

static uint32_t tmp_ComRemainTmp( uint32_t * io_pu32Tempo, uint32_t i_u32Delay )
{
   uint32_t u32Ret ;
   uint32_t u32CurTime  ;
   uint32_t u32ElapsedTime ;

                                       /* test if delays overflows maximum */
   ERR_FATAL_IF( i_u32Delay >= ( TMP_START_FLAG - 1 ) )

      /* Note: With the use of #TMP_START_FLAG as tempo runing state flag,    */
      /* it is not possible to count until #TMP_START_FLAG value (0x80000000) */
      /* (the start value of watched counter is coded over 31 bits).          */

   u32Ret = 0 ;                        /* Remaining time at 0 by default */
                                       /* is temporisation started ? */
   if ( ISSET( *io_pu32Tempo, TMP_START_FLAG ) )
   {
      u32CurTime = millis() ;     /* get millisecond counter */
                                          /* calculate elapsed time without */
                                          /* #TMP_START_FLAG bit  */
      u32ElapsedTime = ( ( u32CurTime & ~TMP_START_FLAG ) -
                        ( *io_pu32Tempo & ~TMP_START_FLAG ) ) ;

                                          /* does substartction overflows ? */
      if ( ISSET( u32ElapsedTime, TMP_START_FLAG ) )
      {
          u32ElapsedTime &= ~TMP_START_FLAG ;    /* clear MSB to get the absolute value */
      }

      if ( u32ElapsedTime < i_u32Delay )  /* elapsed time doesn't reachs delay ? */
      {                                   /* calculate difference between */
                                          /* elapsed time and delay*/
         u32Ret = i_u32Delay - u32ElapsedTime ;
      }
         /* Note: if elapsed time reachs delay, dwRet */
         /* (remaining time) equals 0 by default.     */
   }

   return u32Ret ;
}
