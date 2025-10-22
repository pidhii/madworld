#include "central_config.hpp"
#include "color_manager.hpp"
#include "logging.h"
#include "map_generation.hpp"
#include "exceptions.hpp"
#include "gui/components.hpp"
#include "gui/composer.hpp"
#include "gui/menu.hpp"
#include "ui_manager.hpp"
#include "logging.h"
#include "algorithms/forest.hpp"
#include "map_generation.hpp"
#include "video_manager.hpp"

#include <SDL2/SDL.h>

#include <chrono>
#include <random>
#include <arpa/inet.h>


static std::pair<int, int>
get_window_size(mw::sdl_environment &sdl)
{
  int width, height;
  SDL_GetWindowSize(sdl.get_window(), &width, &height);
  return {width, height};
}


class gui: public mw::component, mw::map_generator_m1 {
  public:
  gui(mw::sdl_environment &sdl)
  : component(sdl)
  {
    size_t seed = std::random_device()();
    info("seed: %zd", seed);

    mw::generate_map(*this, seed, 200, 200);
  }

  int
  get_width() const override
  { return get_window_size(m_sdl).first; }

  int
  get_height() const override
  { return get_window_size(m_sdl).second; }

  void
  draw(const mw::pt2d_i &at) const override
  {
    using namespace std::chrono_literals;

    using clock = std::chrono::steady_clock;
    static const clock::time_point tstart = clock::now();

    const auto [winw, winh] = get_window_size(m_sdl);
    const mw::mapping viewport {{0., 0.}, double(winw), double(winh)};

    //if (clock::now() - tstart < 1s)
      //return;

    //for (const double x : m_vlines)
    //{
      //const auto [x1pix, y1pix] = viewport({x, 0});
      //const auto [x2pix, y2pix] = viewport({x, 1});
      //SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0x33, 0x33, 0x33, 0xFF);
      //SDL_RenderDrawLine(m_sdl.get_renderer(), x1pix, y1pix, x2pix, y2pix);
    //}

    //if (clock::now() - tstart < 2s)
      //return;

    //for (const double y : m_hlines)
    //{
      //const auto [x1pix, y1pix] = viewport({0, y});
      //const auto [x2pix, y2pix] = viewport({1, y});
      //SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0x33, 0x33, 0x33, 0xFF);
      //SDL_RenderDrawLine(m_sdl.get_renderer(), x1pix, y1pix, x2pix, y2pix);
    //}

    //if (clock::now() - tstart < 3s)
      //return;

    //for (const mw::pt2d_d &p : m_vertices)
    //{
      //const auto [xpix, ypix] = viewport(p);
      //circleRGBA(m_sdl.get_renderer(), xpix, ypix, 4, 0xFF, 0x55, 0x55, 0xFF);
    //}

    //if (clock::now() - tstart < 4s)
      //return;

    //for (size_t i = 0; i < m_vertices.size(); ++i)
    //{
      //for (size_t j = i+1; j < m_vertices.size(); ++j)
      //{
        //if (m_walls[j + i*m_vertices.size()])
        //{
          //const auto [x1pix, y1pix] = viewport(m_vertices.at(i));
          //const auto [x2pix, y2pix] = viewport(m_vertices.at(j));
          //SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0x33, 0xFF, 0x66, 0xFF);
          //SDL_RenderDrawLine(m_sdl.get_renderer(), x1pix, y1pix, x2pix, y2pix);
        //}
      //}
    //}

    //if (clock::now() - tstart < 5s)
      //return;

    const std::vector<mw::color_t> colors {
      0xFF5733FF, 0x33FF57FF, 0x3357FFFF, 0xFF33A1FF, 0xFF8333FF, 0x33FFF5FF,
      0x8D33FFFF, 0xFF3380FF, 0x33FF8DFF, 0xFFD633FF, 0xFF5733FF, 0x33A1FFFF,
      0xA833FFFF, 0xFF3333FF, 0x33FF83FF, 0xFFA833FF, 0x33FF33FF, 0x5733FFFF,
      0xFF33D4FF, 0x33FFD4FF, 0xFFB833FF, 0x33FFB8FF, 0x5733A1FF, 0xFFA1B8FF,
      0xFF5733FF, 0x33FFA1FF, 0xA1FF33FF, 0xFFA157FF, 0xFF3357FF, 0x57FFA1FF,
      0xA157FFFF, 0xFFA1A1FF
    };
    int roomcnt = 0;
    const int hlroom =
      std::chrono::duration_cast<std::chrono::seconds>(1*(clock::now() - tstart))
        .count() % 10;
    for (const auto &[roomid, blocks] : m_rooms)
    {
      if (roomcnt % 10 == hlroom)
      {
        for (const auto &[iv, ih] : blocks)
        {
          // a -- b
          // |    |
          // d -- c
          const mw::pt2d_d a = m_vertices[ih + iv*m_hlines.size()],
          b = m_vertices[ih + (iv+1)*m_hlines.size()],
          c = m_vertices[ih+1 + (iv+1)*m_hlines.size()],
          d = m_vertices[ih+1 + iv*m_hlines.size()];
          const mw::pt2d_i apix = mw::pt2d_i(viewport(a)),
                bpix = mw::pt2d_i(viewport(b)),
                cpix = mw::pt2d_i(viewport(c)),
                dpix = mw::pt2d_i(viewport(d));
          const SDL_Rect rect {
            apix.x, // x-offs
            apix.y, // y-offs
            bpix.x - apix.x, // width
            cpix.y - bpix.y  // height
          };
          const div_t qr = div(roomcnt, colors.size());
          const mw::color_rgba rgba {htonl(colors[qr.rem])};
          SDL_SetRenderDrawColor(m_sdl.get_renderer(), rgba.r, rgba.g, rgba.b, 0x20);
          SDL_BlendMode oldblendmode;
          SDL_GetRenderDrawBlendMode(m_sdl.get_renderer(), &oldblendmode);
          SDL_SetRenderDrawBlendMode(m_sdl.get_renderer(), SDL_BLENDMODE_BLEND);
          SDL_RenderFillRect(m_sdl.get_renderer(), &rect);
          SDL_SetRenderDrawBlendMode(m_sdl.get_renderer(), oldblendmode);

          for (const std::vector<size_t> &contour : room_contours(roomid))
          {
            std::vector<SDL_Point> points;
            for (const size_t vertexidx : contour)
            {
              const mw::pt2d_d pix = viewport(m_vertices[vertexidx]);
              points.push_back(SDL_Point {int(pix.x - 2), int(pix.y - 2)});
              circleRGBA(m_sdl.get_renderer(), pix.x - 3, pix.y - 3, 2,
                  0xFF, 0x00, 0x00, 0xFF);
            }
            points.push_back(points.front());
            SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0xFF, 0x00, 0x00, 0xFF);
            SDL_RenderDrawLines(m_sdl.get_renderer(), points.data(), points.size());
          }
        }
      }
      roomcnt += 1;
    }

    //if (clock::now() - tstart < 6s)
      //return;

    for (size_t i = 0; i < m_vertices.size(); ++i)
    {
      for (size_t j = i+1; j < m_vertices.size(); ++j)
      {
        if (m_walls[wall_index(i, j)])
        {
          const auto [x1pix, y1pix] = viewport(m_vertices[i]);
          const auto [x2pix, y2pix] = viewport(m_vertices[j]);
          lineRGBA(m_sdl.get_renderer(), x1pix, y1pix, x2pix, y2pix,
                   0xFF, 0xFF, 0xFF, 0xFF);
        }
      }
    }
  }

  int
  send(const std::string &what, const std::any &data) override
  { return 0; }
};


static int
the_main_v2(int argc, char **argv)
{
  const double nxlines = 10;
  const double nylines = 10;

  mw::video_manager& vman = mw::video_manager::instance();

  mw::sdl_environment &sdl = vman.get_sdl();

  // wrap GUI into a menu to simplify events
  std::shared_ptr<mw::basic_menu> menu =
    std::make_shared<mw::basic_menu>(sdl, new gui {sdl});

  mw::ui_manager uiman {sdl};
  uiman.add_layer(menu);
  uiman.run();

  return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
  eth::init(&argc);

  mw::central_config::set_config_file_path("./config.eth");
  mw::video_config::use_central_config(true);
  mw::color_manager::use_central_config(true);
  int ret = the_main_v2(argc, argv);

  eth::cleanup();
  return ret;
}
