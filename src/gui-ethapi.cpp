#include "gui/ethapi.hpp"

#include <ether/sandbox.hpp>


static mw::gui::component*
to_component(const eth::value &x)
{ return reinterpret_cast<mw::gui::component*>(x.udata()); }

template <typename T>
static T*
to(const eth::value &c)
{
  if (T* ret = dynamic_cast<T*>(to_component(c)))
    return ret;
  else
    throw std::logic_error {"dynamic cast failed"};
}

template <typename Enriched>
static eth::value
_make_enriched_api()
{
  using namespace mw;

  return eth::record({
    {"set_left_margin", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_left_margin(val);
        return eth::nil();
    })},
    {"set_right_margin", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_right_margin(val);
        return eth::nil();
    })},
    {"set_top_margin", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_top_margin(val);
        return eth::nil();
    })},
    {"set_bottom_margin", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_bottom_margin(val);
        return eth::nil();
    })},
    {"set_fill_color", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_fill_color(val);
        return eth::nil();
    })},
    {"set_border_color", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_border_color(val);
        return eth::nil();
    })},
    {"set_min_width", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_min_width(val);
        return eth::nil();
    })},
    {"set_min_height", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->set_min_height(val);
        return eth::nil();
    })},
    {"ignore_signals", eth::function<2>([] (const eth::value &self, const eth::value &val) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->ignore_signals(bool(val));
        return eth::nil();
    })},
    {"on", eth::function<3>([] (const eth::value &self, const eth::value &what, const eth::value &cb) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->on(what, [=] (MWGUI_CALLBACK_ARGS) -> int {
          eth::value s = eth::user_data(static_cast<component*>(self));
          eth::value w = what;
          eth::value d = data.u64;
          return cb(s, w, d).num();
        });
        return eth::nil();
    })},
    {"forward", eth::function<3>([] (const eth::value &self, const eth::value &what, const eth::value &c) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->forward(what, to_component(c["__component"]));
        return eth::nil();
    })},
    {"get_width", eth::function<1>([] (const eth::value &self) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        return eth::value(selfval->get_width());
    })},
    {"get_height", eth::function<1>([] (const eth::value &self) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        return eth::value(selfval->get_height());
    })},
    {"draw", eth::function<2>([] (const eth::value &self, const eth::value &at) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->draw({int(at[0]), int(at[1])});
        return eth::nil();
    })},
    {"send", eth::function<3>([] (const eth::value &self, const eth::value &what, const eth::value &data) {
        Enriched *selfval = to<Enriched>(self["__component"]);
        selfval->send(what, uint64_t(data));
        return eth::nil();
    })},
  });
}

static eth::value
_make_label_base_api()
{
  using namespace mw;

  return eth::record({
    {"get_string", eth::function<1>([] (const eth::value &self) {
        label *lbl = to<label>(self);
        sdl_string *ret = new sdl_string {lbl->get_string()};
        return eth::user_data(ret, [] (void *self) { delete (sdl_string*)self; });
    })},
    {"set_string", eth::function<2>([] (const eth::value &self, const eth::value &s) {
        label *lbl = to<label>(self);
        sdl_string *str = (sdl_string*)s.udata();
        lbl->set_string(*str);
        return eth::nil();
    })},
  });
}


eth::value
mw::gui::make_label_api()
{
  eth::sandbox sb;

  sb.define("label_base_api", _make_label_base_api());
  sb.define("enriched_label_api", _make_enriched_api<label>());
  sb.define("new_label", eth::function<2>([] (const eth::value &sdl, const eth::value &str) {
      sdl_environment *sdlval = (sdl_environment*)sdl.udata();
      sdl_string *strval = (sdl_string*)str.udata();
      label *lbl = new label {*sdlval, *strval};
      return eth::user_data(
          static_cast<component*>(lbl),
          [] (void *self) { delete dynamic_cast<label*>((component*)(self)); }
      );
  }));
  sb.source(SCRIPTS_PATH "/label_api.eth");
  return sb("{wrap_label, make_label}");
}

