#ifndef GUI_XML_UI_LOADER_HPP
#define GUI_XML_UI_LOADER_HPP

#include "components.hpp"
#include "exceptions.hpp"
#include "gui/sdl_string.hpp"
#include "gui/ttf_font.hpp"

#include "pugixml.hpp"

#include <variant>
#include <vector>
#include <map>


namespace mw {
inline namespace gui {


class xml_dom {
  public:
  using xml_dom_list = std::vector<xml_dom>;

  struct shdata {
    pugi::xml_document xmldoc;
    std::map<pugi::xml_node, mw::component*> nodemap;
  };

  xml_dom(std::shared_ptr<shdata> shdata, pugi::xml_node selfxml,
          mw::component *self)
  : m_shdata {shdata}, m_selfxml {selfxml}, m_self {self}
  { }

  pugi::xml_node
  xml() const noexcept
  { return m_selfxml; }

  mw::component *
  get() const noexcept
  { return m_self; }

  operator mw::component * () const noexcept
  { return m_self; }

  mw::component *
  operator -> ()const noexcept
  { return m_self; }

  mw::component &
  operator * () const noexcept
  { return *m_self; }

  xml_dom_list
  select(const pugi::xpath_query &xpath) const;

  xml_dom_list
  select(std::string_view xpath) const
  { return select(pugi::xpath_query {xpath.data()}); }

  xml_dom
  operator () (const pugi::xpath_query &xpath) const;

  xml_dom
  operator () (std::string_view xpath) const
  { return this->operator()(pugi::xpath_query {xpath.data()}); }

  xml_dom
  operator [] (std::string_view id) const;

  private:
  std::shared_ptr<shdata> m_shdata;
  pugi::xml_node m_selfxml;
  mw::component *m_self;
};
using xml_dom_list = xml_dom::xml_dom_list;


class xml_ui_loader {
  static constexpr char _class_name[] = "mw::xml_ui_loader";

  struct _style {
    mw::ttf_font font {"/usr/share/fonts/TTF/DejaVuSansMono.ttf"};
    mw::color_t fg_color {be32toh(0xFFFFFFFF)};
    std::optional<mw::color_t> bg_color {std::nullopt};
    int point_size {24};
    int font_style {0};
  };

  static mw::sdl_string_factory
  _make_string_factory(const _style &style);
  
  public:
  using exception = scoped_exception<_class_name>;

  xml_ui_loader() = default;
  xml_ui_loader(const pugi::xml_document &xmldoc): m_xmldoc {&xmldoc} { }

  struct parameter_proxy {
    parameter_proxy(const std::string &name, const std::string &value)
    : m_name {name}, m_data {value}
    { }

    parameter_proxy(const std::string &name, const pugi::xml_node &value)
    : m_name {name}, m_data {value}
    { }

    void
    insert(pugi::xml_node &parent) const
    {
      if (std::holds_alternative<pugi::xml_node>(m_data))
        parent.append_copy(std::get<pugi::xml_node>(m_data));
      else
        parent.append_child(pugi::node_pcdata)
            .set_value(std::get<std::string>(m_data).c_str());
    }

    std::string_view
    name() const noexcept
    { return m_name; }

    private:
    const std::string m_name;
    std::variant<pugi::xml_node, std::string> m_data;
  };

  using parameters = std::vector<parameter_proxy>;

  xml_dom
  build_component(const pugi::xpath_query &xpath, const parameters& = {});

  xml_dom
  build_component(std::string_view xpath, const parameters& = {});

  xml_dom
  build_component(const pugi::xml_node &xml, const parameters& = {});

  private:
  mw::component *
  _build_and_remember_component(const pugi::xml_node &xml);

  mw::component *
  _build_component(const pugi::xml_node &xml);

  template <typename InIter, typename OutIter>
  static void
  _fix_string(InIter begin, InIter end, OutIter out);

  template <typename OutIter>
  void
  _content_to_string(const pugi::xml_node &xml, OutIter out);

  static mw::color_t
  _parse_color(const char *str);

  template <typename Attrs>
  void
  _update_style_from_attributes(const Attrs &attributes);

  template <typename Component, typename AttrIter>
  void
  _apply_enriched_attributes(Component *component, AttrIter begin, AttrIter end);

  _style m_style;
  std::map<pugi::xml_node, mw::component*> m_components;
  std::optional<const pugi::xml_document*> m_xmldoc;
};


static inline xml_dom
build_component_from_xml(const pugi::xml_node &xml)
{ return xml_ui_loader {}.build_component(xml); }



template <typename Parameterizer> 
std::enable_if_t<std::is_invocable_v<Parameterizer, const pugi::xml_node &,
                                     pugi::xml_node &>>
insert_pi_parameters(const pugi::xml_node &innode, pugi::xml_node &outroot,
                     Parameterizer parameterizer)
{
  switch (innode.type())
  {
    case pugi::node_pi:
      parameterizer(innode, outroot);
      break;

    case pugi::node_element:
    {
      pugi::xml_node outnode = outroot.append_child(pugi::node_element);
      outnode.set_name(innode.name());
      for (const pugi::xml_attribute &attr : innode.attributes())
        outnode.append_attribute(attr.name()).set_value(attr.value());
      for (const pugi::xml_node &inelt : innode.children())
        insert_pi_parameters(inelt, outnode, parameterizer);
      break;
    }

    default:
      outroot.append_copy(innode);
      break;
  }
}

inline void
insert_pi_parameters(
    const pugi::xml_node &innode, pugi::xml_node &outroot,
    const std::map<std::string, std::string> &parameters)
{
  insert_pi_parameters(
      innode, outroot,
      [&parameters](const pugi::xml_node &innode, pugi::xml_node &outroot) {
        const std::string_view pval = parameters.at(innode.name());
        outroot.append_child(pugi::node_pcdata).set_value(pval.data());
      });
}


class xml_text_walker final: public pugi::xml_tree_walker {
  public:
  bool
  for_each(pugi::xml_node &node) override
  {
    switch (node.type())
    {
      case pugi::node_cdata:
      case pugi::node_pcdata:
        m_buffer << node.value();
        return true;

      default:
        throw std::runtime_error {"not a text node"};
    }
  }

  static std::string
  extract_text(pugi::xml_node node)
  {
    xml_text_walker walker;
    node.traverse(walker);
    return walker.m_buffer.str();
  }

  private:
  std::ostringstream m_buffer;
}; // class mw::xml_text_walker


} // namespace mw::gui
} // namespace mw


#endif