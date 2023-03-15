#ifndef UI_LAYER_HPP
#define UI_LAYER_HPP

#include <list>
#include <optional>

namespace mw {

class ui_manager;
class ui_layer;
class ui_float;
typedef std::list<ui_layer*>::const_iterator ui_layer_id;
typedef std::list<ui_float*>::const_iterator ui_float_id;

class ui_layer {
  public:
  enum class size {
    screen_portion,
    whole_screen,
  };

  explicit
  ui_layer(size sz): m_size {sz} { }

  virtual ~ui_layer() = default;

  virtual void draw() const = 0;
  virtual void run_layer(ui_manager&) = 0;
  virtual void destroy_layer() = 0;

  size
  get_size() const noexcept
  { return m_size; }

  const ui_layer_id&
  get_id() const
  { return m_id.value(); }

  private:
  void
  set_id(const ui_layer_id &id)
  { m_id = id; }

  private:
  std::optional<ui_layer_id> m_id;
  size m_size;;

  friend class ui_manager;
}; // class mw::ui_layer


class ui_float {
  public:
  virtual ~ui_float() = default;

  virtual void draw() const = 0;
  virtual void destroy_float() = 0;

  const ui_float_id&
  get_id() const
  { return m_id.value(); }

  private:
  void
  set_id(const ui_float_id &id)
  { m_id = id; }

  private:
  std::optional<ui_float_id> m_id;

  friend class ui_manager;
}; // class mw::ui_float


} // namespace mw

#endif
