/*
 * Obtain magnetic field vector using multiplexed LSM9DS1 IMU sensors.
 * Obtained values take into account the magnetic field of the earth
 * and the immediate surrounding area and SUBTRACTS them from the readings
 * to give a calibrated reading independent of the surrounding magnetic field.
 * 
 * AUTHOR     : Daniel Nichols
 * DATE       : UNKNOWN, 2017, Year of Our Lord
 * 
 * MODIFIED BY: Mohammad Odeh
 * DATE       : Aug. 18th, 2017
 */

// Include required libraries
#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>

// Define important constants
#define LSM9DS1_M             0x1E    // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG            0x6B    // Would be 0x6A if SDO_AG is LOW
#define PRINT_DELAY           50      // 40Hz data readings @ 20 [Milliseconds]
#define CALIBRATION_INDEX     50      // Accounting for ambient magnetic fields
#define DECLINATION           6.55    // Accounting for the Earth's magnetic field
#define BAUDRATE              115200  // Serial communication baudrate
#define NPINS                 3       // Number of select pins
#define NSENS                 3       // Number of select pins


LSM9DS1 imu;                    // Declare sensor class
byte Sx_pin[3]  = {13, 12, 11}; // Select pins: {S0, S1, S2}

// Calibration (BASE) readings
static double imu_BASE[3][3] =  { {0, 0, 0},    //  {1x, 1y, 1z}
                                  {0, 0, 0},    //  {2x, 2y, 2z}
                                  {0, 0, 0} };  //  {3x, 3y, 3z}

void setup() {
  Serial.begin( BAUDRATE );             // Start serial monitor

  for ( byte i = 0; i < NPINS; i++ ) {
    pinMode( Sx_pin[i], OUTPUT );       // Set "Select Pins" as output
  }

  // Initialize sensors and load settings
  for (byte i = 0; i < NSENS; i++) {
    setSensor(i);         // Switch between sensors
    setupIMU();           // Setup IMUs
    calibrateIMU(i);      // Calibrate Sensors
  } Serial.println( F("Success.") );
}

void loop() {

  // IMU readings (CALIBRATED) matrix
  static double B[3][3]       = { {0, 0, 0},      //  {B1_x, B1_y, B1_z}
                                  {0, 0, 0},      //  {B2_x, B2_y, B2_z}
                                  {0, 0, 0} };    //  {B3_x, B3_y, B3_z}

  // IMU readings (RAW) matrix
  static double imu_RAW[3][3] = { {0, 0, 0},    //  {B1_x, B1_y, B1_z}
                                  {0, 0, 0},    //  {B2_x, B2_y, B2_z}
                                  {0, 0, 0} };  //  {B3_x, B3_y, B3_z}

  // Obtain readings from ALL sensors
  for (byte i = 0; i < NSENS; i++) {    // Loop over ROWS
    for (byte j = 0; j < NSENS; j++) {  // Loop over COLUMNS
      setSensor(i);                     // Loop over sensors
      delay( PRINT_DELAY / 2 );         // Print speed

      while ( !imu.magAvailable() );    // Check if sensor is ready

      imu.readMag();                    // Do readings
      imu_RAW[i][0] = double( imu.calcMag(imu.mx) );
      imu_RAW[i][1] = double( imu.calcMag(imu.my) );
      imu_RAW[i][2] = double( imu.calcMag(imu.mz) );

      // Subtract BASE readings from RAW readings to get CALIBRATED readings
      B[i][j] =  imu_RAW[i][j] - imu_BASE[i][j];

      // Print readings to Serial
      Serial.print(B[i][j], 5);
      if ( (i == NSENS - 1) && (j == NSENS - 1) ) {
        Serial.print("\n");
      }
      else {
        Serial.print(", ");
      }
    }
  }
}

// =========================    Setup IMU       ========================
void setupIMU() {
  imu.settings.device.commInterface = IMU_MODE_I2C; //
  imu.settings.device.mAddress      = LSM9DS1_M;    // Load IMU settings
  imu.settings.device.agAddress     = LSM9DS1_AG;   //

  imu.settings.gyro.enabled         = false;        // Disable gyro
  imu.settings.accel.enabled        = false;        // Disable accelerometer
  imu.settings.mag.enabled          = true;         // Enable magnetometer
  imu.settings.temp.enabled         = true;         // Enable temperature sensor

  imu.settings.mag.scale            = 16;           // Set mag scale to +/-12 Gs
  imu.settings.mag.sampleRate       = 5;            // Set OD rate to 20Hz
  imu.settings.mag.tempCompensationEnable = true;   // Enable temperature compensation (good stuff!)

  imu.settings.mag.XYPerformance    = 3;            // 0 = Low || 1 = Medium || 2 = High || 3 = Ultra-high
  imu.settings.mag.ZPerformance     = 3;            // Ultra-high performance

  imu.settings.mag.lowPowerEnable   = false;        // Disable low power mode
  imu.settings.mag.operatingMode    = 0;            // 0 = Continuous || 1 = Single || 2 = OFF
}

// =========================   Calibrate IMU    ========================
void calibrateIMU(byte i) {
  // Temporary value holders needed for calibration
  double imu_hold[3] = {0, 0, 0};         // {x, y, z}

  if ( !imu.begin() ) {
    Serial.print( F("Failed to communicate with LSM9DS1 ") );
    Serial.println( i + 1 );
    while (1);
  } else {
    Serial.println( F("Calibrating. Please Wait.") );
    imu.calibrateMag();

    for (int j = 0; j < CALIBRATION_INDEX ; j++) {
      delay( 50 );                        // Delay for stability
      while ( !imu.magAvailable() );      // Wait until readings are available

      imu.readMag();                      // Perform readings and store into temp holders
      imu_hold[0] += imu.calcMag( imu.mx );
      imu_hold[1] += imu.calcMag( imu.my );
      imu_hold[2] += imu.calcMag( imu.mz );

      // Get average
      if (j == CALIBRATION_INDEX - 1) {
        for ( byte k = 0; k < NSENS; k++) {
          imu_BASE[i][k] = imu_hold[k] / CALIBRATION_INDEX;
        }
      }
    }
  }
}

// =========================    Set Sensor      ========================
// Set the value on the multiplexer to set the sensor to use
void setSensor( byte sensorIndex ) {
  // Sensor 1
  if (sensorIndex == 0 ) {
    for (byte i = 0; i < NPINS; i++) {
      digitalWrite(Sx_pin[i], LOW);
    }
  }

  // Sensor 2
  else if (sensorIndex == 1 ) {
    for (byte i = 0; i < NPINS; i++) {
      if (i == 0) {
        digitalWrite(Sx_pin[i], HIGH);
      } else digitalWrite(Sx_pin[i], LOW);
    }
  }


  // Sensor 3
  else if (sensorIndex == 2) {
    for (byte i = 0; i < NPINS; i++) {
      if (i == 1) {
        digitalWrite(Sx_pin[i], HIGH);
      } else digitalWrite(Sx_pin[i], LOW);
    }
  }

  else Serial.println( F("Invalid Index") );
}

