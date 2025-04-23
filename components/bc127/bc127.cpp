#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include "bc127.h"

namespace esphome {
namespace bc127 {

static const char *const TAG = "bc127";

BC127Component *controller = nullptr;

BC127Component::BC127Component() {
}

void BC127Component::call_notify_incoming_call() {
  // Maximum volume with slight decay for better audibility
  this->send_command("TONE TE 120 TI 0 V 255 D 20 N C4 L 8 N E4 L 8 N G4 L 8");
}

void BC127Component::start_call_ring() {
  this->ringing_ = true;
}

void BC127Component::stop_call_ring() {
  this->ringing_ = false;
}

void BC127Component::volume_up() {
  if (!this->hfp_connection_id.empty()) {
    this->send_command("VOLUME " + this->hfp_connection_id + " UP");
  } else {
    ESP_LOGW(TAG, "No HFP connection ID to send volume up");
  }
}

void BC127Component::volume_down() {
  if (!this->hfp_connection_id.empty()) {
    this->send_command("VOLUME " + this->hfp_connection_id + " DOWN");
  } else {
    ESP_LOGW(TAG, "No HFP connection ID to send volume down");
  }
}

void BC127Component::setup() {
  this->phoneContactManager = PhoneContactManager();
  this->set_onetime(0);
  ESP_LOGCONFIG(TAG, "Setting up module");
  if (controller == nullptr) {
    controller = this;
  } else {
    ESP_LOGE(TAG, "Already have an instance of the BC127");
  }
  this->send_command("RESET");
  ESP_LOGCONFIG(TAG, "Resetting module");
}

void BC127Component::loop() {
  if (this->available()) {
    String received_data = "";

    while (this->available()) {
      char c = this->read();
      received_data += c;
      if (c == '\r') {
        ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
        this->process_data(received_data);
        received_data = "";
      }
    }
  }

  if (this->state == BC127_NOT_READY) {
    ESP_LOGD(TAG, "Not ready");
    delay(500);
    return;
  }

  if (this->state == BC127_READY) {
    this->send_command("BT_STATE ON ON");
    ESP_LOGD(TAG, "Starting pairing");
    this->set_state(BC127_START_PAIRING);
    return;
  }

  // "one-time" logic example
  if (this->onetime == 0) {
    this->set_onetime(1);
    ESP_LOGD(TAG, "LOOP one time");
    this->add_on_connected_callback([this]() {
      ESP_LOGD(TAG, "ADD ON CONNECTED CALLBACK");
    });
    auto &callbacks = on_connected_callbacks;
    std::vector<uint8_t> bytes = {'a', 'b'};
    callbacks.call();
  }

  // Periodically re-play ring while incoming call and ringing is active
  static uint32_t last_ring_millis = 0;
  if (this->state == BC127_INCOMING_CALL && this->ringing_) {
    if (millis() - last_ring_millis > 2000) {
      last_ring_millis = millis();
      this->call_notify_incoming_call();
    }
  }
}

void BC127Component::process_data(const String &data) {
  // Handle 'Ready' state
  if (data.startsWith("Ready")) {
    ESP_LOGI(TAG, "Ready");
    this->set_state(BC127_READY);
    return;
  }

  // Handle 'OK' acknowledgment
  if (data.startsWith("OK")) {
    ESP_LOGD(TAG, "Received OK from BC127");
    // Optionally, track which command was acknowledged
    return;
  }

  // Handle 'OPEN_OK' command
  if (data.startsWith("OPEN_OK")) {
    ESP_LOGD(TAG, "OPEN_OK command received");

    int first_space = data.indexOf(' ');
    int second_space = data.indexOf(' ', first_space + 1);
    int third_space = data.indexOf(' ', second_space + 1);
    ESP_LOGD(TAG, "Spaces: %d, %d, %d", first_space, second_space, third_space);

    if (first_space != -1 && second_space != -1 && third_space != -1) {
      String var1 = data.substring(first_space + 1, second_space); // e.g., "13"
      String var2 = data.substring(second_space + 1, third_space); // e.g., "HFP"
      String var3 = data.substring(third_space + 1);               // e.g., "0CC6FD08CCAF"

      ESP_LOGD(TAG, "Parsed values: var1=%s, var2=%s, var3=%s",
               var1.c_str(), var2.c_str(), var3.c_str());

      if (var2.startsWith("HFP")) {
        this->hfp_connection_id = std::string(var1.c_str()); 
        this->ble_phone_address = std::string(var3.c_str());  
        ESP_LOGI(TAG, "HFP connection id: %s", this->hfp_connection_id.c_str());
        ESP_LOGI(TAG, "BLE phone address: %s", this->ble_phone_address.c_str());
        this->set_state(BC127_CONNECTED);
      }
    } else {
      ESP_LOGW(TAG, "Invalid OPEN_OK command format");
    }
    return;
  }

  // Handle 'CALLER_NUMBER' command
  if (data.startsWith("CALLER_NUMBER")) {
    int first_space = data.indexOf(' ');
    int second_space = data.indexOf(' ', first_space + 1);

    if (first_space != -1 && second_space != -1) {
      String id = data.substring(first_space + 1, second_space);
      String phone_number = data.substring(second_space + 1, data.length() - 1);

      ESP_LOGI(TAG, "Parsed CALLER_NUMBER command");
      ESP_LOGI(TAG, "ID: %s", id.c_str());
      ESP_LOGI(TAG, "Phone Number: %s", phone_number.c_str());

    
      bool is_whitelisted = (this->phoneContactManager.find_contact_by_number(phone_number.c_str()) != nullptr);
      
      ESP_LOGD(TAG, "whitelist_enabled_: %d", this->whitelist_enabled_);
      ESP_LOGD(TAG, "is_whitelisted: %d", is_whitelisted);

        if (this->whitelist_enabled_ && !is_whitelisted) {
          this->set_state(BC127_CALL_BLOCKED);
          ESP_LOGI(TAG, "Call rejected: number not in contact list and device locked");
          this->call_reject();
          return;
        } else {
          this->set_state(BC127_INCOMING_CALL);
          if(this->ringing_enabled_ == true){
            this->start_call_ring();
          }
          ESP_LOGI(TAG, "Incoming call from number: %s", phone_number.c_str());
        }

      // Set callerId
      PhoneContact *c = this->phoneContactManager.find_contact_by_number(phone_number.c_str());
      if (c != nullptr) {
        this->callerId = c->to_string();
      } else {
        this->callerId = std::string("Unknown:") + phone_number.c_str();
      }

      ESP_LOGI(TAG, "Caller ID: %s", this->callerId.c_str());
      this->add_on_call_callback([this]() {
        ESP_LOGD(TAG, "ADD ON CALL CALLBACK");
      });
      auto &callbacks = on_call_callbacks;
      callbacks.call();
    } else {
      ESP_LOGW(TAG, "Invalid CALLER_NUMBER command format");
    }
    return;
  }

  // Handle 'CLOSE_OK' command
  if (data.startsWith("CLOSE_OK")) {
    ESP_LOGD(TAG, "CLOSE_OK command received");

    int first_space = data.indexOf(' ');
    int second_space = data.indexOf(' ', first_space + 1);
    int third_space = data.indexOf(' ', second_space + 1);
    ESP_LOGD(TAG, "Spaces: %d, %d, %d", first_space, second_space, third_space);

    if (first_space != -1 && second_space != -1 && third_space != -1) {
      String var1 = data.substring(first_space + 1, second_space); // e.g., "13"
      String var2 = data.substring(second_space + 1, third_space); // e.g., "HFP"
      String var3 = data.substring(third_space + 1);               // e.g., "0CC6FD08CCAF"

      ESP_LOGD(TAG, "Parsed values: var1=%s, var2=%s, var3=%s",
               var1.c_str(), var2.c_str(), var3.c_str());

      if (var2.startsWith("HFP")) {
        this->hfp_connection_id.clear();
        this->ble_phone_address.clear();
        ESP_LOGI(TAG, "HFP connection id cleared");
        ESP_LOGI(TAG, "BLE phone address cleared");
        this->set_state(BC127_READY);
      }
    } else {
      ESP_LOGW(TAG, "Invalid CLOSE_OK command format");
    }
    return;
  }

  // Handle 'CALL_END' command
  if (data.startsWith("CALL_END")) {
    ESP_LOGI(TAG, "Parsed CALL_END command");
    this->call_end();
    this->set_state(BC127_CONNECTED);
    ESP_LOGI(TAG, "After call end");
    return;
  }

  // Handle 'CALL_OUTGOING' command
  if (data.startsWith("CALL_OUTGOING")) {
    ESP_LOGI(TAG, "Parsed CALL_OUTGOING command");
    this->set_state(BC127_CALL_OUTGOING);
    return;
  }

  // Unknown command
  ESP_LOGW(TAG, "Unknown command received: %s", data.c_str());
}

void BC127Component::call_dial(const char *phone_number) {
  // Only dial if we're connected
  if (this->state == BC127_CONNECTED) {
    // If contact not in the list, skip
    if (this->phoneContactManager.find_contact_by_number(phone_number) == nullptr) {
      ESP_LOGI(TAG, "Not possible to call this phone number because it is not in the contact list");
      return;
    }
    this->callerId = this->phoneContactManager.find_contact_by_number(phone_number)->get_number();
    ESP_LOGI(TAG, "Caller ID: %s", this->callerId.c_str());

    // Send the call command
    this->send_command(std::string("CALL ") + this->hfp_connection_id + " OUTGOING " + phone_number);
  }
}

void BC127Component::call_answer() {
  if (this->state == BC127_INCOMING_CALL) {
    this->send_command(std::string("CALL ") + this->hfp_connection_id + " ANSWER");
    ESP_LOGI(TAG, "Answering call");
    // Stop ringing
    this->stop_call_ring();
    this->set_state(BC127_CALL_IN_COURSE);
  } else {
    ESP_LOGW(TAG, "No incoming call to answer");
  }
}

void BC127Component::call_reject() {
  if (this->state == BC127_INCOMING_CALL || this->state == BC127_CALL_BLOCKED) {
    this->send_command(std::string("CALL ") + this->hfp_connection_id + " REJECT");
    ESP_LOGI(TAG, "Rejecting call");
    // Stop ringing
    this->stop_call_ring();
    this->set_state(BC127_CONNECTED);
  } else {
    ESP_LOGW(TAG, "No incoming call to reject");
  }
}

void BC127Component::call_end() {
  if (this->state == BC127_INCOMING_CALL ||
      this->state == BC127_CALL_IN_COURSE ||
      this->state == BC127_CALL_OUTGOING) {
    this->send_command(std::string("CALL ") + this->hfp_connection_id + " END");
    ESP_LOGI(TAG, "Ending call");

    // Trigger on_ended_call
    this->add_on_ended_call_callback([this]() {
      ESP_LOGD(TAG, "ADD ON ENDED CALL CALLBACK");
    });
    auto &callbacks = on_ended_call_callbacks;
    callbacks.call();

    // Stop ringing if it was still incoming
    this->stop_call_ring();
    this->set_state(BC127_CONNECTED);
  } else {
    ESP_LOGW(TAG, "No call to end right now");
  }
}

void BC127Component::add_phone_contact(const char *name, const char *number) {
  this->phoneContactManager.add_contact(PhoneContact(name, number));
  ESP_LOGI(TAG, "Adding contact: name: %s, number: %s", name, number);
}

void BC127Component::remove_phone_contact(const char *name, const char *number) {
  this->phoneContactManager.remove_contact(PhoneContact(name, number));
  ESP_LOGI(TAG, "Removing contact: name: %s, number: %s", name, number);
}

void BC127Component::remove_all_phone_contacts() {
  this->phoneContactManager.clear_contacts();
  ESP_LOGI(TAG, "All contacts removed");
}

std::vector<std::string> BC127Component::get_contacts() {
  const auto &contacts = this->phoneContactManager.get_contacts();
  std::vector<std::string> contacts_str;
  for (const auto &contact : contacts) {
    contacts_str.push_back(contact.to_string());  // e.g. "Name:Number"
  }
  return contacts_str;
}

void BC127Component::send_command(const std::string &command) {
  ESP_LOGD(TAG, "Sending command: %s", command.c_str());
  this->write_str(command.c_str()); // Send command over UART
  this->write_str("\r");            // Add carriage return
}

void BC127Component::add_on_connected_callback(std::function<void()> &&trigger_function) {
  on_connected_callbacks.add(std::move(trigger_function));
}

void BC127Component::add_on_call_callback(std::function<void()> &&trigger_function) {
  on_call_callbacks.add(std::move(trigger_function));
}

void BC127Component::add_on_ended_call_callback(std::function<void()> &&trigger_function) {
  on_ended_call_callbacks.add(std::move(trigger_function));
}

void BC127Component::dump_config() {
  ESP_LOGCONFIG(TAG, "BC127 module:");
  ESP_LOGCONFIG(TAG, "  Configured for UART communication with BC127");
}

void BC127Component::set_state(int state) {
  this->state = state;
  switch (state) {
    case BC127_NOT_READY:
      ESP_LOGI(TAG, "Setting state: Not ready");
      break;
    case BC127_READY:
      ESP_LOGI(TAG, "Setting state: Ready");
      break;
    case BC127_START_PAIRING:
      ESP_LOGI(TAG, "Setting state: Starting pairing");
      break;
    case BC127_CONNECTED:
      ESP_LOGI(TAG, "Setting state: Connected");
      break;
    case BC127_INCOMING_CALL:
      ESP_LOGI(TAG, "Setting state: Incoming call");
      break;
    case BC127_CALL_IN_COURSE:
      ESP_LOGI(TAG, "Setting state: Call in course");
      break;
    case BC127_CALL_BLOCKED:
      ESP_LOGI(TAG, "Setting state: Call blocked");
      break;
    case BC127_CALL_OUTGOING:
      ESP_LOGI(TAG, "Setting state: Outgoing call in course");
      break;
    default:
      ESP_LOGI(TAG, "Setting state: Unknown");
      break;
  }
}


}  // namespace bc127
}  // namespace esphome
