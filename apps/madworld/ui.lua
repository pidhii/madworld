
epsilon = 1e-7


function alignAtCenter(chld, root)
  root = root or chld.select("..")[0]

  local rw, rh = root.get_dimentions()
  local cw, ch = chld.get_dimentions()

  local hpad = (rw - cw) / 2
  if hpad > epsilon then
    chld.set_left_margin(hpad)
    chld.set_right_margin(hpad)
  end
end


function addFinisher(o, newfinisher)
  -- get old finsher function
  local oldfinisher = o.finisher or function () end
  -- set a finisher that will call both old and new handles
  function o:finish()
    oldfinisher(o)
    newfinisher(o)
  end
end


EnterGameButton = {}
EnterGameButton.__index = EnterGameButton
function EnterGameButton:new(o)
  setmetatable(o, self)

  o.on("clicked", function ()
    local map = generate_map {seed=0}
    local player = create_player {position={50, 25}}
    local npc = create_npc {name="friend", position={25, 30}}
    run_game(map, player, 20, {npc})
  end)

  return o
end


SettingsButton = {}
SettingsButton.__index = SettingsButton
function SettingsButton:new(o)
  setmetatable(o, self)

  o.on("clicked", function ()
    open_settings()
  end)

  return o
end


QuitButton = {}
QuitButton.__index = QuitButton
function QuitButton:new(o)
  setmetatable(o, self)

  o.on("clicked", function ()
    alignAtCenter(o)
  end)

  return o
end

