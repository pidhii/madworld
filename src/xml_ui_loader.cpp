#include "gui/xml_ui_loader.hpp"
#include "gui/utilities.hpp"
#include "video_manager.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <type_traits>


template <typename ...T>
struct state_saver {
  state_saver(T &...refs): m_save {refs...}, m_refs {refs...} {}

  ~state_saver()
  { m_refs = m_save; }

  std::tuple<std::__remove_cvref_t<T>...> m_save;
  std::tuple<T&...> m_refs;
}; // struct state_saver


mw::sdl_string_factory
mw::xml_ui_loader::_make_string_factory(const _style &style)
{
  mw::sdl_string_factory strfac {style.font, style.fg_color};
  strfac.set_font_size(style.point_size);
  strfac.set_style(style.font_style);
  if (style.bg_color)
    strfac.set_bg_color(style.bg_color.value());
  return strfac;
}



static std::string
_node_type_name(pugi::xml_node_type type)
{
  switch (type)
  {
    case pugi::node_null: return "null";
    case pugi::node_doctype: return "doctype";
    case pugi::node_comment: return "comment";
    case pugi::node_document: return "document";
    case pugi::node_declaration: return "declaration";
    case pugi::node_pi: return "pi";
    case pugi::node_element: return "element";
    case pugi::node_pcdata: return "pcdata";
    case pugi::node_cdata: return "cdata";
  }
  std::terminate(); // unreachable
}



mw::xml_dom
mw::xml_ui_loader::build_component(const pugi::xpath_query &query,
                                   const parameters &params)
{
  if (not m_xmldoc.has_value())
    throw std::runtime_error {"xml_ui_loader initialized without xml_document"};
  return build_component(m_xmldoc.value()->select_node(query).node(), params);
}

mw::xml_dom
mw::xml_ui_loader::build_component(std::string_view query,
                                   const parameters &params)
{
  if (not m_xmldoc.has_value())
    throw std::runtime_error {"xml_ui_loader initialized without xml_document"};
  return build_component(m_xmldoc.value()->select_node(query.data()).node(),
                         params);
}

mw::xml_dom
mw::xml_ui_loader::build_component(const pugi::xml_node &xml,
                                   const parameters &params)
{
  std::shared_ptr<xml_dom::shdata> shdata = std::make_shared<xml_dom::shdata>();
  insert_pi_parameters(
      xml, shdata->xmldoc,
      [&params](const pugi::xml_node &innode, pugi::xml_node &outroot) {
        const auto test = [&](auto &p) { return p.name() == innode.name(); };
        const auto it = std::find_if(params.begin(), params.end(), test);
        if (it != params.end())
          it->insert(outroot);
      });
  const pugi::xml_node pxml = shdata->xmldoc.first_child();

  mw::component *c = _build_and_remember_component(pxml);

  shdata->nodemap = std::move(m_components);
  return {shdata, pxml, c};
}


template <typename Component, typename AttrIter>
void
mw::xml_ui_loader::_apply_enriched_attributes(Component *component,
                                              AttrIter begin, AttrIter end)
{
  for (; begin != end; ++begin)
  {
    if (std::strcmp(begin->name(), "left-margin") == 0)
      component->set_left_margin(mw::parse_size(begin->value()));
    if (std::strcmp(begin->name(), "right-margin") == 0)
      component->set_right_margin(mw::parse_size(begin->value()));
    if (std::strcmp(begin->name(), "top-margin") == 0)
      component->set_top_margin(mw::parse_size(begin->value()));
    if (std::strcmp(begin->name(), "bottom-margin") == 0)
      component->set_bottom_margin(mw::parse_size(begin->value()));
    if (std::strcmp(begin->name(), "fill-color") == 0)
      component->set_fill_color(mw::parse_color(begin->value()));
    if (std::strcmp(begin->name(), "border-color") == 0)
      component->set_border_color(mw::parse_color(begin->value()));
    if (std::strcmp(begin->name(), "border-width") == 0)
      component->set_border_width(mw::parse_size(begin->value()));
    if (std::strcmp(begin->name(), "min-width") == 0)
      component->set_min_width(mw::parse_size(begin->value()));
    if (std::strcmp(begin->name(), "min-height") == 0)
      component->set_min_height(mw::parse_size(begin->value()));
  }
}


mw::component *
mw::xml_ui_loader::_build_and_remember_component(const pugi::xml_node &xml)
{
  state_saver _ {m_style};
  _update_style_from_attributes(xml.attributes());

  mw::component *c = _build_component(xml);
  m_components[xml] = c;
  return c;
}


mw::component *
mw::xml_ui_loader::_build_component(const pugi::xml_node &xml)
{
  if (std::strcmp(xml.name(), "label") == 0)
  {
    mw::sdl_string_factory strfac = _make_string_factory(m_style);
    const std::string text = xml_text_walker::extract_text(xml);
    auto *label = new mw::label {strfac(text.c_str())};
    _apply_enriched_attributes(label, xml.attributes_begin(),
                               xml.attributes_end());
    return label;
  }
  else if (std::strcmp(xml.name(), "button") == 0)
  {
    auto button = new mw::button {
      _build_and_remember_component(xml.find_child_by_attribute("tag", "normal")),
      _build_and_remember_component(xml.find_child_by_attribute("tag", "hover")),
    };
    _apply_enriched_attributes(button, xml.attributes_begin(),
                               xml.attributes_end());
    return button;
  }
  else if (std::strcmp(xml.name(), "vertical-layout") == 0)
  {
    auto *layout = new mw::vertical_layout;
    for (const pugi::xml_node &child : xml.children())
      layout->add_component(_build_and_remember_component(child));
    _apply_enriched_attributes(layout, xml.attributes_begin(),
                               xml.attributes_end());
    return layout;
  }
  else if (std::strcmp(xml.name(), "horizontal-layout") == 0)
  {
    auto *layout = new mw::horisontal_layout;
    for (const pugi::xml_node &child : xml.children())
      layout->add_component(_build_and_remember_component(child));
    _apply_enriched_attributes(layout, xml.attributes_begin(),
                               xml.attributes_end());
    return layout;
  }
  else if (std::strcmp(xml.name(), "circular-stack") == 0)
  {
    using namespace std::placeholders;
    std::vector<mw::component*> states;
    std::transform(xml.children().begin(), xml.children().end(),
                    std::back_inserter(states),
                    std::bind(&xml_ui_loader::_build_and_remember_component, this, _1));
    auto *cs = new mw::circular_stack {states.begin(), states.end()};
    _apply_enriched_attributes(cs, xml.attributes_begin(),
                               xml.attributes_end());
    return cs;
  }
  else if (std::strcmp(xml.name(), "text-entry") == 0)
  {
    SDL_Renderer *rend = mw::video_manager::instance().get_sdl().get_renderer();
    mw::sdl_string_factory strfac = _make_string_factory(m_style);
    mw::texture_info linfo, winfo;
    strfac("w").get_texture_info(rend, winfo);
    strfac("l").get_texture_info(rend, linfo);

    const int reqw = mw::parse_size(xml.attribute("width").as_string("0"));
    const int reqh = mw::parse_size(xml.attribute("height").as_string("0"));
    const int w = std::max(reqw, winfo.w);
    const int h = std::max(reqh, linfo.h);
    const bool cursor = xml.attribute("cursor").as_bool(false);
    auto *textent = new text_entry {strfac, w, h};
    textent->set_cursor(cursor);
    _apply_enriched_attributes(textent, xml.attributes_begin(), xml.attributes_end());
    return textent;
  }

  error("unknown component: type='%s'", _node_type_name(xml.type()).c_str());
  throw exception {"unknown component type"};
}


template <typename InIter, typename OutIter>
void
mw::xml_ui_loader::_fix_string(InIter begin, InIter end, OutIter out)
{
  const auto safe_isspace = [] (char c) { return std::isspace(unsigned(c)); };
  // trim leading spaces
  begin = std::find_if_not(begin, end, safe_isspace);

  while (begin != end)
  {
    if (std::isspace(*begin))
    { // replace a sequence of space-characters with a single white-space
      // and trim trailing space-characters
      begin = std::find_if_not(begin, end, safe_isspace);
      if (begin == end)
        return;
      *out++ = ' ';
    }

    *out++ = *begin++;
  }
  return;
}


template <typename Attrs>
void
mw::xml_ui_loader::_update_style_from_attributes(const Attrs &attributes)
{
  for (const pugi::xml_attribute &attr : attributes)
  {
    if (std::strcmp(attr.name(), "fg") == 0)
      m_style.fg_color = mw::parse_color(attr.value());
    else if (std::strcmp(attr.name(), "bg") == 0)
      m_style.bg_color = mw::parse_color(attr.value());
    else if (std::strcmp(attr.name(), "font-size") == 0)
      m_style.point_size = attr.as_uint();
    else if (std::strcmp(attr.name(), "font") == 0)
    { // Don't update font if path didnt change (expencive)
      if (m_style.font.get_font_path() != attr.value())
        m_style.font = ttf_font {attr.value()};
    }
  }
}

mw::xml_dom_list
mw::xml_dom::select(const pugi::xpath_query &xpath) const
{
  xml_dom_list result;
  for (const pugi::xpath_node &xnode : m_selfxml.select_nodes(xpath))
  {
    mw::component *childc = m_shdata->nodemap.at(xnode.node());
    result.emplace_back(m_shdata, xnode.node(), childc);
  }
  return result;
}

mw::xml_dom
mw::xml_dom::operator () (const pugi::xpath_query &xpath) const
{
  pugi::xml_node childxml = m_selfxml.select_node(xpath).node();
  mw::component *childc = m_shdata->nodemap.at(childxml);
  return xml_dom {m_shdata, childxml, childc};
}

mw::xml_dom
mw::xml_dom::operator [] (std::string_view id) const
{
  using namespace std::string_literals;
  const pugi::xpath_query q {("descendant::*[@id='" + std::string {id} + "']").c_str()};
  pugi::xml_node childxml = m_selfxml.select_node(q).node();
  mw::component *childc = m_shdata->nodemap.at(childxml);
  return xml_dom {m_shdata, childxml, childc};
}
