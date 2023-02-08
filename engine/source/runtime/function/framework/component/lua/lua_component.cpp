
#include "runtime/function/framework/component/lua/lua_component.h"

namespace Piccolo
{
    void LuaComponent::postLoadResource(std::weak_ptr<GObject> parent_gobject)
    {
        m_parent_object       = parent_gobject;
        m_lua_state.open_libraries(sol::lib::base);
    }

    void LuaComponent::tick(float delta_time)
    {
        //LOG_INFO(m_lua_script)
        m_lua_state.script(m_lua_script);
    }

} // namespace Piccolo