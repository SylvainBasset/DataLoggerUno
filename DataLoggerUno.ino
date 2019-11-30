
#include "DataLoggerUno.h"

#include <SPI.h> // Pour la communication SPI
#include <SD.h>  // Pour la communication avec la carte SD

#include <Wire.h> // Pour la communication I2C


/*--------------------------------------------------------------------*/
/* Defines                                                            */
/*--------------------------------------------------------------------*/

#define NB_CHANNELS                    2

#define DLG_FILENAME_FMT  "Rec_%02d.csv"

#define DLG_ECH_PERIOD                10

#define DLG_DISP_PERIOD             1062

#define DLG_MOY_DEPTH                  4

#define DLG_ADC_VOLT_REF            4980     /* tension de reference de l'ADC */

#define DLG_ADC_SCALE               1024     /* échelle ADC 10 bits */

#define DLG_V1_VOTLAGE_OFFSET         10

typedef struct
{
   int AnalogPin1 ;
   int AnalogPin2 ;
   uint16_t u16ResmOhm ;
} s_ChannelDesc ;

static s_ChannelDesc const k_aChDesc [NB_CHANNELS] =
{
   {
      .AnalogPin1 = DLG_CHANNEL1_V1,
      .AnalogPin2 = DLG_CHANNEL1_V2,
      .u16ResmOhm = 1000,
   },
   {
      .AnalogPin1 = DLG_CHANNEL2_V1,
      .AnalogPin2 = DLG_CHANNEL2_V2,
      .u16ResmOhm = 1000,
   },
} ;

//
///** Broche CS de la carte SD */
//static const byte SDCARD_CS_PIN = 10; // A remplacer suivant votre shield SD

typedef struct
{
   s_BasicFilter FilterData1 ;
   uint16_t u16RawValues1 [DLG_MOY_DEPTH] ;
   s_BasicFilter FilterData2 ;
   uint16_t u16RawValues2 [DLG_MOY_DEPTH] ;

   uint16_t u16Volt ;
   uint16_t u16VoltCur ;
   int16_t i16Current ;
   bool bMesOk ;
} s_Measure ;


/*--------------------------------------------------------------------*/
/* Variables                                                          */
/*--------------------------------------------------------------------*/

static s_Measure l_aMes [NB_CHANNELS] ;

static uint32_t l_u32DispIdx ;

static uint32_t l_u32TmpReadAdc ;
static uint32_t l_u32TmpSendData ;

static char l_szDataFileName [13] ;

static bool l_bError ;

/*--------------------------------------------------------------------*/
/* Prototypes                                                         */
/*--------------------------------------------------------------------*/

static bool dlg_InitFile( void ) ;
static bool dlg_WriteFile( char C* i_pszStr ) ;
static void dlg_ProcessMeasure( void ) ;


/*--------------------------------------------------------------------*/
/* Initialisation                                                     */
/*--------------------------------------------------------------------*/

void setup()
{
   s_DateTime DateTime ;
   char szLine [48] ;

   Serial.begin(115200) ;           /* Initialisation du port série (debug) */

   pinMode( DLG_CHANNEL1_RELAY, OUTPUT ) ;
   pinMode( DLG_CHANNEL2_RELAY, OUTPUT ) ;


   //DateTime.u8Seconds = 0 ;
   //DateTime.u8Minutes = 0 ;
   //DateTime.u8Hours = 12 ; // 12h 0min 0sec
   //DateTime.u8WeekDay = 4 ;
   //DateTime.u8Days = 1 ;
   //DateTime.u8Months = 12 ;
   //DateTime.u8Year = 16 ; // 1 dec 2016
   //rtc_AdjustCurrentDatetime( &DateTime ) ;

   if ( ! l_bError )
   {
      l_bError = rtc_ReadCurDatetime( &DateTime ) ;

      Serial.println( DateTime.u8Seconds ) ;
      Serial.println( DateTime.u8Minutes ) ;
      Serial.println( DateTime.u8Hours ) ;
      Serial.println( DateTime.u8WeekDay ) ;
      Serial.println( DateTime.u8Days ) ;
      Serial.println( DateTime.u8Months ) ;
      Serial.println( DateTime.u8Year ) ;
      Serial.println(F("-------------------"));

      if ( l_bError )
      {
         Serial.println(F("Error : invalid date/time")) ;
      }
   }

   if ( ! l_bError )
   {
      snprintf( szLine, sizeof(szLine),
                "date/time : 20%02d-%02d-%02d:%02d-%02d-%02d",
                DateTime.u8Year, DateTime.u8Months, DateTime.u8Days,
                DateTime.u8Hours, DateTime.u8Minutes, DateTime.u8Seconds ) ;

      l_bError = dlg_InitFile() ;
   }

   if ( l_bError )
   {
      Serial.println(F("Initialisation Error, Datalogger is not working"));
      for( ;; ) ;
   }

   bflt_Init( &l_aMes[0].FilterData1, l_aMes[0].u16RawValues1, ARRAY_SIZE( l_aMes[0].u16RawValues1 ) ) ;
   bflt_Init( &l_aMes[0].FilterData2, l_aMes[0].u16RawValues2, ARRAY_SIZE( l_aMes[0].u16RawValues2 ) ) ;
   l_aMes[0].bMesOk = true ;

   bflt_Init( &l_aMes[1].FilterData1, l_aMes[1].u16RawValues1, ARRAY_SIZE( l_aMes[1].u16RawValues1 ) ) ;
   bflt_Init( &l_aMes[1].FilterData2, l_aMes[1].u16RawValues2, ARRAY_SIZE( l_aMes[1].u16RawValues2 ) ) ;
   l_aMes[1].bMesOk = true ;

   digitalWrite( DLG_CHANNEL1_RELAY, HIGH ) ;
   digitalWrite( DLG_CHANNEL2_RELAY, HIGH ) ;

   tmp_StartTmpMs( &l_u32TmpReadAdc ) ;
   tmp_StartTmpMs( &l_u32TmpSendData ) ;
}


/*--------------------------------------------------------------------*/
/* cyclic task                                                        */
/*--------------------------------------------------------------------*/

void loop()
{
   s_DateTime DateTime ;
   char szLine [80] ;
   uint8_t u8ReadSerial ;
   uint8_t i ;

   if ( ! l_bError )
   {
      if ( tmp_IsEndTmpMs( &l_u32TmpReadAdc, DLG_ECH_PERIOD ) )
      {
         tmp_StartTmpMs( &l_u32TmpReadAdc ) ;

         dlg_ProcessMeasure() ;

         if ( tmp_IsEndTmpMs( &l_u32TmpSendData, DLG_DISP_PERIOD ) )
         {
            tmp_StartTmpMs( &l_u32TmpSendData ) ;

            rtc_ReadCurDatetime( &DateTime ) ;

            snprintf( szLine, sizeof(szLine),
                      "%05lu; %02d-%02d-%02d-%02d-%02d-%02d; %u; %u; %d; %u; %u; %d\r\n",
                      l_u32DispIdx,
                      DateTime.u8Year, DateTime.u8Months, DateTime.u8Days,
                      DateTime.u8Hours, DateTime.u8Minutes, DateTime.u8Seconds,
                      l_aMes[0].u16Volt, l_aMes[0].u16VoltCur, l_aMes[0].i16Current,
                      l_aMes[1].u16Volt, l_aMes[1].u16VoltCur, l_aMes[1].i16Current ) ;

            Serial.println( szLine ) ;
            dlg_WriteFile( szLine ) ;
            l_u32DispIdx++ ;
         }
      }

      if ( l_u32DispIdx > 5 )
      {
         for ( i = 0 ; i < ARRAY_SIZE( l_aMes ) ; i++ )
         {
            if ( l_aMes[i].bMesOk )
            {
               if ( l_aMes[i].u16Volt > 8400 )
               {
                  strncpy( szLine, "Tension trop haute", sizeof(szLine) ) ;
                  l_aMes[i].bMesOk = false ;
               }

               if ( l_aMes[i].u16Volt < 5000 )
               {
                  strncpy( szLine, "Tension trop basse", sizeof(szLine) ) ;
                  l_aMes[i].bMesOk = false ;
               }
               if ( l_aMes[i].i16Current > 2000 )
               {
                  strncpy( szLine, "courant trop haut", sizeof(szLine) ) ;
                  l_aMes[i].bMesOk = false ;
               }

               if ( ! l_aMes[i].bMesOk )
               {
                  Serial.println( szLine ) ;
                  dlg_WriteFile( szLine ) ;
                  if ( i == 0 )
                  {
                     digitalWrite(7, LOW) ;
                     strncpy( szLine, "Arret mesure 1", sizeof(szLine) ) ;
                  }
                  else
                  {
                     digitalWrite( DLG_CHANNEL2_RELAY, LOW ) ;
                     strncpy( szLine, "Arret mesure 2", sizeof(szLine) ) ;
                  }
                  Serial.println( szLine ) ;
                  dlg_WriteFile( szLine ) ;
               }
            }
         }
      }

      if (Serial.available() > 0)
      {
         u8ReadSerial = Serial.read();

         if ( u8ReadSerial == 'e' )
         {
            Serial.println( F("End") ) ;

            digitalWrite(DLG_CHANNEL1_RELAY, LOW) ;
            digitalWrite(DLG_CHANNEL2_RELAY, LOW) ;
            for ( ;; ) ;
         }
      }
   }
}


/*====================================================================*/

/*--------------------------------------------------------------------*/
/* initialize SDcard file                                             */
/*--------------------------------------------------------------------*/

static bool dlg_InitFile( void )
{
   bool bError ;
   uint16_t u16Index ;

   bError = false ;

   if ( ! bError )
   {
      if ( ! SD.begin( DLG_SDCARD_CS ) )
      {
         Serial.println( F("Error : SD Init Err") ) ;
         l_bError = true ;
      }
   }

   if ( ! bError )
   {
      u16Index = 0 ;

      for ( u16Index = 0 ; u16Index < 1000 ; u16Index++ )
      {
         sprintf( l_szDataFileName, DLG_FILENAME_FMT, u16Index ) ;

         if ( ! SD.exists( l_szDataFileName ) )
         {
            break ;
         }
      }

      if ( u16Index >= 1000 )
      {
         Serial.println( F("Error : Unreachable filename index") ) ;
         bError = true ;
      }
   }

   if ( ! bError )
   {
      Serial.println( l_szDataFileName ) ;
      bError |= dlg_WriteFile( "Tension T1_1; Tension T1_2; Current T1;" ) ;
      bError |= dlg_WriteFile( "Tension T2_1; Tension T2_2; Current T2;" ) ;
   }

   return bError ;
}


/*--------------------------------------------------------------------*/
/* write string in file (append)                                      */
/*--------------------------------------------------------------------*/

static bool dlg_WriteFile( char C* i_pszStr )
{
   bool bError ;
   File fDatafile ;
   uint16_t u16NbWritten ;

   bError = false ;

   if ( ! bError )
   {
      fDatafile = SD.open( l_szDataFileName, FILE_WRITE ) ;

      if ( fDatafile )
      {
      }
      else
      {
         Serial.println( F("Error : File cant be opened") ) ;
         bError = true ;
      }
   }

   if ( ! bError )
   {
      u16NbWritten = fDatafile.write(i_pszStr);

      if ( u16NbWritten != strlen(i_pszStr) )
      {
         Serial.println( F("Error : File write operation error") ) ;
         Serial.println( u16NbWritten ) ;
         Serial.println( strlen(i_pszStr) ) ;
         bError = true ;
      }
      else
      {
      }

      fDatafile.flush() ;
      fDatafile.close() ;
   }

   return bError ;
}


/*--------------------------------------------------------------------*/
/* Read Adc value for all channels, with average and voltage/current  */
/* calculation                                                        */
/*--------------------------------------------------------------------*/

static void dlg_ProcessMeasure( void )
{
   uint8_t i ;
   uint16_t u16RawVal ;
   uint16_t u16Val ;
   uint16_t u16VoltagemV1 ;
   uint16_t u16VoltagemV2 ;
   s_Measure * pMes ;
   s_ChannelDesc C* pDesc ;

   pMes = l_aMes ;
   pDesc = k_aChDesc ;

   for ( i = 0 ; i < ARRAY_SIZE( l_aMes ) ; i++ )
   {
      u16RawVal = analogRead( pDesc->AnalogPin1 ) ;
      u16Val = bflt_ProcessVal( &pMes->FilterData1, u16RawVal, NULL ) ;
      u16VoltagemV1 = ( ( (uint32_t)u16Val * DLG_ADC_VOLT_REF ) / ( 1024 - 1 ) ) << 1 ;
      //if ( u16VoltagemV1 != 0 )
      //{
      //   u16VoltagemV1 += DLG_V1_VOTLAGE_OFFSET ;
      //}

      u16RawVal = analogRead( pDesc->AnalogPin2 ) ;
      u16Val = bflt_ProcessVal( &pMes->FilterData2, u16RawVal, NULL ) ;
      u16VoltagemV2 = ( ( (uint32_t)u16Val * DLG_ADC_VOLT_REF ) / ( 1024 - 1 ) ) << 1 ;

      pMes->u16Volt = u16VoltagemV1 ;
      pMes->u16VoltCur = u16VoltagemV2 ;
      pMes->i16Current = ( ( (int32_t)u16VoltagemV1 - (int32_t)u16VoltagemV2 ) * 1000 ) / pDesc->u16ResmOhm ;

      pMes++ ;
      pDesc++ ;
   }
}