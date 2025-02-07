#pragma once

#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome.h"
#include <ArduinoJson.h>
#include "automation.h"
#include "phoneContactManager.h"
#include <string>  // Ensure this is included

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
  // Member data
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



  void set_whitelist_enabled(bool enabled) { this->whitelist_enabled_ = enabled; }
  void set_ringing_enabled(bool enabled) { this->ringing_enabled_ = enabled; }

  // Check if BC127 is in "connected" state
  bool is_connected() const {
    return (this->state == BC127_CONNECTED);
  }

  // Ring tone control
  void call_notify_incoming_call();  // Sends TONE command once
  void start_call_ring();           // Flag to periodically re-trigger ring
  void stop_call_ring();            // Stop re-triggering

  // Volume control
  void volume_up();                 // VOLUME <linkID> UP
  void volume_down();               // VOLUME <linkID> DOWN

 protected:
  int onetime;
  int state = 0;
  int whitelist_enabled_ = true;
  int ringing_enabled_ = true;

  std::string ble_phone_address = "";
  std::string hfp_connection_id = "";

  void process_data(const String &data);
  void send_command(const std::string &command);
  void set_state(int state);

  CallbackManager<void()> on_connected_callbacks;
  CallbackManager<void()> on_call_callbacks;
  CallbackManager<void()> on_ended_call_callbacks;

  // Manage phone contacts
  PhoneContactManager phoneContactManager;

  // Ringing flag
  bool ringing_{false};
};

// Expose the global pointer if needed
extern BC127Component *controller;

}  // namespace bc127
}  // namespace esphome
