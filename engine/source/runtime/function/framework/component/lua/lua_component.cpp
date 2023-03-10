#include "runtime/function/framework/component/lua/lua_component.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/object/object.h"

namespace Piccolo
{
    // 查询函数 查找component_field
    bool find_component_field(std::weak_ptr<GObject>     game_object,
                              const char*                field_name,
                              Reflection::FieldAccessor& field_accessor,
                              void*&                     target_instance)
    {
        auto components = game_object.lock()->getComponents();

        std::istringstream iss(field_name);
        std::string        current_name;
        std::getline(iss, current_name, '.');
        auto component_iter = std::find_if(
            components.begin(), components.end(), [current_name](auto c) { return c.getTypeName() == current_name; });
        // 找到对应的component
        if (component_iter != components.end())
        {
            // meta field_instance两个被while循环需要的迭代变量
            auto  meta           = Reflection::TypeMeta::newMetaFromName(current_name);
            void* field_instance = component_iter->getPtr();

            // find target field
            while (std::getline(iss, current_name, '.'))
            {
                Reflection::FieldAccessor* fields;
                int                        fields_count = meta.getFieldsList(fields);
                auto                       field_iter   = std::find_if(
                    fields, fields + fields_count, [current_name](auto f) { return f.getFieldName() == current_name; });
                if (field_iter == fields + fields_count)
                {
                    delete[] fields;
                    return false;
                }

                field_accessor = *field_iter;
                delete[] fields;

                target_instance = field_instance;

                // for next iteration
                field_instance = field_accessor.get(target_instance);
                field_accessor.getTypeMeta(meta);
            }
            return true;
        }
        return false;
    }

    template<typename T>
    void LuaComponent::set(std::weak_ptr<GObject> game_object, const char* name, T value)
    {
        // LOG_INFO在runtime/core/base/macro.h中定义
        LOG_INFO("set process");
        LOG_INFO(name);
        Reflection::FieldAccessor field_accessor;
        void*                     target_instance;
        if (find_component_field(game_object, name, field_accessor, target_instance))
        {
            field_accessor.set(target_instance, &value);
        }
        else
        {
            LOG_ERROR("Can't find target field.");
        }
    }

    template<typename T>
    T LuaComponent::get(std::weak_ptr<GObject> game_object, const char* name)
    {
        LOG_INFO("get process");
        LOG_INFO(name);

        Reflection::FieldAccessor field_accessor;
        void*                     target_instance;
        if (find_component_field(game_object, name, field_accessor, target_instance))
        {
            //(T*)将类型强制转换为T类型
            return *(T*)field_accessor.get(target_instance);
        }
        else
        {
            LOG_ERROR("Can't find target field.");
        }
    }

    void LuaComponent::invoke(std::weak_ptr<GObject> game_object, const char* name)
    {
        LOG_INFO(name);

        Reflection::TypeMeta meta;
        void*                target_instance = nullptr;
        std::string          method_name;

        // get target instance and meta
        std::string target_name(name);
        size_t      pos = target_name.find_first_of('.');
        // substr不会因为第二个参数过大而执行错误 会尽可能的返回字符串
        method_name = target_name.substr(pos + 1, target_name.size());
        target_name = target_name.substr(0, pos);

        if (target_name.find_first_of('.') == target_name.npos)
        {
            // target is a component
            auto components = game_object.lock()->getComponents();

            auto component_iter = std::find_if(
                components.begin(), components.end(), [target_name](auto c) { return c.getTypeName() == target_name; });
            if (component_iter != components.end())
            {
                meta            = Reflection::TypeMeta::newMetaFromName(target_name);
                target_instance = component_iter->getPtr();
            }
            else
            {
                LOG_ERROR("Can't find component!");
                return;
            }
        }
        else
        {
            Reflection::FieldAccessor field_accessor;
            if (find_component_field(game_object, name, field_accessor, target_instance))
            {
                target_instance = field_accessor.get(target_instance);
                field_accessor.getTypeMeta(meta);
            }
            else
            {
                LOG_ERROR("Can't find target field.");
                return;
            }
        }

        // invoke function
        Reflection::MethodAccessor* methods;
        size_t                      method_count = meta.getMethodsList(methods);
        auto                        method_iter  = std::find_if(
            methods, methods + method_count, [method_name](auto m) { return m.getMethodName() == method_name; });
        if (method_iter != methods + method_count)
        {
            method_iter->invoke(target_instance);
        }
        else
        {
            LOG_ERROR("Can't find method.");
        }

        delete[] methods;
    }

    void LuaComponent::postLoadResource(std::weak_ptr<GObject> parent_gobject)
    {
        m_parent_object = parent_gobject;
        m_lua_state.open_libraries(sol::lib::base);
        m_lua_state.set_function("set_float", &LuaComponent::set<float>);
        m_lua_state.set_function("get_bool", &LuaComponent::get<bool>);
        m_lua_state.set_function("invoke", &LuaComponent::invoke);

        m_lua_state["GameObject"] = m_parent_object;
    }

    void LuaComponent::tick(float delta_time)
    {
        // LOG_INFO(m_lua_script)
        m_lua_state.script(m_lua_script);
    }

} // namespace Piccolo