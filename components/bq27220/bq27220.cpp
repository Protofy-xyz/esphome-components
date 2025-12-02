#include "bq27220.h"
#include "bq27220_data_memory.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#define BQ27220_ID (0x0220u)
/** Delay between 2 writes into Subclass/MAC area. Fails at ~120us. */
#define BQ27220_MAC_WRITE_DELAY_US (250u)
/** Delay between we ask chip to load data to MAC and it become valid. Fails at ~500us. */
#define BQ27220_SELECT_DELAY_US (1000u)
/** Delay between 2 control operations(like unseal or full access). Fails at ~2500us.*/
#define BQ27220_MAGIC_DELAY_US (5000u)
/** Delay before freshly written configuration can be read. Fails at ? */
#define BQ27220_CONFIG_DELAY_US (10000u)
/** Config apply delay. Must wait, or DM read returns garbage. */
#define BQ27220_CONFIG_APPLY_US (2000000u)
/** Timeout for common operations. */
#define BQ27220_TIMEOUT_COMMON_US (2000000u)
/** Timeout for reset operation. Normally reset takes ~2s. */
#define BQ27220_TIMEOUT_RESET_US (4000000u)
/** Timeout cycle interval  */
#define BQ27220_TIMEOUT_CYCLE_INTERVAL_US (1000u)
/** Timeout cycles count helper */
#define BQ27220_TIMEOUT(timeout_us) ((timeout_us) / (BQ27220_TIMEOUT_CYCLE_INTERVAL_US))

namespace esphome {
namespace bq27220 {

static const char *const TAG = "bq27220";

static uint8_t bq27220_get_checksum(uint8_t* data, uint16_t len) {
    uint8_t ret = 0;
    for(uint16_t i = 0; i < len; i++) {
        ret += data[i];
    }
    return 0xFF - ret;
}

bool BQ27220Component::parameterCheck(uint16_t address, uint32_t value, size_t size, bool update)
{
    if(!(size == 1 || size == 2 || size == 4)) {
        ESP_LOGE(TAG, "(%d) Parameter size error", __LINE__);
        return false;
    }

    bool ret = false;
    uint8_t buffer[6] = {0};
    uint8_t old_data[4] = {0};

    do {
        buffer[0] = address & 0xFF;
        buffer[1] = (address >> 8) & 0xFF;

        for(size_t i = 0; i < size; i++) {
            buffer[1 + size - i] = (value >> (i * 8)) & 0xFF;
        }

        if(update) {
            if(!this->write_register(static_cast<uint8_t>(CommandSelectSubclass), buffer, size + 2)) {
                ESP_LOGE(TAG, "(%d) DM write failed", __LINE__);
                break;
            }
            // We must wait, otherwise write will fail
            delayMicroseconds(BQ27220_MAC_WRITE_DELAY_US);

            // Calculate the check sum: 0xFF - (sum of address and data) OR 0xFF
            uint8_t checksum = bq27220_get_checksum(buffer, size + 2);
            // Write the check sum to 0x60 and the total length of (address + parameter data + check sum + length) to 0x61
            buffer[0] = checksum;
            // 2 bytes address, `size` bytes data, 1 byte check sum, 1 byte length
            buffer[1] = 2 + size + 1 + 1;
            if(!this->write_register(static_cast<uint8_t>(CommandMACDataSum), buffer, size + 2)) {
                ESP_LOGE(TAG, "(%d) CRC write failed", __LINE__);
                break;
            }
            // We must wait, otherwise write will fail
            delayMicroseconds(BQ27220_CONFIG_DELAY_US);
            ret = true;
        } else {
            if(!this->write_register(static_cast<uint8_t>(CommandSelectSubclass), buffer, 2)) {
                ESP_LOGE(TAG, "(%d) DM SelectSubclass for read failed", __LINE__);
                break;
            }
            // bqstudio uses 15ms wait delay here
            delayMicroseconds(BQ27220_SELECT_DELAY_US);

            if(!this->read_register(static_cast<uint8_t>(CommandMACData), old_data, size)) {
                ESP_LOGE(TAG, "(%d) DM read failed", __LINE__);
                break;
            }
            // bqstudio uses burst reads with continue(CommandSelectSubclass without argument) and ~5ms between burst
            delayMicroseconds(BQ27220_SELECT_DELAY_US);

            if(*(uint32_t*)&(old_data[0]) != *(uint32_t*)&(buffer[2])) {
                ESP_LOGE(TAG, "(%d) Data at 0x%04x(%zu): 0x%08lx!=0x%08lx", __LINE__, address, size,
                         *(uint32_t*)&(old_data[0]), *(uint32_t*)&(buffer[2]));
            } else {
                ret = true;
            }
        }
    } while(0);

    return ret;
}

bool BQ27220Component::dateMemoryCheck(const BQ27220DMData *data_memory, bool update)
{
    if(update) {
        const uint16_t cfg_request = Control_ENTER_CFG_UPDATE;
        if(!this->write_register(static_cast<uint8_t>(CommandSelectSubclass), (uint8_t*)&cfg_request, sizeof(cfg_request))) {
            ESP_LOGE(TAG, "(%d) ENTER_CFG_UPDATE command failed", __LINE__);
            return false;
        }

        // Wait for enter CFG update mode
        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        BQ27220OperationStatus operation_status;
        while(--timeout > 0) {
            if(!getOperationStatus(&operation_status)) {
                ESP_LOGW(TAG, "(%d) Failed to get operation status, retries left %lu", __LINE__, timeout);
            } else if(operation_status.reg.CFGUPDATE) {
                break;
            };
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US);
        }

        if(timeout == 0) {
            ESP_LOGE(TAG, "(%d) Enter CFGUPDATE mode failed, CFG %u, SEC %u", __LINE__,
                     operation_status.reg.CFGUPDATE, operation_status.reg.SEC);
            return false;
        }
    }

    // Process data memory records
    bool result = true;
    while (data_memory->type != BQ27220DMTypeEnd)
    {
        if(data_memory->type == BQ27220DMTypeWait) {
            delayMicroseconds(data_memory->value.u32);
        } else if(data_memory->type == BQ27220DMTypeU8) {
            result &= parameterCheck(data_memory->address, data_memory->value.u8, 1, update);
        } else if(data_memory->type == BQ27220DMTypeU16) {
            result &= parameterCheck(data_memory->address, data_memory->value.u16, 2, update);
        } else if(data_memory->type == BQ27220DMTypeU32) {
            result &= parameterCheck(data_memory->address, data_memory->value.u32, 4, update);
        } else if(data_memory->type == BQ27220DMTypeI8) {
            result &= parameterCheck(data_memory->address, data_memory->value.i8, 1, update);
        } else if(data_memory->type == BQ27220DMTypeI16) {
            result &= parameterCheck(data_memory->address, data_memory->value.i16, 2, update);
        } else if(data_memory->type == BQ27220DMTypeI32) {
            result &= parameterCheck(data_memory->address, data_memory->value.i32, 4, update);
        } else if(data_memory->type == BQ27220DMTypeF32) {
            result &= parameterCheck(data_memory->address, data_memory->value.u32, 4, update);
        } else if(data_memory->type == BQ27220DMTypePtr8) {
            result &= parameterCheck(data_memory->address, *(uint8_t*)data_memory->value.u32, 1, update);
        } else if(data_memory->type == BQ27220DMTypePtr16) {
            result &= parameterCheck(data_memory->address, *(uint16_t*)data_memory->value.u32, 2, update);
        } else if(data_memory->type == BQ27220DMTypePtr32) {
            result &= parameterCheck(data_memory->address, *(uint32_t*)data_memory->value.u32, 4, update);
        } else {
            ESP_LOGE(TAG, "(%d) Invalid DM Type", __LINE__);
        }
        data_memory++;
    }
    
    // Finalize configuration update
    if(update && result) {
        controlSubCmd(Control_EXIT_CFG_UPDATE_REINIT);

        // Wait for gauge to apply new configuration
        delayMicroseconds(BQ27220_CONFIG_APPLY_US);

        // ensure that we exited config update mode
        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        BQ27220OperationStatus operation_status;
        while(--timeout > 0) {
            if(!getOperationStatus(&operation_status)) {
                ESP_LOGW(TAG, "(%d) Failed to get operation status, retries left %lu", __LINE__, timeout);
            } else if(operation_status.reg.CFGUPDATE != true) {
                break;
            }
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US);
        }

        // Check timeout
        if(timeout == 0) {
            ESP_LOGE(TAG, "(%d) Exit CFGUPDATE mode failed", __LINE__);
            return false;
        }
    }
    return result;
}

bool BQ27220Component::init(const BQ27220DMData *data_memory)
{
    bool result = false;
    bool reset_and_provisioning_required = false;

    do{
        uint16_t data = getDeviceNumber();
        if(data != BQ27220_ID) {
            ESP_LOGE(TAG, "(%d) Invalid Device Number %04x != 0x0220", __LINE__, data);
            break;
        }
        
        // Unseal device since we are going to read protected configuration
        if(!unsealAccess()) {
            break;
        }

        // Try to recover gauge from forever init
        BQ27220OperationStatus operat;
        if(!getOperationStatus(&operat)) {
            break;
        }
        if(!operat.reg.INITCOMP || operat.reg.CFGUPDATE) {
            ESP_LOGW(TAG, "(%d) Incorrect state, reset needed", __LINE__);
            reset_and_provisioning_required = true;
        }

        // Ensure correct profile is selected
        ESP_LOGI(TAG, "(%d) Checking chosen profile", __LINE__);
        BQ27220ControlStatus control_status;
        if(!getControlStatus(&control_status)) {
            ESP_LOGE(TAG, "(%d) Failed to get control status", __LINE__);
            break;
        }
        if(control_status.reg.BATT_ID != 0) {
            ESP_LOGW(TAG, "(%d) Incorrect profile, reset needed", __LINE__);
            reset_and_provisioning_required = true;
        }

        // Ensure correct configuration loaded into gauge DataMemory
        // Only if reset is not required, otherwise we don't
        if(!reset_and_provisioning_required) {
            ESP_LOGI(TAG, "(%d) Checking data memory", __LINE__);
            if(!dateMemoryCheck(data_memory, false)) {
                ESP_LOGW(TAG, "(%d) Incorrect configuration data, reset needed", __LINE__);
                reset_and_provisioning_required = true;
            }
        }

        // Reset needed
        if(reset_and_provisioning_required) {
            if(!reset()) {
                ESP_LOGE(TAG, "(%d) Failed to reset device", __LINE__);
            }

            // Get full access to read and modify parameters
            // Also it looks like this step is totally unnecessary
            if(!fullAccess()) {
                break;
            }

            // Update memory
            ESP_LOGI(TAG, "(%d) Updating data memory", __LINE__);
            dateMemoryCheck(data_memory, true);
            if(!dateMemoryCheck(data_memory, false)) {
                ESP_LOGE(TAG, "(%d) Data memory update failed", __LINE__);
                break;
            }
        }

        if(!sealAccess()) {
            ESP_LOGE(TAG, "(%d) Seal failed", __LINE__);
            break;
        }

    } while(0);
    return result;
}

bool BQ27220Component::reset(void)
{
    bool result = false;
    do{
        controlSubCmd(Control_RESET);
        // delay(10);

        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_RESET_US);
        BQ27220OperationStatus operat = {0};
        while (--timeout > 0)
        {
            if(!getOperationStatus(&operat)){
                ESP_LOGW(TAG, "Failed to get operation status, retries left %lu", timeout);
            }else if(operat.reg.INITCOMP == true){
                break;
            }
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US); // delay(2);
        }
        if(timeout == 0) {
            ESP_LOGE(TAG, "INITCOMP timeout after reset");
            break;
        }
        ESP_LOGD(TAG, "(%d) Cycles left: %lu", __LINE__, timeout);
        result = true;
    } while(0);
    return result;
}

// Sealed Access
bool BQ27220Component::sealAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};
    do{
        getOperationStatus(&operat);
        if(operat.reg.SEC == Bq27220OperationStatusSecSealed)
        {
            result = true;
            break;
        }

        controlSubCmd(Control_SEALED);
        // delay(10);
        delayMicroseconds(BQ27220_SELECT_DELAY_US);

        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecSealed)
        {
            ESP_LOGE(TAG, "Seal failed %u", operat.reg.SEC);
            break;
        }
        result = true;
    } while(0);

    return result;
}

bool BQ27220Component::unsealAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};
    do{
        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecSealed)
        {
            result = true;
            break;
        }

        controlSubCmd(UnsealKey1);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); // delay(10);
        controlSubCmd(UnsealKey2);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US);  // delay(10);

        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecUnsealed)
        {
            ESP_LOGE(TAG, "Unseal failed %u", operat.reg.SEC);
            break;
        }
        result = true;
    } while (0);

    return result;
}

bool BQ27220Component::fullAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};

    do{
        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        while (--timeout > 0)
        {
            if(!getOperationStatus(&operat)){
                ESP_LOGW(TAG, "Failed to get operation status, retries left %lu", timeout);
            }else {
                break;
            }
        }
        if(timeout == 0) {
            ESP_LOGE(TAG, "Failed to get operation status");
            break;
        }
        // Serial.printf("Cycles left: %lu\n", timeout);

        // Already full access
        if(operat.reg.SEC == Bq27220OperationStatusSecFull){
            result = true;
            break;
        }
        // Must be unsealed to get full access
        if(operat.reg.SEC != Bq27220OperationStatusSecUnsealed){
            ESP_LOGW(TAG, "(%d) Not in unsealed state", __LINE__);
            break;
        }

        controlSubCmd(FullAccessKey);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); //delay(10);
        controlSubCmd(FullAccessKey);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); //delay(10);

        if(!getOperationStatus(&operat)){
            ESP_LOGE(TAG, "Status query failed");
            break;
        }
        if(operat.reg.SEC != Bq27220OperationStatusSecFull){
            ESP_LOGE(TAG, "Full access failed %u", operat.reg.SEC);
            break;
        }
        result = true;
    } while (0);
    return result;
}

uint16_t BQ27220Component::getDeviceNumber(void)
{
    uint16_t devid = 0;
    // Request device number(chip PN)
    controlSubCmd(Control_DEVICE_NUMBER);
    // Enterprise wait(MAC read fails if less than 500us)
    // bqstudio uses ~15ms 
    delayMicroseconds(BQ27220_SELECT_DELAY_US); // delay(15);
    // Read id data from MAC scratch space
    this->read_register(static_cast<uint8_t>(CommandMACData), (uint8_t *)&devid, 2);

    // Serial.printf("device number:0x%x\n", devid);
    return devid;
}

uint16_t BQ27220Component::getVoltage(void)
{
    return readRegU16(CommandVoltage);
}
int16_t BQ27220Component::getCurrent(void)
{
    return readRegU16(CommandCurrent);
}
bool BQ27220Component::getControlStatus(BQ27220ControlStatus *ctrl_sta)
{
    (*ctrl_sta).full = readRegU16(CommandControl);
    return true;
}
bool BQ27220Component::getBatteryStatus(BQ27220BatteryStatus *batt_sta)
{
    (*batt_sta).full = readRegU16(CommandBatteryStatus);
    return true;
}
bool BQ27220Component::getOperationStatus(BQ27220OperationStatus *oper_sta)
{
    (*oper_sta).full = readRegU16(CommandOperationStatus);
    return true;
}
bool BQ27220Component::getGaugingStatus(BQ27220GaugingStatus *gauging_sta)
{
    // Request gauging data to be loaded to MAC
    controlSubCmd(Control_GAUGING_STATUS);
    // Wait for data being loaded to MAC
    delayMicroseconds(BQ27220_SELECT_DELAY_US);
    // Read id data from MAC scratch space
    (*gauging_sta).full = readRegU16(CommandMACData);
    return true;
}
uint16_t BQ27220Component::getTemperature(void)
{
    return readRegU16(CommandTemperature);
}
uint16_t BQ27220Component::getFullChargeCapacity(void)
{
    return readRegU16(CommandFullChargeCapacity);
}
uint16_t BQ27220Component::getDesignCapacity(void)
{
    return readRegU16(CommandDesignCapacity);
}
uint16_t BQ27220Component::getRemainingCapacity(void)
{
    return readRegU16(CommandRemainingCapacity);
}
uint16_t BQ27220Component::getStateOfCharge(void)
{
    return readRegU16(CommandStateOfCharge);
}
uint16_t BQ27220Component::getStateOfHealth(void)
{
    return readRegU16(CommandStateOfHealth);
}


void BQ27220Component::setup() {
  ESP_LOGI(TAG, "Initializing BQ27220");

  ESP_LOGI(TAG, "BQ27220 setup complete.");
}

  
void BQ27220Component::update() {

    // Publish Voltage 
    uint16_t voltage_mv = this->getVoltage(); 
    ESP_LOGD(TAG, "Voltage: %u mV", voltage_mv);
    if (voltage_sensor_ != nullptr) {
      voltage_sensor_->publish_state(voltage_mv / 1000.0f); 
    }
  
    // Publish Current 
    int16_t current_raw = this->getCurrent(); 
    ESP_LOGD(TAG, "Current: %d mA", current_raw);
    if (current_sensor_ != nullptr) {
      current_sensor_->publish_state(current_raw);
    } 
  
    // Publish SOC
    uint8_t soc = this->getStateOfCharge(); // UNTESTED - report back on results
    ESP_LOGD(TAG, "State of Charge: %u%%", soc);
    if (soc_sensor_ != nullptr) {
        soc_sensor_->publish_state(soc);
      }
      
    // Publish Remaining Capacity
    uint16_t remaining_mah = this->getRemainingCapacity();
    ESP_LOGD(TAG, "Remaining Capacity: %u mAh", remaining_mah);
    if (remaining_capacity_sensor_ != nullptr) {
        remaining_capacity_sensor_->publish_state(remaining_mah);
      }
    
    // Publish Temperature
    uint16_t raw_temp = this->getTemperature();
    ESP_LOGD(TAG, "RawTemperature: %u", raw_temp);
    float temperature = (raw_temp * 0.1f) - 273.15f;
    ESP_LOGD(TAG, "Temperature: %.2f °C", temperature);
    if (temperature_sensor_ != nullptr) {
        temperature_sensor_->publish_state(temperature);
      }
    
    // Publish Full Charge Capacity
    uint16_t fcc_mah = this->getFullChargeCapacity();
    ESP_LOGD(TAG, "Full Charge Capacity: %u mAh", fcc_mah);
    if (full_charge_capacity_sensor_ != nullptr) {
        full_charge_capacity_sensor_->publish_state(fcc_mah);
      }
      
    // Publish Design Capacity
    uint16_t dc_mah = this->getDesignCapacity();
    ESP_LOGD(TAG, "Design Capacity: %u mAh", dc_mah);
    if (design_capacity_sensor_ != nullptr) {
        design_capacity_sensor_->publish_state(dc_mah);
      }
    // Publish State of Health
    uint16_t soh = this->getStateOfHealth();
    ESP_LOGD(TAG, "State of Health: %u%%", soh);
    if (state_of_health_sensor_ != nullptr) {
        state_of_health_sensor_->publish_state(soh);
      }

    // Publish Device Number
    uint16_t devnum = this->getDeviceNumber();
    ESP_LOGD(TAG, "Device Number: 0x%04X", devnum);
    if (device_number_sensor_ != nullptr) {
        device_number_sensor_->publish_state(devnum);
      }

}

}  // namespace bq27220
}  // namespace esphome
