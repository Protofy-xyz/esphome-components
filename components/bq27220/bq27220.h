#pragma once

// FROM LILYGO
#ifndef BQ27227_H
#define BQ27227_H
#define DEFAULT_SCL  18
#define DEFAULT_SDA  8
// END FROM LILYGO

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/hal.h"

//FROM LILYGO
#include "bq27220_def.h"
#include "bq27220_data_memory.h"
//END FROM LILYGO

namespace esphome { // REVIEW ** should be putting imported lilygo bq27220 stuff inside of here or leave it outside? 
namespace bq27220 { // REVIEW ** Same as previous.. Does the imported lilygo code go IN here or outside of the namespaces?





//  FROM LILYGO
typedef union ControlStatus{
    struct __reg
    {
        // Low byte, Low bit first
        uint8_t BATT_ID : 3; /**< Battery Identification */
        bool SNOOZE     : 1; /**< SNOOZE mode is enabled */
        bool BCA        : 1; /**< fuel gauge board calibration routine is active */
        bool CCA        : 1; /**< Coulomb Counter Calibration routine is active */
        uint8_t RSVD0   : 2; /**< Reserved */
        // High byte, Low bit first
        uint8_t RSVD1; /**< Reserved */
    } reg;
    uint16_t full;
} BQ27220ControlStatus;

typedef union BatteryStatus {
    struct __reg
    {
        // Low byte, Low bit first
        uint16_t DSG        : 1; /**< The device is in DISCHARGE */
        uint16_t SYSDWN     : 1; /**< System down bit indicating the system should shut down */
        uint16_t TDA        : 1; /**< Terminate Discharge Alarm */
        uint16_t BATTPRES   : 1; /**< Battery Present detected */
        uint16_t AUTH_GD    : 1; /**< Detect inserted battery */
        uint16_t OCVGD      : 1; /**< Good OCV measurement taken */
        uint16_t TCA        : 1; /**< Terminate Charge Alarm */
        uint16_t RSVD       : 1; /**< Reserved */
        // High byte, Low bit first
        uint16_t CHGING     : 1; /**< Charge inhibit */
        uint16_t FC         : 1; /**< Full-charged is detected */
        uint16_t OTD        : 1; /**< Overtemperature in discharge condition is detected */
        uint16_t OTC        : 1; /**< Overtemperature in charge condition is detected */
        uint16_t SLEEP      : 1; /**< Device is operating in SLEEP mode when set */
        uint16_t OCVFALL    : 1; /**< Status bit indicating that the OCV reading failed due to current */
        uint16_t OCVCOMP    : 1; /**< An OCV measurement update is complete */
        uint16_t FD         : 1; /**< Full-discharge is detected */
    } reg;
    uint16_t full;
}BQ27220BatteryStatus;

typedef enum {
    Bq27220OperationStatusSecSealed = 0b11,
    Bq27220OperationStatusSecUnsealed = 0b10,
    Bq27220OperationStatusSecFull = 0b01,
} Bq27220OperationStatusSec;

typedef union OperationStatus{
    struct __reg
    {
        // Low byte, Low bit first
        bool CALMD      : 1; /**< Calibration mode enabled */
        uint8_t SEC     : 2; /**< Current security access */
        bool EDV2       : 1; /**< EDV2 threshold exceeded */
        bool VDQ        : 1; /**< Indicates if Current discharge cycle is NOT qualified or qualified for an FCC updated */
        bool INITCOMP   : 1; /**< gauge initialization is complete */
        bool SMTH       : 1; /**< RemainingCapacity is scaled by smooth engine */
        bool BTPINT     : 1; /**< BTP threshold has been crossed */
        // High byte, Low bit first
        uint8_t RSVD1   : 2; /**< Reserved */
        bool CFGUPDATE  : 1; /**< Gauge is in CONFIG UPDATE mode */
        uint8_t RSVD0   : 5; /**< Reserved */
    } reg;
    uint16_t full;
} BQ27220OperationStatus;

typedef union GaugingStatus{
    struct __reg
    {
        // Low byte, Low bit first
        bool FD       : 1; /**< Full Discharge */
        bool FC       : 1; /**< Full Charge */
        bool TD       : 1; /**< Terminate Discharge */
        bool TC       : 1; /**< Terminate Charge */
        bool RSVD0    : 1; /**< Reserved */
        bool EDV      : 1; /**< Cell voltage is above or below EDV0 threshold */
        bool DSG      : 1; /**< DISCHARGE or RELAXATION */
        bool CF       : 1; /**< Battery conditioning is needed */
        // High byte, Low bit first
        uint8_t RSVD1 : 2; /**< Reserved */
        bool FCCX     : 1; /**< fcc1hz clock going into CC: 0 = 1 Hz, 1 = 16 Hz*/
        uint8_t RSVD2 : 2; /**< Reserved */
        bool EDV1     : 1; /**< Cell voltage is above or below EDV1 threshold */
        bool EDV2     : 1; /**< Cell voltage is above or below EDV2 threshold */
        bool VDQ      : 1; /**< Charge cycle FCC update qualification */
    } reg;
    uint16_t full;
} BQ27220GaugingStatus;


class BQ27220Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;

  void set_voltage_sensor(sensor::Sensor *sensor) { voltage_sensor_ = sensor; }
  void set_current_sensor(sensor::Sensor *sensor) { current_sensor_ = sensor; }
  void set_soc_sensor(sensor::Sensor *sensor) { soc_sensor_ = sensor; }
  void set_remaining_capacity_sensor(sensor::Sensor *sensor) { remaining_capacity_sensor_ = sensor; }
  void set_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void set_full_charge_capacity_sensor(sensor::Sensor *sensor) { full_charge_capacity_sensor_ = sensor; }
  void set_design_capacity_sensor(sensor::Sensor *sensor) { design_capacity_sensor_ = sensor; }
  void set_state_of_health_sensor(sensor::Sensor *sensor) { state_of_health_sensor_ = sensor; }
  void set_device_number_sensor(sensor::Sensor *sensor) { device_number_sensor_ = sensor; }


 protected:
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *soc_sensor_{nullptr};
  sensor::Sensor *remaining_capacity_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *full_charge_capacity_sensor_{nullptr};
  sensor::Sensor *design_capacity_sensor_{nullptr};
  sensor::Sensor *state_of_health_sensor_{nullptr};
  sensor::Sensor *device_number_sensor_{nullptr};

  
    bool getIsCharging(void){
        BQ27220BatteryStatus batt;
        if(getBatteryStatus(&batt)) {
            return !batt.reg.DSG;
        }
        return false;
    }

    bool parameterCheck(uint16_t address, uint32_t value, size_t size, bool update);
    bool dateMemoryCheck(const BQ27220DMData *data_memory, bool update);

    bool init(const BQ27220DMData *data_memory = gauge_data_memory);
    bool reset(void);

    // Sealed Access
    bool sealAccess(void);
    bool unsealAccess(void);
    bool fullAccess(void);

    // get
    uint16_t getDeviceNumber(void);  // sub-commands
    uint16_t getVoltage(void);
    int16_t getCurrent(void);
    bool getControlStatus(BQ27220ControlStatus *ctrl_sta);
    bool getBatteryStatus(BQ27220BatteryStatus *batt_sta);
    bool getOperationStatus(BQ27220OperationStatus *oper_sta);
    bool getGaugingStatus(BQ27220GaugingStatus *gauging_sta);
    uint16_t getTemperature(void);
    uint16_t getFullChargeCapacity(void);
    uint16_t getDesignCapacity(void);
    uint16_t getRemainingCapacity(void);
    uint16_t getStateOfCharge(void);
    uint16_t getStateOfHealth(void);

    // i2c
    uint16_t readRegU16(uint16_t reg) {
        uint8_t data[2];
        this->read_register(static_cast<uint8_t>(reg), data, size_t{2});
        return ((uint16_t) data[1] << 8) | data[0];
    }

    // uint16_t readCtrlWord(uint16_t fun) {
    //     uint8_t msb = (fun >> 8);
    //     uint8_t lsb = (fun & 0x00FF);
    //     uint8_t cmd[2] = { lsb, msb };
    //     uint8_t data[2] = {0};

    //     this->write_register(static_cast<uint8_t>((uint8_t)BQ27220_COMMAND_CONTROL, cmd, 2);

    //     if (this->read_register(static_cast<uint8_t>((uint8_t) 0, data, 2)) {
    //         return ((uint16_t)data[1] << 8) | data[0];
    //     }
    //     return 0;
    // }

    bool controlSubCmd(uint16_t sub_cmd) {
        uint8_t msb = (sub_cmd >> 8);
        uint8_t lsb = (sub_cmd & 0x00FF);
        uint8_t buf[2] = { lsb, msb };
        this->write_register(static_cast<uint8_t>(CommandControl), buf, 2);
        return true;
    }
private:
   //rem TwoWire *wire = NULL;
    //rem?  uint8_t addr = 0;
    //rem int scl = -1;
    //rem int sda = -1;
    BQ27220BatteryStatus bat_st;
};


// END FROM LILYGO

}  // namespace bq27220
}  // namespace esphome

#endif
