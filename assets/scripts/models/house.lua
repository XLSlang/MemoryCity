local function draw_house(cr, sx, sy, zoom)
    local tw = 64.0 * zoom
    local th = 32.0 * zoom
    local hw = tw / 2.0
    local hh = th / 2.0
    local height = 40.0 * zoom

    -- 左侧墙
    set_rgb(cr, 0.50, 0.30, 0.20)
    move_to(cr, sx - hw, sy + hh) line_to(cr, sx, sy + th) line_to(cr, sx, sy + th - height) line_to(cr, sx - hw, sy + hh - height) close_path(cr) fill_preserve(cr)
    set_rgba(cr, 0, 0, 0, 0.4) set_line_width(cr, 1.0) stroke(cr)

    -- 右侧墙
    set_rgb(cr, 0.65, 0.40, 0.25)
    move_to(cr, sx, sy + th) line_to(cr, sx + hw, sy + hh) line_to(cr, sx + hw, sy + hh - height) line_to(cr, sx, sy + th - height) close_path(cr) fill_preserve(cr)
    set_rgba(cr, 0, 0, 0, 0.4) stroke(cr)

    -- 屋顶
    set_rgb(cr, 0.80, 0.30, 0.25)
    move_to(cr, sx, sy - height) line_to(cr, sx + hw, sy + hh - height) line_to(cr, sx, sy + th - height) line_to(cr, sx - hw, sy + hh - height) close_path(cr) fill_preserve(cr)
    set_rgba(cr, 0, 0, 0, 0.5) stroke(cr)
end

-- 将 house 和 cottage 都绑定到这个函数上
RegisterModel("house", draw_house)
RegisterModel("cottage", draw_house)