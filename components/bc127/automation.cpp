#include "automation.h"
#include "bc127.h"

namespace esphome
{
    namespace bc127
    {

        BC127ConnectedTrigger::BC127ConnectedTrigger(BC127Component *obj)
        {
            std::vector<uint8_t> bytes = obj->bytes;
            obj->add_on_connected_callback([this,bytes]()
                                           { this->trigger(bytes); });
        }
        IncomingCallTrigger::IncomingCallTrigger(BC127Component *obj)
        {
            // std::vector<uint8_t> bytes = obj->bytes;
            std::string bytes = obj->callerId;
            obj->add_on_call_callback([this,bytes]()
                                           { this->trigger(bytes); });
        }
        
    } // namespace bc127
} // namespace esphome
