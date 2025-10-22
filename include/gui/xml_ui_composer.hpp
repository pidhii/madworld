#ifndef GUI_XML_UI_COMPOSER_HPP
#define GUI_XML_UI_COMPOSER_HPP

#include "gui/xml_ui_loader.hpp"


namespace mw {
inline namespace gui {


sol::lua_value
xml_to_lua(const pugi::xml_node &xml, sol::state &lua);;


class lua_binder {
  public:
  lua_binder(sol::state &lua): m_lua {lua} { }

  template <typename... Args>
  auto
  bind(mw::component *c, std::string_view tname, Args &&...args)
  {
    sol::table o = m_lua.create_table();
    return bind_o(c, tname, o, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto
  bind_o(mw::component *c, std::string_view tname, sol::table &o, Args &&...args)
  {
    c->lua_export(o);
    switch (m_lua[tname].get_type())
    {
      case sol::type::function:
        return m_lua[tname](o, std::forward<Args>(args)...);

      case sol::type::table: {
        sol::table t = m_lua[tname];
        return t["new"](t, o, std::forward<Args>(args)...);
      }

      default:
        throw std::runtime_error {
            "can't do lua binding with object of type" +
            sol::type_name(m_lua, m_lua[tname].get_type())};
    }
  }

  private:
  sol::state &m_lua;
};


struct nothing { };

template <typename PP = nothing>
class xml_ui_composer {
  public:
  xml_ui_composer(sol::state &lua, PP &&preprocessor = PP {})
  : m_lua {lua}, m_preprocessor {std::forward<PP>(preprocessor)}
  { }

  void
  load_document(const pugi::xml_document &xmldoc)
  {
    // Copy the document to perform mutations
    pugi::xml_document tmpdoc;
    for (pugi::xml_node node : xmldoc.children())
      tmpdoc.append_copy(node);

    // Attribute insertions
    if constexpr (not std::is_same_v<PP, nothing>)
      m_preprocessor(tmpdoc);

    // Load scripts
    for (pugi::xpath_node xnode : tmpdoc.select_nodes("/script"))
    {
      pugi::xml_node node = xnode.node();
      const std::string script = mw::xml_text_walker::extract_text(xnode.node());
      if (node.attribute("path"))
      {
        assert(script.empty());
        m_lua.script_file(node.attribute("path").value());
      }
      else
        m_lua.script(script);
    }

    // Append this document to the internal document
    for (const pugi::xml_node &node : tmpdoc.children())
      m_xmldoc.append_copy(node);
  }

  void
  load_document(std::string_view path)
  {
    pugi::xml_document xmldoc;
    xmldoc.load_file(path.data(), pugi::parse_default | pugi::parse_pi);
    load_document(xmldoc);
  }

  mw::xml_dom
  build_component(std::string_view xpath,
                  const mw::xml_ui_loader::parameters &params = {})
  {
    mw::xml_ui_loader uiloader;
    pugi::xml_node xml = m_xmldoc.select_node(xpath.data()).node();
    mw::xml_dom rootdom = uiloader.build_component(xml, params);

    using dom_tree_type = std::map<pugi::xml_node, sol::table>;
    std::shared_ptr<dom_tree_type> domtree = std::make_shared<dom_tree_type>();

    // (lua export) Local DOM traversal function
    auto _select = [rootdom, domtree, luap = &m_lua](std::string_view xpath) {
      sol::table result = luap->create_table();
      size_t i = 0;
      for (mw::xml_dom r : rootdom.select(xpath))
      {
        if (domtree->count(r.xml()))
          result[i++] = domtree->at(r.xml());
      }
      return result;
    };

    for (mw::xml_dom dom : rootdom.select("//*"))
    {
      sol::table o = m_lua.create_table();
      o.set_function("select", _select);
      o.set("xml", xml_to_lua(dom.xml(), m_lua));

      if (dom.xml().attribute("lua-bind"))
      {
        for (const auto &p : params)
        {
          pugi::xml_document dummydoc;
          p.insert(dummydoc);
          sol::lua_value value = xml_to_lua(dummydoc.first_child(), m_lua);
          o[p.name()] = value;
        }

        const std::string_view tname = dom.xml().attribute("lua-bind").value();
        o = lua_binder(m_lua).bind_o(dom, tname, o);
      }
      else
        dom->lua_export(o);

      domtree->emplace(dom.xml(), o);
    }

    return rootdom;
  }

  private:
  pugi::xml_document m_xmldoc;
  sol::state &m_lua;
  PP m_preprocessor;
};


} // namespace mw::gui
} // namespace mw

#endif