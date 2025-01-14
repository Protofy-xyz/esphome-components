#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include "bc127.h"

namespace esphome {
  namespace bc127 {

    static const char *const TAG = "bc127";

    BC127Component::BC127Component() {
      // Constructor (empty or initialization as needed)
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

      // Reset the module
      this->send_command("RESET");
      ESP_LOGCONFIG(TAG, "Resetting module");
    }

    void BC127Component::loop() {
      // Check if data is available from BC127
      if (this->available()) {
        String received_data = "";

        // Read the UART data
        while (this->available()) {
          char c = this->read();
          received_data += c;
          // If we detect carriage return, process the command
          if (c == '\r') {
            ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
            this->process_data(received_data);
            received_data = "";  // Reset for the next message
          }
        }
      }

      if (this->state == BC127_NOT_READY) {
        ESP_LOGD(TAG, "Not ready");
        delay(500);
        return;
      }

      // Transition from NOT_READY -> READY -> START_PAIRING
      if (this->state == BC127_READY) {
        this->send_command("BT_STATE ON ON");
        ESP_LOGD(TAG, "Starting pairing");
        this->set_state(BC127_START_PAIRING);
        return;
      }

      // Run once after startup
      if (this->onetime == 0) {
        this->set_onetime(1);
        ESP_LOGD(TAG, "LOOP one time");

        // Example connected callback
        this->add_on_connected_callback([this]() {
          ESP_LOGD(TAG, "ADD ON CONNECTED CALLBACK");
        });
        auto &callbacks = on_connected_callbacks;
        std::vector<uint8_t> bytes = {'a', 'b'};
        callbacks.call();
      }
    }

    void BC127Component::process_data(const String &data) {
      // "Ready"
      if (data.startsWith("Ready")) {
        ESP_LOGI(TAG, "Ready");
        this->set_state(BC127_READY);
        return;
      }

      // "OPEN_OK 13 HFP 0CC6FD08CCAF"
      if (data.startsWith("OPEN_OK")) {
        ESP_LOGD(TAG, "OPEN_OK command received");
        int first_space = data.indexOf(' ');
        int second_space = data.indexOf(' ', first_space + 1);
        int third_space = data.indexOf(' ', second_space + 1);

        if (first_space != -1 && second_space != -1 && third_space != -1) {
          String var1 = data.substring(first_space + 1, second_space); // "13"
          String var2 = data.substring(second_space + 1, third_space); // "HFP"
          String var3 = data.substring(third_space + 1);               // "0CC6FD08CCAF"
          ESP_LOGD(TAG, "Parsed values: var1=%s, var2=%s, var3=%s", var1.c_str(), var2.c_str(), var3.c_str());

          if (var2.startsWith("HFP")) {
            this->hfp_connection_id = var1;
            this->ble_phone_address = var3;
            ESP_LOGI(TAG, "HFP connection id: %s", this->hfp_connection_id.c_str());
            ESP_LOGI(TAG, "BLE phone address: %s", this->ble_phone_address.c_str());
            this->set_state(BC127_CONNECTED);
          }
        } else {
          ESP_LOGW(TAG, "Invalid OPEN_OK command format");
        }
        return;
      }

      // "CALLER_NUMBER 13 000000000"
      if (data.startsWith("CALLER_NUMBER")) {
        int first_space = data.indexOf(' ');
        int second_space = data.indexOf(' ', first_space + 1);

        if (first_space != -1 && second_space != -1) {
          String id = data.substring(first_space + 1, second_space);
          String phone_number = data.substring(second_space + 1, data.length() - 1);

          ESP_LOGI(TAG, "Parsed CALLER_NUMBER command");
          ESP_LOGI(TAG, "ID: %s", id.c_str());
          ESP_LOGI(TAG, "Phone Number: %s", phone_number.c_str());

          // If contact not in list => block the call
          if (this->phoneContactManager.find_contact_by_number(phone_number.c_str()) == nullptr) {
            this->set_state(BC127_CALL_BLOCKED);
            ESP_LOGI(TAG, "Call rejected because the phone number is not in the contact list");
            this->call_reject();
            return;
          }

          // Otherwise, it's a valid incoming call
          this->set_state(BC127_INCOMING_CALL);
          this->callerId = this->phoneContactManager.find_contact_by_number(phone_number.c_str())->to_string();
          ESP_LOGI(TAG, "Caller ID: %s", this->callerId.c_str());

          // Trigger the "on_call" callbacks
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

      // "CLOSE_OK 13 HFP 0CC6FD08CCAF"
      if (data.startsWith("CLOSE_OK")) {
        ESP_LOGD(TAG, "CLOSE_OK command received");
        int first_space = data.indexOf(' ');
        int second_space = data.indexOf(' ', first_space + 1);
        int third_space = data.indexOf(' ', second_space + 1);

        if (first_space != -1 && second_space != -1 && third_space != -1) {
          String var1 = data.substring(first_space + 1, second_space); // "13"
          String var2 = data.substring(second_space + 1, third_space); // "HFP"
          String var3 = data.substring(third_space + 1);               // "0CC6FD08CCAF"
          ESP_LOGD(TAG, "Parsed values: var1=%s, var2=%s, var3=%s", var1.c_str(), var2.c_str(), var3.c_str());

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

      // "CALL_END"
      if (data.startsWith("CALL_END")) {
        ESP_LOGI(TAG, "Parsed CALL_END command");
        this->call_end();            // Attempt to end the call
        this->set_state(BC127_CONNECTED);
        ESP_LOGI(TAG, "After call end");
        return;
      }

      // "CALL_OUTGOING"
      if (data.startsWith("CALL_OUTGOING")) {
        ESP_LOGI(TAG, "Parsed CALL_OUTGOING command");
        this->set_state(BC127_CALL_OUTGOING);
        return;
      }

      ESP_LOGW(TAG, "Unknown command received: %s", data.c_str());
    }

    void BC127Component::call_dial(const char *phone_number) {
      // Dial out if connected
      if (this->state == BC127_CONNECTED) {
        // Must be in the contact list
        if (this->phoneContactManager.find_contact_by_number(phone_number) == nullptr) {
          ESP_LOGI(TAG, "Not possible to call this phone number because it's not in the contact list");
          return;
        }

        this->callerId = this->phoneContactManager.find_contact_by_number(phone_number)->get_number();
        ESP_LOGI(TAG, "Caller ID: %s", this->callerId.c_str());

        this->send_command(std::string("CALL ") + std::string(this->hfp_connection_id.c_str()) + 
                           " OUTGOING " + phone_number);
      }
    }

    void BC127Component::call_answer() {
      if (this->state == BC127_INCOMING_CALL) {
        // Send "answer" command to BC127
        this->send_command(std::string("CALL ") + std::string(this->hfp_connection_id.c_str()) + " ANSWER");
        ESP_LOGI(TAG, "Answering call");
        this->set_state(BC127_CALL_IN_COURSE);
      } else {
        ESP_LOGW(TAG, "No incoming call to answer");
      }
    }

    //=================================================
    // Reject an incoming call => Immediately fire on_ended_call callbacks
    //=================================================
    void BC127Component::call_reject() {
      if ((this->state == BC127_INCOMING_CALL) || (this->state == BC127_CALL_BLOCKED)) {
        // Send "reject" command to BC127
        this->send_command(std::string("CALL ") + std::string(this->hfp_connection_id.c_str()) + " REJECT");
        ESP_LOGI(TAG, "Rejecting call");

        // ===== IMMEDIATE "call ended" notification =====
        // Instead of calling call_end(), we directly fire on_ended_call callbacks here.
        this->add_on_ended_call_callback([this]() {
          ESP_LOGD(TAG, "ADD ON ENDED CALL CALLBACK (Reject path)");
        });
        auto &callbacks = on_ended_call_callbacks;
        callbacks.call();

        // Return to "Connected" state
        this->set_state(BC127_CONNECTED);
      } else {
        ESP_LOGW(TAG, "No incoming call to reject");
      }
    }

    //=================================================
    // End a call in course or outgoing call
    //=================================================
    void BC127Component::call_end() {
      // Only send the "END" command if we are actually in a call
      if (this->state == BC127_CALL_IN_COURSE || (this->state == BC127_CALL_OUTGOING)) {
        this->send_command(std::string("CALL ") + std::string(this->hfp_connection_id.c_str()) + " END");
        ESP_LOGI(TAG, "Ending call");

        this->add_on_ended_call_callback([this]() {
          ESP_LOGD(TAG, "ADD ON ENDED CALL CALLBACK");
        });
        auto &callbacks = on_ended_call_callbacks;
        callbacks.call();

        this->set_state(BC127_CONNECTED);
      } else {
        ESP_LOGW(TAG, "No incoming/outgoing call to end");
      }
    }

    //=================================================
    // Manage phone contacts
    //=================================================
    void BC127Component::add_phone_contact(const char *name, const char *number) {
      this->phoneContactManager.add_contact(PhoneContact(name, number));
      ESP_LOGI(TAG, "Adding contact: name: %s, number: %s", name, number);
    }

    void BC127Component::remove_phone_contact(const char *name, const char *number) {
      this->phoneContactManager.remove_contact(PhoneContact(name, number));
      ESP_LOGI(TAG, "Removing contact: name: %s, number: %s", name, number);
    }

    std::vector<std::string> BC127Component::get_contacts() {
      if (!this->phoneContactManager.get_contacts().empty()) {
        const std::vector<PhoneContact> &contacts = this->phoneContactManager.get_contacts();
        std::vector<std::string> contacts_str;
        for (const auto &contact : contacts) {
          contacts_str.push_back(contact.to_string());
        }
        return contacts_str;
      }
      return {};
    }

    //=================================================
    // Low-level send command over UART
    //=================================================
    void BC127Component::send_command(const std::string &command) {
      ESP_LOGD(TAG, "Sending command: %s", command.c_str());
      this->write_str(command.c_str());  // send command via UART
      this->write_str("\r");            // append carriage return
    }

    //=================================================
    // Callback registrations
    //=================================================
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

    //=================================================
    // Set internal state
    //=================================================
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
        case BC127_CALL_OUTGOING:
          ESP_LOGI(TAG, "Setting state: Outgoing call in course");
          break;
        default:
          ESP_LOGI(TAG, "Setting state: Unknown state");
          break;
      }
    }

    BC127Component *controller = nullptr;

  }  // namespace bc127
}  // namespace esphome
