/*
 * DeviceSpecificFunctions.h
 *
 * The following script comprises all of the functions used for the operation of the device and are also unique to such device
 * 
 * Device: Stethoscope
 *
 * Michael Xynidis
 * Fluvio Lobo Fenoglietto
 * 10/27/2016
 */

 
#include <SerialFrame.h>

//
// *** Variables
//

SerialFrame sf1 = SerialFrame();

Frame txFr = (Frame)
  {
    123,
    STETHOSCOPE,
    HEARTRATE,
    "0"
  };

File          frec;
File          hRate;

elapsedMillis msecs;
elapsedMillis triggerTime;
elapsedMillis elapsed;
elapsedMillis timeStamp;
elapsedMillis restTime;

int           ndx           = 0;
static int    heartRateI    = 0;
unsigned int  hrSample[3]   = { 0, 0, 0 };
unsigned int  intervalRate  = 0;
unsigned int  minS1S2       = 120;                      // must be greater-than 100ms
unsigned int  maxS1S2       = 350;                      // must be less-than 400ms
unsigned int  maxHBInterval = 5000;                     //ms

float         sigThreshold  = 0.30;

bool          beatHeard     = false;
bool          atRest        = true;
bool          transition    = atRest;
bool          soundTwo      = false;

String        lineOut       = "";


//
// *** Adjust Microphone Gain Level
//
void adjustMicLevel()
{
  // TODO: read the peak_QrsMeter1 object and adjust sgtl5000_1.micGain()
  // if anyone gets this working, please submit a github pull request :-)
}


//
// *** Wave Amplitude Peaks
//
bool waveAmplitudePeaks()
{
  if ( msecs > 40 )                                     // sample at 25MHz
  {
     msecs = 0;
    if ( peak_QrsMeter.available() )
    {
      float peakNumber  = 0.0;

      peakNumber = peak_QrsMeter.read();

      int   leftPeak    = peakNumber  * 30.0;
      int   count;
      char  fillCharL   = ' ';
      char  fillCharR   = ' ';

      Serial.print( "Threshold: " );
      Serial.print( peakNumber );
      Serial.print( "/" );
      Serial.print( sigThreshold );
      Serial.print( " Peak: " );
      Serial.print( leftPeak );
      Serial.print( '\t' );

      for ( int i = 0; i < 30 - leftPeak; i++ )
      {
         if ( i == (30 - (sigThreshold * 30)) )
          Serial.print( "." );
        else
          Serial.print( " " );
      }

      if ( peakNumber >= sigThreshold )                 // Sound is heard.
      {
        fillCharL = '<';
        fillCharR = '>';
        if ( atRest )                                   // If heart is at rest, then a sound is heard...
          transition  = true;                           // ....there is a 'transition',
        else                                            // otherwise...
          transition  = false;                          // ....there is no transition
        atRest        = false;                          // ....but with either event, the heart is NOT at rest.
      }
      else                                              // No sound heard (or, below threshold)
      {
        fillCharL = ' ';
        fillCharR = ' ';
        if ( !atRest )                                  // If heart was NOT at rest, then NO sound is heard...
          transition  = true;                           // ....there is a 'transition',
        else                                            // otherwise...
          transition  = false;                          // ....there is no transition
        atRest        = true;                           // ....but with either event, the heart is presently at rest.
      }


      if ( transition && !atRest && !soundTwo )         // transitioning into (potential) 1st heart sound
      {
        if ( restTime > minS1S2 )
        {
          intervalRate  = triggerTime;
          triggerTime   = 0;
        }
      }

      else if ( !transition && !atRest && !soundTwo )   // within (potential) 1st heart sound *(may be unnecessary)*
      {
      }

      else if ( transition && atRest && !soundTwo )     // transitioning out of (potential) 1st heart sound
      {
        soundTwo      = true;
        restTime      = 0;
      }

      else if ( transition && !atRest && soundTwo )     // transitioning into (potential) 2nd heart sound
      {
        if ( ( triggerTime >= minS1S2 )                 // qualifies as 2nd heart sound if S1S2 interval falls...
          && ( triggerTime <= maxS1S2 ) )               // ....within the defined range
        {
          beatHeard     = true;
        }
      }

      else if ( !transition && !atRest && soundTwo )    // within (potential) 1st heart sound *(may be unnecessary)*
      {
        beatHeard     = false;
      }

      else if ( transition && atRest && soundTwo )      // transitioning out of (potential) 2nd heart sound
      {
        soundTwo      = false;
        beatHeard     = false;
        intervalRate  = 0;
      }

      else if ( !transition && atRest && restTime > maxS1S2 )
      {
        soundTwo = false;
      }
      
      else
        beatHeard     = false;


      if ( beatHeard )                                  // This section calculates a running average...
      {                                                 // heart rate from three successive samples
        hrSample[ndx] = elapsed;  // - timestamp;
        if ( ndx == 2 )
        {
          heartRateI = 60000 / ((hrSample[0] + hrSample[1] + hrSample[2]) / 3);
          for ( int i = 0; i < 3; i++ ) hrSample[i] = 0;
          ndx = 0;
        }
        else ndx++;
        elapsed = 0;
      }
//      else if ( elapsed > maxHBInterval )               // If no heartbeat is detected within this...
//        heartRateI = 0;                                 // ....interval, heart rate is set to zero.

      for ( int i = 0; i < leftPeak; i++ )
        Serial.print( fillCharL );

      if ( beatHeard )
        Serial.print( "|*|" );
      else if ( transition )
        Serial.print( "|.|" );
      else if ( atRest )
        Serial.print( "| |" );
      else
        Serial.print( "|-|" );

      for ( count = 0; count < leftPeak; count++ )
        Serial.print( fillCharR );

      while ( count++ <= 30 )
      {
         if ( count == sigThreshold * 30 )
          Serial.print( "." );
        else
          Serial.print( " " );
      }

      if ( transition )
      {
        Serial.print( "\ttime: " );
        Serial.print( triggerTime );
      }
      if ( beatHeard )
      {
        Serial.print( "\tHR: " );
        Serial.print( heartRateI );
      }
      Serial.println();
    }
  }
  return beatHeard;
} // End of waveAmplitudePeaks()


void switchMode( int m )
{
    Serial.print( "\nMode = "  );  Serial.print( mode );
    mode        = m;                                                                                            // Change value of operation mode for continous recording
    Serial.print( " -> "  );  Serial.println( mode );
}

//
// *** Start Recording
//
boolean startRecording()
{
  Serial.println( "EXECUTING startRecording()" );                                                               // Identification of function executed

  mixer_mic_Sd.gain( 0, mixerInputON  );                                                                        // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
  mixer_mic_Sd.gain( 1, mixerInputON  );                                                                        // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
  mixer_mic_Sd.gain( 2, mixerInputOFF );                                                                        // Set gain of mixer_mic_Sd, channel2 to 0
  
  char  fileRec[ses.fileRec.length()+1];                                                                        // Conversion from string to character array
  ses.fileRec.toCharArray( fileRec, sizeof( fileRec ) );

  char  fileDat[ses.fileDat.length()+1];                                                                        // Conversion from string to character array
  ses.fileDat.toCharArray( fileDat, sizeof( fileDat ) );
  
  if ( SD.exists( fileRec ) ) SD.remove( fileRec );                                                             // Check for existence of RECORD.RAW
  if ( SD.exists( fileDat ) ) SD.remove( fileDat );                                                             // Check for existence of HRATE.DAT

  frec  = SD.open( fileRec, FILE_WRITE );                                                                       // Create and open RECORD.RAW file
  Serial.println( frec );
  hRate = SD.open( fileDat,  FILE_WRITE );                                                                      // Create and open HRATE.DAT file
  Serial.println( hRate );

  if (  SD.open( fileRec, FILE_WRITE ) &&                                                                       // Create and open RECORD.RAW file
        SD.open( fileDat, FILE_WRITE ) )                                                                        // Create and open HRATE.DAT file
  {

    Serial.print( "\nMinimum S1-S2 interval:\t\t"       );  Serial.print(   minS1S2       );  Serial.print( "ms" );
    Serial.print( "\nMaximum S1-S2 interval:\t\t"       );  Serial.print(   maxS1S2       );  Serial.print( "ms" );
    Serial.print( "\nNo-heartrate detect interval:\t"   );  Serial.print(   maxHBInterval );  Serial.print( "ms" );
    Serial.print( "\nHearsound detection threshold:\t"  );  Serial.print(   sigThreshold  );
    Serial.print( "\nLow-pass filter roll-off freq:\t"  );  Serial.print(   freqLowPass   );
    Serial.print( "\nHighshelf filter roll-off freq:\t" );  Serial.println( freqHighShelf );
    Serial.print( "\nRecord file on SD is named:\t"   );    Serial.print(   fileRec       );
    Serial.print( "\nHR Data file on SD is named:\t"    );  Serial.println( fileDat       );

    queue_recMic.begin();
    recordState = RECORDING;
    switchMode( 1 );
    timeStamp   = 0;
    sf1.StartSend( STRING, 1000 );                                                                              // Begin transmitting heartrate data as a String
    Serial.println( "Stethoscope began RECORDING" );                                                            // Function execution confirmation over USB serial
    Serial.println( "sending: ACK..." );
    BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
    Serial.println( "continueRecording() called" );
    return true;
  }
  else
    Serial.println( "Stethoscope CANNOT begin RECORDING" );                                                     // Function execution failed, notification over USB serial
    Serial.println( "sending: NAK..." );
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
}


//
// *** Continue Recording
//
void continueRecording()
{
  if ( queue_recMic.available() >= 2 )
  {
    byte buffer[512];                                                                                           // Fetch 2 blocks from the audio library and copy into a 512 byte buffer.
                                                                                                                // The Arduino SD library is most efficient when full 512 byte sector size writes are used.
    memcpy( buffer, queue_recMic.readBuffer(), 256 );
    queue_recMic.freeBuffer();
    memcpy( buffer + 256, queue_recMic.readBuffer(), 256 );
    queue_recMic.freeBuffer();                                                                                  // write all 512 bytes to the SD card
    //elapsedMicros usec = 0;
    frec.write( buffer, 512 );
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    //Serial.print( "SD write, us = " );
    //Serial.println( usec );
    bool beatCaptured = waveAmplitudePeaks();                                                                   // write HR and time to file at each heart beat
    if ( beatCaptured )
    {
      lineOut = String( heartRateI, DEC ) + "," + String( timeStamp, DEC ) + "\r\n";
      hRate.print( lineOut );
      txFr = sf1.Get();                                                                                         // get values from existing TX data frame
      txFr.DataString = String( heartRateI );                                                                   // update data-string value with heartrate
      sf1.Set( txFr );                                                                                          // set TX data frame with new heartate value
    }
  }
} // End of continueRecording()


//
// *** Stop Recording
//
boolean stopRecording()
{
  Serial.println( "EXECUTING stopRecording" );

  mixer_mic_Sd.gain( 0, mixerInputON  );                                                                        // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
  mixer_mic_Sd.gain( 1, mixerInputON  );                                                                        // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
  mixer_mic_Sd.gain( 2, mixerInputOFF );                                                                        // Set gain of mixer_mic_Sd, channel2 to 0
  
  queue_recMic.end();
  if ( recordState == RECORDING )
  { 
    sf1.StopSend( STRING );                                                                                     // Terminate transmitting heartrate data as a String
    Serial.println( "Stethoscope will STOP RECORDING" );                                                        // Function execution confirmation over USB serial
    Serial.println( "sending: ACK..." );
    BTooth.write( ACK );
    while ( queue_recMic.available() > 0 )
    {
      frec.write( (byte*)queue_recMic.readBuffer(), 256 );
      queue_recMic.freeBuffer();
    }
    frec.close();
    hRate.close();
    recordState = STANDBY;
    switchMode( 4 );
    return true;
  }
  else
    Serial.println( "Stethoscope CANNOT STOP RECORDING" );                                                      // Function execution confirmation over USB serial
    Serial.println( "sending: NAK..." );
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
}


//
// *** Start Playing
//
boolean startPlaying( String fileName )
{
  Serial.println( "EXECUTING startPlaying()" );                                                                 // Identification of function executed

  mixer_mic_Sd.gain( 0, mixerInputOFF );                                                                        // Set the microphone channel 0 to mute (gain value = 0)
  mixer_mic_Sd.gain( 1, mixerInputOFF );                                                                        // Set the microphone channel 1 to mute (gain value = 0)
  mixer_mic_Sd.gain( 2, mixerInputON  );                                                                        // Set the gain of the playback audio signal

  char  filePly[fileName.length()+1];                                                                           // Conversion from string to character array
  fileName.toCharArray( filePly, sizeof( filePly ) );
  
  if ( SD.exists( filePly ) )
  {
    playRaw_sdHeartSound.play( filePly );
    recordState = PLAYING;
    switchMode( 2 );
    Serial.println( "Stethoscope began PLAYING" );                                                              // Function execution confirmation over USB serial
    Serial.println( "sending: ACK..." );
    BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
    return true;
  }
  else
    Serial.println( "Stethoscope CANNOT begin PLAYING" );                                                       // Function execution confirmation over USB serial
    Serial.println( "sending: NAK..." );
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
}


//
// *** Continue Playing
//
void continuePlaying() 
{
  if ( !playRaw_sdHeartSound.isPlaying() )
  {
    playRaw_sdHeartSound.stop();
  }
}


//
// *** Stop Playing
//
boolean stopPlaying()
{
  Serial.println( "stopPlaying" );
  if ( recordState == PLAYING ) playRaw_sdHeartSound.stop();
  recordState = STANDBY;
  switchMode( 4 );
  Serial.println( "Stethoscope stopping PLAY" );                                                              // Function execution confirmation over USB serial
  Serial.println( "sending: ACK..." );
  BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
  return true;
}


//
// *** Start Blending
// Start Blending is a variant of the Start Playing/Playback function.  Instead of muting one channel,
//  the function progressively attenuates one channel while amplifying an overlaying signal.
boolean startBlending( String fileName )
{
  Serial.println( "EXECUTING startBlending()" );                                                                // Identification of function executed

  mixer_mic_Sd.gain( 0, mixerInputON  );                                                                        // Keep the microphone channel 0 at its normal gain value
  mixer_mic_Sd.gain( 1, mixerInputON  );                                                                        // Keep the microphone channel 1 at its normal gain value
  mixer_mic_Sd.gain( 2, mixerInputOFF );                                                                        // Set the gain of the playback audio signal to mute for starter

  char  filePly[fileName.length()+1];                                                                           // Conversion from string to character array
  fileName.toCharArray( filePly, sizeof( filePly ) );
  Serial.println(filePly);
  Serial.println(SD.exists(filePly));
  
  if ( SD.exists( filePly ) )
  {
    playRaw_sdHeartSound.play( filePly );
    recordState = PLAYING;
    switchMode( 5 );
    Serial.println( "Stethoscope began BLENDING" );                                                             // Function execution confirmation over USB serial
    BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
    return true;
  }
  else
    Serial.println( "Stethoscope CANNOT begin BLENDING" );                                                      // Function execution confirmation over USB serial
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
}


//
// *** Continue Blending
//
void continueBlending() 
{
  if ( !playRaw_sdHeartSound.isPlaying() )
  {
    playRaw_sdHeartSound.stop();
  }
  if ( mixerLvL > 0.10 )
  {
    mixerLvL = mixerLvL - 0.000005;
    mixer_mic_Sd.gain( 0, mixerLvL );
    mixer_mic_Sd.gain( 1, mixerLvL );
    mixer_mic_Sd.gain( 2, (1 - mixerLvL) );
    Serial.println( mixerLvL );
  }
}


//
// *** Stop Blending
//
boolean stopBlending()
{
  Serial.println( "stopBlending" );
  if ( recordState == PLAYING ) playRaw_sdHeartSound.stop();
  mixerLvL    = 1;
  recordState = STANDBY;
  switchMode( 4 );
  Serial.println( "Stethoscope stopped BLENDING" );                                                             // Function execution confirmation over USB serial
  Serial.println( "sending: ACK..." );
  BTooth.write( ACK );                                                                                          // ACKnowledgement sent back through bluetooth serial
  return true;
}


//
// *** Start Microphone Passthrough Mode
//     This function re-configures the stethoscope to pass audio through the microphone to the speakers
//
boolean startAudioPassThrough()
{
  Serial.println( "EXECUTING startAudioPassThrough()" );
  if ( recordState == RECORDING ) stopRecording();                                                              // Stop recording if recording
  if ( recordState == PLAYING   ) stopPlaying();                                                                // Stop playback if playing

  if ( selectedInput == AUDIO_INPUT_MIC )
  {
    mixer_mic_Sd.gain( 0, mixerInputON  );                                                                      // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
    mixer_mic_Sd.gain( 1, mixerInputON  );                                                                      // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
    mixer_mic_Sd.gain( 2, mixerInputOFF );                                                                      // Set gain of mixer_mic_Sd, channel2 to 0
    recordState = PASSTHRU;
    switchMode( 4 );
    Serial.println( "Stethoscope switched AUDIO PASSTHROUGH mode." );                                           // Function execution confirmation over USB serial
    Serial.println( "sending: ACK..." );
    BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
    return true;
  }
  else
  {
    Serial.println( "Stethoscope CANNOT switch to AUDIO PASSTHROUGH mode." );                                   // Function execution confirmation over USB serial
    Serial.println( "sending: NAK..." );
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
  }
} // End of startAudioPassThrough()


//
// *** Continue Microphone Passthrough Mode
//
boolean continueAudioPassThrough()
{
  mixer_mic_Sd.gain( 0, mixerInputON  );                                                                        // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
  mixer_mic_Sd.gain( 1, mixerInputON  );                                                                        // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
  mixer_mic_Sd.gain( 2, mixerInputOFF );                                                                        // Set gain of mixer_mic_Sd, channel2 to 0
  recordState = PASSTHRU;
  return true;
} // End of continueAudioPassThrough()


//
// *** Start Detecting Heartbeat Peaks from Microphone Audio.
//     This function uses the wave peak detection tool to find and measure heartbeat/rate from the microphone audio.
//     The function does not record data to a file, rather is needed to send information for remote display via serial port communication.
//     Note that the function acts as an alternative to startMicStream().
//
boolean startHeartBeatMonitoring()
{
  Serial.println( "EXECUTING startTrackingMicStream()" );
  if ( recordState == RECORDING ) stopRecording();                                                              // Stop recording if recording
  if ( recordState == PLAYING ) stopPlaying();                                                                  // Stop playback if playing
  if ( selectedInput == AUDIO_INPUT_MIC )
  {
    mixer_mic_Sd.gain( 0, mixerInputON  );                                                                      // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
    mixer_mic_Sd.gain( 1, mixerInputON  );                                                                      // Set gain of mixer_mic_Sd, channel0 to 0.5 - Microphone on
    mixer_mic_Sd.gain( 2, mixerInputOFF );                                                                      // Set gain of mixer_mic_Sd, channel2 to 0
    queue_recMic.begin();
    recordState = DETECTING;
    switchMode( 3 );
    sf1.StartSend( STRING, 1000 );                                                                              // Begin transmitting heartrate data as a String
    Serial.println( "Stethoscope STARTed DETECTING heartbeat from MIC audio." );                                // Function execution confirmation over USB serial
    Serial.println( "sending: ACK..." );
    BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
    return true;
  }
  else
  {
    Serial.println( "Stethoscope CANNOT START DETECTING heartbeat from MIC audio." );                           // Function execution confirmation over USB serial
    Serial.println( "sending: NAK..." );
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
  }
} // End of startMonitoring()


//
// *** Continue Tracking Microphone Stream
//     This is the companion function to trackingMicStream()
//     The function continues the tracking of audio peaks
//
boolean continueHeartBeatMonitoring()
{
    bool beatCaptured = waveAmplitudePeaks();                                                                   // write HR and time to file at each heart beat
    if ( beatCaptured )
    {
      txFr = sf1.Get();                                                                                         // get values from existing TX data frame
      txFr.DataString = String( heartRateI );                                                                   // update data-string value with heartrate
      sf1.Set( txFr );                                                                                          // set TX data frame with new heartate value
    }
  return true;
} // End of continueMonitoring()



//
// *** Stop Tracking Microphone Stream
//     This function terminates startTrackingMicStream().
//
boolean stopHeartBeatMonitoring()
{
  Serial.println( "EXECUTING stopTrackingMicStream()" );
  if ( recordState == DETECTING )
  {
    recordState = PASSTHRU;
    switchMode( 4 );
    sf1.StopSend( STRING );                                                                                     // Terminate transmitting heartrate data as a String
    Serial.println( "Stethoscope will STOP DETECTING heartbeat from MIC audio." );                              // Function execution confirmation over USB serial
    Serial.println( "sending: ACK..." );
    BTooth.write( ACK );                                                                                        // ACKnowledgement sent back through bluetooth serial
    return true;
  }
  else
  {
    Serial.println( "Stethoscope CANNOT STOP DETECTING heartbeat from MIC audio." );                            // Function execution confirmation over USB serial
    Serial.println( "sending: NAK..." );
    BTooth.write( NAK );                                                                                        // Negative AcKnowledgement sent back through bluetooth serial
    return false;
  }
} // End of stopHeartBeatMonitoring()
