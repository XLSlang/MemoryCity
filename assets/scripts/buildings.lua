-- 这是一个全局表，用来存放所有的模型绘制函数
-- 键是物品 ID，值是对应的绘制函数
BuildingModels = BuildingModels or {}

-- 注册模型的方法，给 models/ 里的脚本调用
function RegisterModel(id, draw_func)
    BuildingModels[id] = draw_func
end

-- C 代码调用入口
function draw_building(cr, sx, sy, zoom, id)
    -- 根据 ID 查表，如果找到了对应的模型，就调用它
    local draw_func = BuildingModels[id]
    if draw_func then
        draw_func(cr, sx, sy, zoom)
    end
end