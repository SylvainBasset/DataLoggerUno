
#include "DataLoggerUno.h"

/*----------------------------------------------------------------------------*/
/* Fatal error                                                                */
/*----------------------------------------------------------------------------*/

void err_FatalError( void )
{
   Serial.println(F("Fatal Error !!!")) ;

   while( true )                       /* infinite loop */
   {

   }
}
