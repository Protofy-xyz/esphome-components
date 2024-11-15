#pragma once

#include <memory>
#include "esphome/core/automation.h"
#include "esphome/core/optional.h"

namespace esphome
{
    namespace bc127
    {
        class BC127Component; // Declaración adelantada

        class BC127ConnectedTrigger : public Trigger<std::vector<uint8_t>>
        {
        public:
            explicit BC127ConnectedTrigger(BC127Component *obj);
        };

        class IncomingCallTrigger : public Trigger<std::string>
        {
        public:
            explicit IncomingCallTrigger(BC127Component *obj);
        };

        class EndedCallTrigger : public Trigger<std::string>
        {
        public:
            explicit EndedCallTrigger(BC127Component *obj);
        };
    } // namespace bc127
} // namespace esphome
