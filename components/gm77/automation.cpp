#include "automation.h"
#include "gm77.h"

namespace esphome
{
    namespace gm77
    {

         OnTagTrigger::OnTagTrigger(GM77Component *obj)
        {
            std::string bytes = obj->tag;
            obj->add_on_tag_callback([this,bytes]()
                                           { this->trigger(bytes); });
        }
        
    } // namespace gm77
} // namespace esphome
