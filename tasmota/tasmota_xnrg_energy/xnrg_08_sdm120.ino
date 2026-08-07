/*
  xnrg_08_sdm120.ino - Eastron SDM120-Modbus energy meter support for Tasmota

  Copyright (C) 2026  Gennaro Tortone, Theo Arends, Marius Bezuidenhout

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_ENERGY_SENSOR
#ifdef USE_SDM120
/*********************************************************************************************\
 * Eastron SDM120 or SDM220 Modbus energy meter
 *
 * Based on: https://github.com/reaper7/SDM_Energy_Meter
\*********************************************************************************************/

#define XNRG_08             8

// can be user defined in my_user_config.h
#ifndef SDM120_SPEED
  #define SDM120_SPEED      2400    // default SDM120 baud rate: 2400, default SDM220 baud rate: 9600
#endif
// can be user defined in my_user_config.h
#ifndef SDM120_ADDR
  #define SDM120_ADDR       1       // default SDM120/SDM220 Modbus address
#endif

#if SDM120_SPEED < 9600
  #warning **** SDM120: Baud rates below 9600 may cause watchdog restarts ****
#endif

#include <TasmotaModbus.h>
TasmotaModbus *Sdm120Modbus = nullptr;
struct SDM120Block {
  uint16_t start;
  uint16_t count;
};

const SDM120Block sdm120_blocks[] = {
  { 0x0000, 0x000B },  // Voltage and Current 12 registers
  { 0x000C, 0x001A },  // Active power through phase angle 26 registers
  { 0x0046, 0x001A },  // frequency through demand values 26 registers
  { 0x0102, 0x0008 },  // current demands 8 registers
  { 0x0156, 0x0004 }   // total energy 4 registers
};

constexpr uint8_t SDM120_BLOCK_COUNT = sizeof(sdm120_blocks) / sizeof(SDM120Block);
constexpr uint16_t SDM120_MAX_BLOCK_REGISTERS = 26;
constexpr uint16_t SDM120_MAX_RESPONSE = 5 + (SDM120_MAX_BLOCK_REGISTERS * 2);
uint8_t sdm120_buffer[SDM120_MAX_RESPONSE] = { 0 };

const uint8_t device_unknown = 0;
const uint8_t is_sdm120 = 1;
const uint8_t is_sdm220 = 2;

struct SDM120 {
  float import_active = 0;
  float import_reactive = 0;
  float export_reactive = 0;
  float phase_angle = 0;
  float maximum_import_power_demand = 0;
  float input_power_demand = 0;
  float total_reactive = 0;
  float total_power_demand = 0;
  float maximum_power_demand = 0;
  float export_power_demand = 0;
  float maximum_export_power_demand = 0;
  float current_demand = 0;
  float maximum_current_demand = 0;
  uint8_t block_state = 0;
  uint8_t send_retry = 0;
  uint8_t sdm_120_220 = 0;
  uint8_t first_run = true;
} Sdm120;

/*********************************************************************************************/

float SDM120GetFloat(const uint8_t *buffer, uint16_t block_start, uint16_t register_address)
{
  uint16_t register_offset = register_address - block_start;
  uint16_t byte_offset = 3 + (register_offset * 2);
  
  float value;
  
  ((uint8_t*)&value)[3] = buffer[byte_offset + 0];
  ((uint8_t*)&value)[2] = buffer[byte_offset + 1];
  ((uint8_t*)&value)[1] = buffer[byte_offset + 2];
  ((uint8_t*)&value)[0] = buffer[byte_offset + 3];

  return value;
}

void SDM120DecodeBlock(uint8_t block,
                       const uint8_t *buffer)
{
  const uint16_t start = sdm120_blocks[block].start;

  float value;

  switch (block) {
    // --------------------------------------------------
    // 0x0000 -> 0x000B
    // --------------------------------------------------
    case 0:
      value = SDM120GetFloat(buffer, start, 0x0000);
      if (value >= 0.0f && value <= 500.0f) {
        Energy->voltage[0] = value;
      }

      Energy->current[0] =
        SDM120GetFloat(buffer, start, 0x0006);
      break;

    // --------------------------------------------------
    // 0x000C -> 0x0025
    // --------------------------------------------------
    case 1:
      Energy->active_power[0] =
        SDM120GetFloat(buffer, start, 0x000C);

      Energy->apparent_power[0] =
        SDM120GetFloat(buffer, start, 0x0012);

      Energy->reactive_power[0] =
        SDM120GetFloat(buffer, start, 0x0018);


      value = SDM120GetFloat(buffer, start, 0x001E);
      if (value >= -1.0f && value <= 1.0f) {
        Energy->power_factor[0] = value;
      }

      // SDM220 phase angle
      value = SDM120GetFloat(buffer, start, 0x0024);
      if (value >= -90.0f && value <= 90.0f) {
        Sdm120.phase_angle = value;
      }
      break;

    // --------------------------------------------------
    // 0x0046 -> 0x005F
    // --------------------------------------------------
    case 2:
      value = SDM120GetFloat(buffer, start, 0x0046);
      if (value >= 0.0f && value <= 100.0f) {
        Energy->frequency[0] = value;
      }

      Sdm120.import_active =
        SDM120GetFloat(buffer, start, 0x0048);

      Energy->export_active[0] =
        SDM120GetFloat(buffer, start, 0x004A);

      Sdm120.import_reactive =
        SDM120GetFloat(buffer, start, 0x004C);

      Sdm120.export_reactive =
        SDM120GetFloat(buffer, start, 0x004E);

      Sdm120.total_power_demand =
        SDM120GetFloat(buffer, start, 0x0054);

      Sdm120.maximum_power_demand =
        SDM120GetFloat(buffer, start, 0x0056);

      Sdm120.input_power_demand =
        SDM120GetFloat(buffer, start, 0x0058);

      Sdm120.maximum_import_power_demand =
        SDM120GetFloat(buffer, start, 0x005A);

      Sdm120.export_power_demand =
        SDM120GetFloat(buffer, start, 0x005C);

      Sdm120.maximum_export_power_demand =
        SDM120GetFloat(buffer, start, 0x005E);

      break;


    // --------------------------------------------------
    // 0x0102 -> 0x0109
    // --------------------------------------------------
    case 3:

      Sdm120.current_demand =
        SDM120GetFloat(buffer, start, 0x0102);

      Sdm120.maximum_current_demand =
        SDM120GetFloat(buffer, start, 0x0108);

      break;


    // --------------------------------------------------
    // 0x0156 -> 0x0159
    // --------------------------------------------------
    case 4:

      Energy->import_active[0] =
        SDM120GetFloat(buffer, start, 0x0156);

      Sdm120.total_reactive =
        SDM120GetFloat(buffer, start, 0x0158);

      EnergyUpdateTotal();

      break;
  }
}

void SDM120Every250ms(void)
{
  if (nullptr == Sdm120Modbus || nullptr == Energy) {
    return;
  }

  bool data_ready = Sdm120Modbus->ReceiveReady();

  if (data_ready) {

    const SDM120Block &block =
        sdm120_blocks[Sdm120.block_state];

    uint32_t error =
        Sdm120Modbus->ReceiveBuffer(
            sdm120_buffer,
            block.count
        );

    if (error) {

      AddLog(
        LOG_LEVEL_DEBUG,
        PSTR("SDM: Modbus error %u from block 0x%04X (%u registers)"),
        error,
        block.start,
        block.count
      );

    } else {

      const uint16_t received =
          Sdm120Modbus->ReceiveCount();

      AddLogBuffer(
        LOG_LEVEL_DEBUG_MORE,
        sdm120_buffer,
        received
      );

      const uint16_t expected =
          5 + (block.count * 2);

      if (received < expected) {

        AddLog(
          LOG_LEVEL_DEBUG,
          PSTR("SDM: Invalid response length %u, expected %u"),
          received,
          expected
        );

      } else {

        Energy->data_valid[0] = 0;

        SDM120DecodeBlock(
          Sdm120.block_state,
          sdm120_buffer
        );

        Sdm120.block_state++;

        if (Sdm120.block_state >= SDM120_BLOCK_COUNT) {
          Sdm120.block_state = 0;

          /*
           * Device detection can happen here because
           * an entire measurement cycle has completed.
           */
          if (Sdm120.sdm_120_220 == device_unknown) {

            if ((Sdm120.phase_angle == 0) &&
                (Sdm120.maximum_import_power_demand > 0)) {

              Sdm120.sdm_120_220 = is_sdm120;

              AddLog(
                LOG_LEVEL_INFO,
                PSTR("SDM: Device determined to be SDM120")
              );

            } else {

              Sdm120.sdm_120_220 = is_sdm220;

              AddLog(
                LOG_LEVEL_INFO,
                PSTR("SDM: Device determined to be SDM220")
              );
            }
          }
        }
      }
    }
  }

  /*
   * Send next block request
   */
  if ((0 == Sdm120.send_retry) || data_ready) {

    Sdm120.send_retry = 5;

    const SDM120Block &block =
        sdm120_blocks[Sdm120.block_state];

    Sdm120Modbus->Send(
      SDM120_ADDR,
      0x04,
      block.start,
      block.count
    );

  } else {
    Sdm120.send_retry--;
  }
}

void Sdm120SnsInit(void)
{
  delete Sdm120Modbus;
  Sdm120Modbus = nullptr;

  Sdm120Modbus = new (std::nothrow) TasmotaModbus(Pin(GPIO_SDM120_RX), Pin(GPIO_SDM120_TX), Pin(GPIO_NRG_MBS_TX_ENA), Pin(GPIO_MBS_RX_ENA));
  if (nullptr == Sdm120Modbus) {
    AddLog(LOG_LEVEL_ERROR, PSTR("SDM: Modbus allocation failed"));
    TasmotaGlobal.energy_driver = ENERGY_NONE;
    return;
  }

  uint8_t result = Sdm120Modbus->Begin(SDM120_SPEED);
  if (result) {
    if (2 == result) { ClaimSerial(); }
#ifdef ESP32
    AddLog(LOG_LEVEL_DEBUG, PSTR("SDM: Serial UART%d"), Sdm120Modbus->getUart());
#endif
  } else {
    delete Sdm120Modbus;
    Sdm120Modbus = nullptr;
    TasmotaGlobal.energy_driver = ENERGY_NONE;
  }
}

void Sdm120DrvInit(void)
{
  if (PinUsed(GPIO_SDM120_RX) && PinUsed(GPIO_SDM120_TX)) {
    TasmotaGlobal.energy_driver = XNRG_08;
  }
}

void Sdm220Reset(void)
{
  if (Sdm120.sdm_120_220 != is_sdm220) { return; }

  Sdm120.phase_angle = Sdm120.import_active = Sdm120.import_reactive = Sdm120.export_reactive = 0;
}

void Sdm220Show(bool json) {
  if (json) {
    ResponseAppend_P(PSTR(",\"" D_JSON_IMPORT_ACTIVE "\":%s"), EnergyFmt(&Sdm120.import_active, Settings->flag2.energy_resolution));
    ResponseAppend_P(PSTR(",\"" D_JSON_IMPORT_REACTIVE "\":%s"), EnergyFmt(&Sdm120.import_reactive, Settings->flag2.energy_resolution));
    ResponseAppend_P(PSTR(",\"" D_JSON_EXPORT_REACTIVE "\":%s"), EnergyFmt(&Sdm120.export_reactive, Settings->flag2.energy_resolution));
    if (Sdm120.sdm_120_220 != is_sdm220) {
      ResponseAppend_P(PSTR(",\"" D_JSON_PHASE_ANGLE "\":%s"), EnergyFmt(&Sdm120.phase_angle, 2));
    }
#ifdef USE_WEBSERVER
  } else {
    // SDM120 / SDM220
    WSContentSend_PD(HTTP_SNS_IMPORT_REACTIVE, WebEnergyFmt(&Sdm120.import_reactive, Settings->flag2.energy_resolution, 2));
    WSContentSend_PD(HTTP_SNS_EXPORT_REACTIVE, WebEnergyFmt(&Sdm120.export_reactive, Settings->flag2.energy_resolution, 2));
    WSContentSend_PD(HTTP_SNS_TOTAL_REACTIVE, WebEnergyFmt(&Sdm120.total_reactive, Settings->flag2.energy_resolution));
    WSContentSend_PD(HTTP_SNS_MAX_POWER, WebEnergyFmt(&Sdm120.maximum_power_demand, Settings->flag2.wattage_resolution));

    // SDM220
    if (Sdm120.sdm_120_220 == is_sdm220) {
      WSContentSend_PD(HTTP_SNS_PHASE_ANGLE, WebEnergyFmt(&Sdm120.phase_angle, 2));
    }
#endif  // USE_WEBSERVER
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xnrg08(uint32_t function)
{
  bool result = false;

  switch (function) {
    case FUNC_EVERY_250_MSECOND:
      SDM120Every250ms();
      break;
    case FUNC_JSON_APPEND:
      Sdm220Show(1);
      break;
#ifdef USE_WEBSERVER
    case FUNC_WEB_COL_SENSOR:
      Sdm220Show(0);
      break;
#endif  // USE_WEBSERVER
    case FUNC_ENERGY_RESET:
      Sdm220Reset();
      break;
    case FUNC_INIT:
      Sdm120SnsInit();
      break;
    case FUNC_PRE_INIT:
      Sdm120DrvInit();
      break;
  }
  return result;
}

#endif  // USE_SDM120
#endif  // USE_ENERGY_SENSOR
