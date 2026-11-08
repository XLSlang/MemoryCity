local function draw_tree(cr, sx, sy, zoom)
    local tw = 64.0 * zoom
    local th = 32.0 * zoom
    local cx = sx
    local cy = sy + th / 2.0
    local trunk_w = 6.0 * zoom
    local trunk_h = 20.0 * zoom
    local crown_r = 16.0 * zoom
    local crown_y = cy - trunk_h - crown_r * 0.4

    -- 树干
    set_rgb(cr, 0.45, 0.30, 0.15)
    move_to(cr, cx - trunk_w/2, cy) line_to(cr, cx + trunk_w/2, cy) line_to(cr, cx + trunk_w/2, cy - trunk_h) line_to(cr, cx - trunk_w/2, cy - trunk_h) close_path(cr) fill_preserve(cr)
    set_rgba(cr, 0, 0, 0, 0.35) set_line_width(cr, 1.0 * zoom) stroke(cr)

    -- 下层树冠
    set_rgb(cr, 0.20, 0.55, 0.20)
    move_to(cr, cx - crown_r, crown_y + crown_r * 0.6) line_to(cr, cx + crown_r, crown_y + crown_r * 0.6) line_to(cr, cx, crown_y - crown_r * 0.2) close_path(cr) fill_preserve(cr)
    set_rgba(cr, 0, 0, 0, 0.35) stroke(cr)

    -- 上层树冠
    set_rgb(cr, 0.25, 0.65, 0.25)
    move_to(cr, cx - crown_r * 0.8, crown_y - crown_r * 0.1) line_to(cr, cx + crown_r * 0.8, crown_y - crown_r * 0.1) line_to(cr, cx, crown_y - crown_r * 0.9) close_path(cr) fill_preserve(cr)
    set_rgba(cr, 0, 0, 0, 0.35) stroke(cr)
end

-- 将 pine 和 tree 都绑定到这个函数上
RegisterModel("tree", draw_tree)
RegisterModel("pine", draw_tree)