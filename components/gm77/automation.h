#pragma once

#include <memory>
#include "esphome/core/automation.h"
#include "esphome/core/optional.h"

namespace esphome
{
    namespace gm77
    {
        class GM77Component; // Declaración adelantada

        class OnTagTrigger : public Trigger<std::string>
        {
        public:
            explicit OnTagTrigger(GM77Component *obj);
        };

    } // namespace gm77
} // namespace esphome
