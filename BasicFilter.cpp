
#include "DataLoggerUno.h"


/*--------------------------------------------------------------------*/

void bflt_Init( s_BasicFilter * io_pFilter, uint16_t * io_pu16RawBuf, byte i_byRawBufSize )
{
  memset( io_pu16RawBuf, 0, i_byRawBufSize ) ;

  io_pFilter->byBufSize = i_byRawBufSize ;
  io_pFilter->byIndex = 0 ;
  io_pFilter->byNbVAls = 0 ;
  io_pFilter->u32Total = 0 ;
  io_pFilter->pu16BufValues = io_pu16RawBuf ;
}


/*--------------------------------------------------------------------*/

unsigned int bflt_ProcessVal( s_BasicFilter * io_pFilter, uint16_t i_u16Value, byte * o_pbyStatus )
{
   uint16_t u16Moy ;

   io_pFilter->u32Total -= io_pFilter->pu16BufValues[io_pFilter->byIndex] ;

   io_pFilter->pu16BufValues[io_pFilter->byIndex] = i_u16Value ;

   io_pFilter->u32Total += i_u16Value ;

   io_pFilter->byIndex = NEXTIDX( io_pFilter->byIndex, io_pFilter->byBufSize ) ;

   if ( io_pFilter->byNbVAls < io_pFilter->byBufSize )
   {
      io_pFilter->byNbVAls++ ;
   }

   u16Moy = io_pFilter->u32Total / io_pFilter->byBufSize ;

   //*o_pbyStatus = 0 ; //SBA not used for now

   return u16Moy ;
}
