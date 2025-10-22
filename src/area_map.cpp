#include "area_map.hpp"
#include "logging.h"
#include "walls.hpp"
#include "object_factory.hpp"
#include "vision.hpp"
#include "textures.hpp"
#include "physics.hpp"
#include "color_manager.hpp"

#include <ether/sandbox.hpp>

#include <vector>
#include <sstream>
#include <algorithm>
#include <stack>

#include <boost/format.hpp>


mw::area_map::area_map(sdl_environment &sdl, texture_storage &texstorage)
: m_sdl {sdl},
  m_scale {22},
  m_x_offs {0},
  m_y_offs {0},
  m_width {500},
  m_height {500},
  m_has_walls {false},
  m_texstorage {texstorage},
  m_vicinity_grid {size_t(m_width) / 5, size_t(m_height) / 5},
  m_msglog {sdl, video_manager::instance().get_font(),
    color_manager::instance()["Normal"], 800, 200}
{ }

mw::area_map::~area_map()
{
  for (const auto& ent : m_objects)
    delete ent.objptr;
}

void
mw::area_map::init_background(int pixw, int pixh)
{
  SDL_Renderer *rend = m_sdl.get_renderer();

  m_bgtex = create_texture(rend, SDL_TEXTUREACCESS_TARGET, pixw, pixh);

  SDL_Texture *oldtarget = get_render_target(rend);
  set_render_target(rend, m_bgtex);
  SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(rend);
  set_render_target(rend, oldtarget);

  m_world_to_bgtex = mapping {{0, 0}, (pixw-1)/m_width, (pixh-1)/m_height};
}


void
mw::area_map::build_walls()
{
  if (not m_has_walls)
  {
    const std::vector<pt2d_d> corners = {
      {0, 0}, {m_width, 0}, {m_width, m_height}, {0, m_height}, {0, 0},
    };
    const object_id id = add_static_object(new basic_wall<solid_ends> {corners});
    register_phys_obstacle(id);
    m_has_walls = true;
  }
  else
    warning("attempt to build map-walls when they are already present");
}

void
mw::area_map::load(const std::string &path)
{
  eth::sandbox ether;
  std::ostringstream cmd;
  cmd << "first(load('" << path << "'))";
  const eth::value conf = ether(cmd.str());

  m_width = conf["size"][0];
  m_height = conf["size"][1];
  m_has_walls = bool(conf["has_walls"]);
  for (eth::value l = conf["obstacles"]; not l.is_nil(); l = l.cdr())
  {
    try
    {
      object *obj = build_object(l.car());
      object_id obsid = add_static_object(obj);

      if (dynamic_cast<phys_obstacle*>(obj))
        register_phys_obstacle(obsid);
      if (dynamic_cast<vis_obstacle*>(obj))
        register_vis_obstacle(obsid);
    }
    catch (const std::logic_error &err)
    {
      error("failed to load object (%s)", err.what());
      throw exception {"failed to load map (" + path + ")"}.in(__func__);
    }
  }

  // refresh the grid
  build_grid();
}

void
mw::area_map::build_grid()
{
  // discard current grid
  m_static_grid = boost::none;
  m_static_grid.emplace(rectangle {
    {m_x_offs, m_y_offs},
    get_width(),
    get_height()}
  );

  // initialize the new grid
  grid<bool> &g = m_static_grid.value();
  const double maxcellsize = 5;
  const double mincellsize = 0.5;
  g.scan([&] (grid_view<bool> &gcell) {
    for (const object_entry &objent : m_objects)
    {
      if ((objent.flags & oflag::is_static) == 0)
        continue;

      if (not objent.pobsit.has_value())
      {
        warning("static object is not a phys_obstacle");
        continue;
      }

      if (gcell.is_leaf())
      {
        // divide cells larger then the max size
        if (gcell.get_box().width > maxcellsize)
        {
          gcell.divide(2, 2, false);
          break;
        }
        // dont subdivide further if cell dost not contain any phys-obstacles
        if (not (*objent.pobsit.value())->overlap_box(gcell.get_box()))
          continue;
        // otherwize, sibdivide even more until below min size
        if (gcell.get_box().width > mincellsize)
        {
          gcell.divide(2, 2, false);
          break;
        }
        // mark as occupied
        gcell.set_value(true);
        break;
      }
    }
  });
}

const mw::grid<bool>&
mw::area_map::get_grid() const
{
  if (not m_static_grid.has_value())
  {
    error("static grid was not initialized");
    error("call mw::area_map::build_grid() to initialize it");
    throw exception {
      "static grid was not initialized"
    }.in(__func__);
  }
  return m_static_grid.value();
}

mw::object_id
mw::area_map::add_object(object *obj) noexcept
{
  m_objects.emplace_back(obj);
  return {--m_objects.end()};
}

mw::object_id
mw::area_map::add_static_object(object *obj) noexcept
{
  m_objects.emplace_back(obj);
  const object_id id = --m_objects.end();
  id.get()->flags |= oflag::is_static;
  _put_on_vicinity_grid(id, true);
  return id;
}

void
mw::area_map::register_phys_object(object_id it)
{
  object_entry& ent = *it.get();
  phys_object *obs = dynamic_cast<phys_object*>(ent.objptr);
  if (obs == nullptr)
  {
    throw exception {
      "referred object can not be casted into a phys_object"
    }.in(__func__);
  }
  m_phys_objects.push_back(obs);
  ent.pobjit = --m_phys_objects.cend();
}

void
mw::area_map::register_phys_obstacle(object_id it)
{
  object_entry& ent = *it.get();
  phys_obstacle *obs = dynamic_cast<phys_obstacle*>(ent.objptr);
  if (obs == nullptr)
  {
    throw exception {
      "referred object can not be casted into a phys_obstacle"
    }.in(__func__);
  }
  m_phys_obstacles.push_back(obs);
  ent.pobsit = --m_phys_obstacles.cend();
}

void
mw::area_map::register_vis_obstacle(object_id it)
{
  object_entry& ent = *it.get();
  vis_obstacle *obs = dynamic_cast<vis_obstacle*>(ent.objptr);
  if (obs == nullptr)
  {
    throw exception {
      "referred object can not be casted into a vis_obstacle"
    }.in(__func__);
  }
  m_vis_obstacles.push_back(obs);
  ent.vobsit = --m_vis_obstacles.cend();
}

void
mw::area_map::zoom(const pt2d_i &p, double newscale) noexcept
{
  const double mul = newscale/m_scale;
  m_x_offs = fma(-mul, p.x-m_x_offs, p.x);
  m_y_offs = fma(-mul, p.y-m_y_offs, p.y);
  m_scale = newscale;
}

void
mw::area_map::adjust_offset(const pt2d_d &p, const pt2d_i &pix) noexcept
{
  m_x_offs = pix.x - m_scale*p.x;
  m_y_offs = pix.y - m_scale*p.y;
}

void
mw::area_map::adjust_to_box_w(const pt2d_i &at, int w) noexcept
{
  m_x_offs = at.x;
  m_y_offs = at.y;
  m_scale = w/m_width;
}

void
mw::area_map::adjust_to_box_h(const pt2d_i &at, int h) noexcept
{
  m_x_offs = at.x;
  m_y_offs = at.y;
  m_scale = h/m_height;
}

void
mw::area_map::draw() const noexcept
{
  SDL_Renderer *rend = m_sdl.get_renderer();
  pt2d_i corners[] = {
    point_to_pixels({0, 0}),
    point_to_pixels({m_width, 0}),
    point_to_pixels({m_width, m_height}),
    point_to_pixels({0, m_height}),
    point_to_pixels({0, 0}),
  };
  SDL_SetRenderDrawColor(rend, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDrawLines(rend, (SDL_Point*)corners, 5);
}

void
mw::area_map::tick(physics_processor &physproc, int msec)
{
  for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
  {
    object* obj = it->objptr;
    if (it->pobjit.has_value())
      physproc.add_object(const_cast<phys_object*>(*it->pobjit.value()));
    else if (it->pobsit.has_value())
      physproc.add_obstacle(const_cast<phys_obstacle*>(*it->pobsit.value()));
  }

  physproc.process(*this);

  for (auto it = m_objects.begin(); it != m_objects.end();)
  {
    object* obj = it->objptr;
    if (obj->is_gone())
    {
      auto tmp = it;
      ++it;

      if (tmp->pobjit.has_value())
        m_phys_objects.erase(tmp->pobjit.value());
      if (tmp->pobsit.has_value())
        m_phys_obstacles.erase(tmp->pobsit.value());
      if (tmp->vobsit.has_value())
        m_vis_obstacles.erase(tmp->vobsit.value());

      m_objects.erase(tmp);
      delete obj;
      continue;
    }

    obj->update(*this, msec);
    ++it;
  }
}

void
mw::area_map::draw_all() const
{
  SDL_Renderer *rend = m_sdl.get_renderer();
  for (const auto &ent : m_objects)
    ent.objptr->draw(*this);
}

void
mw::area_map::draw_visible(const vision_processor &local_vision,
    const vision_processor &global_vision) const
{
  SDL_Renderer *rend = m_sdl.get_renderer();

  m_global_vision = global_vision;

  // draw everything but vis-obstacles
  for (const auto &ent : m_objects)
  {
    if (not ent.vobsit.has_value())
      ent.objptr->draw(*this);
  }

  // draw obstacles within local vision
  for (const sight &s : local_vision.get_sights())
    s.static_data.obs->draw(*this, s);

  m_global_vision = boost::none;
}

void
mw::area_map::draw_obstacles() const
{
  SDL_Renderer *rend = m_sdl.get_renderer();
  for (const phys_obstacle *obs : m_phys_obstacles)
    obs->draw(*this);
}

//SDL_Texture*
//mw::area_map::_create_msgtex(int width, int height) const
//{
  //SDL_Renderer *rend = m_sdl.get_renderer();

  //SDL_Texture *canvas =
    //create_texture(rend, SDL_TEXTUREACCESS_TARGET, width, height);
  //SDL_SetTextureBlendMode(canvas, SDL_BLENDMODE_BLEND);
  //SDL_Texture *oldtarget = get_render_target(rend);
  //set_render_target(rend, canvas);
  //SDL_BlendMode oldblend;
  //SDL_GetRenderDrawBlendMode(rend, &oldblend);
  //SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);
  //SDL_SetRenderDrawColor(rend, 0xFF, 0x00, 0x00, 0x44);
  //SDL_RenderClear(rend);
  //SDL_SetRenderDrawBlendMode(rend, oldblend);
  //set_render_target(rend, oldtarget);

  //return canvas;
//}

void
mw::area_map::add_message(const std::string &msgstr)
{ m_msglog.add_message(msgstr); }

void
mw::area_map::draw_messages() const
{
  int w, h;
  SDL_GetWindowSize(m_sdl.get_window(), &w, &h);
  m_msglog.draw({10, h - 210});
}

void
mw::area_map::blit_glow_with_shadowcast(SDL_Texture *tex,
    const rectangle &dstbox, uint8_t alpha, int flags,
    vision_processor &localvision) const
{
  SDL_Renderer *rend = m_sdl.get_renderer();
  if (not m_bgtex)
    abort();

  uint32_t format;
  int access, w, h;
  const int pixw = dstbox.width * m_scale;
  const int pixh = dstbox.height * m_scale;

  //SDL_QueryTexture(tex, &format, &access, &w, &h);

  // create new texture, we will draw on it
  SDL_Texture *canvas = create_texture(rend, SDL_TEXTUREACCESS_TARGET, pixw, pixh);
  SDL_Texture *oldtarget = get_render_target(rend);
  set_render_target(rend, canvas);

  // maps between map-CS and canvas-CS
  SDL_QueryTexture(canvas, &format, &access, &w, &h);
  const mapping map_to_screen {{m_x_offs, m_y_offs}, m_scale, m_scale};
  const rectangle pixbox = map_to_screen(dstbox);
  const mapping tex_to_screen {to_vec(pixbox.offset), pixbox.width/w, pixbox.height/h};
  const mapping map_to_tex = compose(tex_to_screen.inverse(), map_to_screen);
  const mapping bgtex_to_tex = compose(map_to_tex, m_world_to_bgtex.inverse());

  // draw glow-texture
  SDL_SetTextureAlphaMod(tex, alpha);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
  SDL_RenderCopy(rend, tex, nullptr, nullptr); // TODO handle errors

  // draw bakcground
  SDL_Rect bgtexrect = m_world_to_bgtex(dstbox);
  SDL_Rect bgdstrect = map_to_tex(dstbox);
  if (bgtexrect.x < 0)
  {
    const double bgtex_dx = 0 - bgtexrect.x;
    bgtexrect.x += bgtex_dx;
    bgtexrect.w -= bgtex_dx;
    const double tex_dx = bgtex_dx * bgtex_to_tex.get_x_scale();
    bgdstrect.x += tex_dx;
    bgdstrect.w -= tex_dx;
  }
  if (bgtexrect.y < 0)
  {
    const double bgtex_dy = 0 - bgtexrect.y;
    bgtexrect.y += bgtex_dy;
    bgtexrect.h -= bgtex_dy;
    const double tex_dy = bgtex_dy * bgtex_to_tex.get_y_scale();
    bgdstrect.y += tex_dy;
    bgdstrect.h -= tex_dy;
  }
  const double bgtexw = m_width * m_world_to_bgtex.get_x_scale();
  const double bgtexh = m_height * m_world_to_bgtex.get_y_scale();
  if (bgtexrect.x + bgtexrect.w > bgtexw)
  {
    const double bgtex_dw = bgtexrect.x + bgtexrect.w - bgtexw;
    bgtexrect.w -= bgtex_dw;
    const double tex_dw = bgtex_dw * bgtex_to_tex.get_x_scale();
    bgdstrect.w -= tex_dw;
  }
  if (bgtexrect.y + bgtexrect.h > bgtexh)
  {
    const double bgtex_dh = bgtexrect.y + bgtexrect.h - bgtexh;
    bgtexrect.h -= bgtex_dh;
    const double tex_dh = bgtex_dh * bgtex_to_tex.get_y_scale();
    bgdstrect.h -= tex_dh;
  }
  static SDL_BlendMode superblend = SDL_BLENDMODE_INVALID;
  if (superblend == SDL_BLENDMODE_INVALID)
  {
    superblend = SDL_ComposeCustomBlendMode(
      SDL_BLENDFACTOR_SRC_ALPHA,
      SDL_BLENDFACTOR_ONE,
      SDL_BLENDOPERATION_ADD,
      SDL_BLENDFACTOR_DST_ALPHA,
      SDL_BLENDFACTOR_ZERO,
      SDL_BLENDOPERATION_ADD);
    //superblend = SDL_BLENDMODE_ADD;
  }
  SDL_SetTextureBlendMode(m_bgtex, superblend);
  SDL_RenderCopy(rend, m_bgtex, &bgtexrect, &bgdstrect); // TODO handle errors

  // cast shadows
  SDL_SetRenderTarget(rend, canvas);
  // TODO: fix this (vision origin must be provided by caller)
  const pt2d_d viscenter =
    dstbox.offset + vec2d_d(dstbox.width, dstbox.height)/2;
  const double visradius = std::max(dstbox.width, dstbox.height)/2;
  localvision.set_source({viscenter, visradius});
  localvision.load_obstacles(m_vis_obstacles);
  localvision.process();
  localvision.shadowcast(rend, dstbox, SDL_BLENDMODE_NONE, 0x00000000, map_to_tex);

  // apply effects due to global vision
  vision_processor::sight_container localsights {localvision.get_sights().begin(),
                                                 localvision.get_sights().end()};
  if (flags & blit_flags::apply_global_vision and m_global_vision.has_value())
  {
    m_global_vision
      .value()
      .shadowcast(rend, dstbox, SDL_BLENDMODE_NONE, 0x00000000, map_to_tex);
    const bool foreignsource =
      localvision.get_source().center != m_global_vision->get_source().center;
    m_global_vision.value().apply(foreignsource, localsights);
  }
  set_render_target(rend, oldtarget);

  // draw texture to the screen
  SDL_Rect dst;
  dst.x = pixbox.offset.x;
  dst.y = pixbox.offset.y;
  dst.w = pixbox.width;
  dst.h = pixbox.height;
  SDL_SetTextureBlendMode(canvas, SDL_BLENDMODE_BLEND);
  SDL_RenderCopy(rend, canvas, nullptr, &dst);

  // destroy texture
  SDL_DestroyTexture(canvas);

  // drow locally visible objects
  if (flags & blit_flags::draw_sights)
  {
    for (const sight &s : localsights)
      s.static_data.obs->draw(*this, s);
  }
}

void
mw::area_map::blit_glow_with_shadowcast(SDL_Texture *tex,
    const rectangle &dstbox, uint8_t alpha, int flags) const
{
  vision_processor visproc;
  blit_glow_with_shadowcast(tex, dstbox, alpha, flags, visproc);
}

void
mw::area_map::add_terrain_overlay(SDL_Texture *tex, const rectangle &dstbox,
    uint8_t alpha) const
{
  if (m_bgtex == nullptr)
    return;

  SDL_Renderer *rend = m_sdl.get_renderer();

  uint32_t format;
  int access, w, h;

  const pt2d_d pixoffs = m_world_to_bgtex(dstbox.offset);
  const int pixw = dstbox.width * m_world_to_bgtex.get_x_scale();
  const int pixh = dstbox.height * m_world_to_bgtex.get_y_scale();

  SDL_Texture *oldtarget = get_render_target(rend);
  set_render_target(rend, m_bgtex);
  SDL_SetTextureAlphaMod(tex, alpha);
  SDL_Rect dstrect;
  dstrect.x = pixoffs.x;
  dstrect.y = pixoffs.y;
  dstrect.w = pixw;
  dstrect.h = pixh;
  if (SDL_RenderCopy(rend, tex, nullptr, &dstrect) < 0)
  {
    error("failed to copy texture (%s)", SDL_GetError());
    abort();
  }
  set_render_target(rend, oldtarget);
}

eth::value
mw::area_map::dump() const
{
  eth::value obstacles;
  for (const phys_obstacle *obs : m_phys_obstacles)
  {
    const eth::value obsdump = dynamic_cast<const object*>(obs)->dump();
    if (not obsdump.is_nil())
      obstacles = eth::cons(obsdump, obstacles);
  }

  return eth::record({
    {"size", eth::tuple(m_width, m_height)},
    {"has_walls", eth::boolean(m_has_walls)},
    {"obstacles", obstacles},
  });
}

void
mw::area_map::_put_on_vicinity_grid(const object_id &id, bool is_static)
{
  const object_entry& ent = *id.get();
  const phys_obstacle *pobs = dynamic_cast<const phys_obstacle*>(ent.objptr);
  if (pobs == nullptr)
  {
    throw exception {
      "referred object can not be casted into a phys_obstacle"
    }.in(__func__);
  }

  const auto [nx, ny] = m_vicinity_grid.get_dimentions();
  const double cw = m_width / nx;
  const double ch = m_height / ny;

  struct compare_points {
    bool operator () (const pt2d<size_t> &a, const pt2d<size_t> &b) const
    { return a.x == b.x ? a.y < b.y : a.x < b.x; }
  };
  std::set<pt2d<size_t>, compare_points> visited_cells;
  std::stack<pt2d<size_t>> stack;

  const pt2d_d pt = pobs->sample_point();
  const size_t ix0 = std::floor(pt.x / cw);
  const size_t iy0 = std::floor(pt.y / ch);
  stack.emplace(ix0, iy0);
  if (is_static)
    m_vicinity_grid.put_static(ix0, iy0, id);
  else
    m_vicinity_grid.put(ix0, iy0, id);

  while (not stack.empty())
  {
    const auto [ix0, iy0] = stack.top();
    stack.pop();

    for (int dx : {-1, 0, +1})
    {
      if ((ix0 == 0 and dx == -1) or
          (ix0 == nx-1 and dx == +1))
        continue;

      for (int dy : {-1, 0, +1})
      {
        if ((iy0 == 0 and dy == -1) or
            (iy0 == ny-1 and dy == +1))
          continue;

        const size_t ix = ix0 + dx;
        const size_t iy = iy0 + dy;

        if (visited_cells.find({ix, iy}) == visited_cells.end())
        {
          const rectangle cellbox {{ix*cw, iy*ch}, cw, ch};
          if (pobs->overlap_box(cellbox))
          {
            stack.emplace(ix, iy);
            if (is_static)
              m_vicinity_grid.put_static(ix, iy, id);
            else
              m_vicinity_grid.put(ix, iy, id);
          }
          visited_cells.emplace(ix, iy);
        }
      }
    }
  }
}

