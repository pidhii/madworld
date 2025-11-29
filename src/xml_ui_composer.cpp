#include "gui/xml_ui_composer.hpp"


sol::reference
mw::xml_to_lua(const pugi::xml_node &xml, sol::state &lua)
{
  switch (xml.type())
  {
    case pugi::node_null:
      return sol::nil;

    case pugi::node_element:
    {
      sol::table t = lua.create_table();
      size_t i = 0;
      for (const pugi::xml_node &childxml : xml.children())
      {
        sol::reference child = xml_to_lua(childxml, lua);
        t.set(i++, child);
        if (childxml.name())
          t.set(childxml.name(), child);
      }
      t.set("_attr", lua.create_table());
      for (const pugi::xml_attribute &attr : xml.attributes())
        t["_attr"][attr.name()] = sol::object {lua, sol::in_place, attr.value()};
      return t;
    }

    case pugi::node_cdata:
    case pugi::node_pcdata:
      return sol::object {lua, sol::in_place, xml.value()};

    default:
      throw std::runtime_error {"can't convert given xml into lua object"};
  }
}
