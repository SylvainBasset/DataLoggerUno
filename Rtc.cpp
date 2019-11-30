

#include <Arduino.h>

#include "DataLoggerUno.h"

#include <Wire.h> // Pour la communication I2C



/*--------------------------------------------------------------------*/
/* Defines                                                            */
/*--------------------------------------------------------------------*/

/** Adresse I2C du module RTC DS1307 */
const uint8_t DS1307_ADDRESS = 0x68;

/** Adresse du registre de contrôle du module RTC DS1307 */
const uint8_t DS1307_CTRL_REGISTER = 0x07;

/** Adresse et taille de la NVRAM du module RTC DS1307 */
const uint8_t DS1307_NVRAM_BASE = 0x08;
const uint8_t DS1307_NVRAM_SIZE = 56;


typedef enum
{
  SQW_1_HZ = 0,      /**!< Signal à 1Hz sur la broche SQW */
  SQW_4096_HZ,       /**!< Signal à 4096Hz sur la broche SQW */
  SQW_8192_HZ,       /**!< Signal à 8192Hz sur la broche SQW */
  SQW_32768_HZ,      /**!< Signal à 32768Hz sur la broche SQW */
  SQW_DC             /**!< Broche SQW toujours à LOW ou HIGH */
} DS1307_Mode_t ;

#define RTC_HOURS_AMPM_BIT       0x40
#define RTC_SECONDS_INVALID_BIT    0x80

static s_DateTime const k_DefaultDatetime =
{
   .u8Seconds = 0,
   .u8Minutes = 0,
   .u8Hours = 0,
   .u8WeekDay = 0,
   .u8Days = 1,
   .u8Months = 1,
   .u8Year = 0,
} ;


/*--------------------------------------------------------------------*/
/* Prototypes                                                         */
/*--------------------------------------------------------------------*/

static byte bcd_to_decimal(byte bcd) ;
static byte decimal_to_bcd(byte decimal) ;


/*--------------------------------------------------------------------*/

bool rtc_ReadCurDatetime( s_DateTime * o_pDatetime )
{
   s_DateTime RawDateTime ;
   bool bInvalidDatetime ;

   Wire.setClock(10000) ;

   /* Début de la transaction I2C */
   Wire.beginTransmission(DS1307_ADDRESS);
   Wire.write((byte) 0); // Lecture mémoire à l'adresse 0x00
   Wire.endTransmission(); // Fin de la transaction I2C

   /* Lit 7 octets depuis la mémoire du module RTC */
   Wire.requestFrom(DS1307_ADDRESS, (byte) 7);

   RawDateTime.u8Seconds = Wire.read() ;
   RawDateTime.u8Minutes = Wire.read() ;
   RawDateTime.u8Hours = Wire.read() ;
   RawDateTime.u8WeekDay = Wire.read() ;
   RawDateTime.u8Days = Wire.read() ;
   RawDateTime.u8Months = Wire.read() ;
   RawDateTime.u8Year = Wire.read() ;

   if ( ( ! ISSET( RawDateTime.u8Seconds, RTC_SECONDS_INVALID_BIT ) ) &&
        ( ! ISSET( RawDateTime.u8Hours, RTC_HOURS_AMPM_BIT ) ) )
   {
      o_pDatetime->u8Seconds = bcd_to_decimal( RawDateTime.u8Seconds ) ;
      o_pDatetime->u8Minutes = bcd_to_decimal( RawDateTime.u8Minutes ) ;
      o_pDatetime->u8Hours =   bcd_to_decimal( RawDateTime.u8Hours ) ;
      o_pDatetime->u8WeekDay = bcd_to_decimal( RawDateTime.u8WeekDay ) ;
      o_pDatetime->u8Days =    bcd_to_decimal( RawDateTime.u8Days ) ;
      o_pDatetime->u8Months =  bcd_to_decimal( RawDateTime.u8Months ) ;
      o_pDatetime->u8Year =    bcd_to_decimal( RawDateTime.u8Year ) ;

      bInvalidDatetime = false ;
   }
   else
   {
      *o_pDatetime = k_DefaultDatetime ;
      bInvalidDatetime = true ;
   }

   return bInvalidDatetime ;
}


/*--------------------------------------------------------------------*/

void rtc_AdjustCurrentDatetime( s_DateTime C* i_pDatetime )
{
   Wire.setClock(10000) ;

   Wire.beginTransmission(DS1307_ADDRESS) ;
   Wire.write((byte) 0); // Ecriture mémoire à l'adresse 0x00

   Wire.write( decimal_to_bcd( i_pDatetime->u8Seconds ) & 127) ;
   Wire.write( decimal_to_bcd( i_pDatetime->u8Minutes ) ) ;
   Wire.write( decimal_to_bcd( i_pDatetime->u8Hours ) & 63) ;
   Wire.write( decimal_to_bcd( i_pDatetime->u8WeekDay ) ) ;
   Wire.write( decimal_to_bcd( i_pDatetime->u8Days ) ) ;
   Wire.write( decimal_to_bcd( i_pDatetime->u8Months ) ) ;
   Wire.write( decimal_to_bcd( i_pDatetime->u8Year ) ) ;

   Wire.endTransmission(); // Fin de transaction I2C
}


/*====================================================================*/

/*--------------------------------------------------------------------*/

/** Fonction de conversion BCD -> decimal */
static byte bcd_to_decimal(byte bcd)
{
  return (bcd / 16 * 10) + (bcd % 16);
}

/** Fonction de conversion decimal -> BCD */
static byte decimal_to_bcd(byte decimal)
{
  return (decimal / 10 * 16) + (decimal % 10);
}