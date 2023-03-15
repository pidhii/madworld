#include "object_factory.hpp"
#include "walls.hpp"

static void
decode_points(const eth::value &l, std::vector<mw::pt2d_d> &pts)
{
  for (eth::value it = l; not it.is_nil(); it = it.cdr())
  {
    const eth::value &x = it.car();
    pts.emplace_back(x[0], x[1]);
  }
}

mw::object*
mw::build_object(const eth::value &objdata)
{
  const std::string tag = objdata.tag();
  const eth::value data = objdata.val();

  if (tag == "basic_wall")
  {
    std::vector<pt2d_d> vertices;
    decode_points(data["vertices"], vertices);
    const std::string ends_spec = data["ends_spec"];
    if (ends_spec == "ignore_ends")
    {
      basic_wall<ignore_ends> *ret = new basic_wall<ignore_ends> {vertices};
      ret->set_color(data["color"]);
      return ret;
    }
    else if (ends_spec == "better_ends")
    {
      basic_wall<better_ends> *ret = new basic_wall<better_ends> {vertices};
      ret->set_color(data["color"]);
      return ret;
    }
    else if (ends_spec == "solid_ends")
    {
      basic_wall<solid_ends> *ret = new basic_wall<solid_ends> {vertices};
      ret->set_color(data["color"]);
      return ret;
    }
    else
      throw std::logic_error {"mw::build_object -- unknown `ends_spec` value"};
  }

  else if (tag == "filled_wall")
  {
    std::vector<pt2d_d> vertices;
    decode_points(data["vertices"], vertices);
    filled_wall *ret = new filled_wall {vertices};
    ret->set_edge_color(data["edge_color"]);
    ret->set_fill_color(data["fill_color"]);
    return ret;
  }

  else
    throw std::logic_error {"mw::build_object -- undefined object"};
}

