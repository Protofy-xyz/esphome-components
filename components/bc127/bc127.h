#pragma once

#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome.h"
#include <ArduinoJson.h>
#include "automation.h"
#include "phoneContactManager.h"

#define BC127_NOT_READY 0
#define BC127_READY 1
#define BC127_START_PAIRING 2
#define BC127_CONNECTED 3
#define BC127_INCOMING_CALL 4
#define BC127_CALL_IN_COURSE 5
#define BC127_CALL_BLOCKED 6
#define BC127_CALL_OUTGOING 7

namespace esphome {
namespace bc127 {

class BC127Component : public Component, public uart::UARTDevice {
 public:
  // Some example member data (just as before)
  std::vector<uint8_t> bytes = {'a', 'b', 'c'};
  std::string callerId;

  BC127Component();
  virtual ~BC127Component() {}

  // Standard overrides
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Callbacks
  void add_on_connected_callback(std::function<void()> &&trigger_function);
  void add_on_call_callback(std::function<void()> &&trigger_function);
  void add_on_ended_call_callback(std::function<void()> &&trigger_function);

  // Telephony methods
  void call_dial(const char *phone_number);
  void call_answer();
  void call_reject();
  void call_end();

  // Contact list management
  void add_phone_contact(const char *name, const char *number);
  void remove_phone_contact(const char *name, const char *number);
  std::vector<std::string> get_contacts();

  // One-time logic
  void set_onetime(int val) { this->onetime = val; }
  int get_onetime() { return this->onetime; }

  // Locking
  void set_locked(bool locked);

  // ------------------------------------------------------------------
  // New helper to check if BC127 is in the "connected" state
  // ------------------------------------------------------------------
  bool is_connected() const {
    return (this->state == BC127_CONNECTED);
  }

 protected:
  int onetime;
  int state = 0;

  String ble_phone_address = "";
  String hfp_connection_id = "";

  void process_data(const String &data);
  void send_command(const std::string &command);
  void set_state(int state);

  CallbackManager<void()> on_connected_callbacks;
  CallbackManager<void()> on_call_callbacks;
  CallbackManager<void()> on_ended_call_callbacks;

  // Manage phone contacts
  PhoneContactManager phoneContactManager;

  // Internal locked/unlocked
  bool locked_{false};
};

// Expose the global pointer if needed
extern BC127Component *controller;

}  // namespace bc127
}  // namespace esphome
