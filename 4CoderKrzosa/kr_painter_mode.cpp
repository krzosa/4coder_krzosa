/* Guide

1. You need to #include this file in your custom layer (4coder_default_bindings.cpp by default) below the line: "#include "4coder_default_include.cpp""

2. In your "void custom_layer_init(Application_Links *app)",
set "painter_whole_screen_render_caller" as as HookID_WholeScreenRenderCaller
like this:

"""
    // NOTE(allen): default hooks and command maps
  set_all_default_hooks(app);
    set_custom_hook(app, HookID_RenderCaller, RenderCaller);
  set_custom_hook(app, HookID_WholeScreenRenderCaller, painter_whole_screen_render_caller);
  mapping_init(tctx, &framework_mapping);
"""

3. Bind painting commands to hotkeys (Those are at the end of the file)
4. Done!*/
#define KR_PAINTER_MODE

struct brush_in_time
{
  Vec2_i32 p;
  b8 mouse_l;
};

struct Painter
{
  b32 initialized;
  b32 is_painting;
#define painter_max_brush_strokes 10000
  brush_in_time *brush_strokes;
  i64 brush_strokes_size;
  i32 brush_size;
  i32 brush_size_control;
  Managed_ID color;
};
global Painter global_painter = {false, false, 0, 0, 20, 5, defcolor_text_default};

function void
painter_init()
{
  global_painter.brush_strokes = (brush_in_time *)heap_allocate(&global_heap, sizeof(brush_in_time) * painter_max_brush_strokes);
  global_painter.initialized = true;
}


function void
painter_whole_screen_render_caller(Application_Links *app, Frame_Info frame_info){
  if(!global_painter.is_painting) return;
  Painter *p = &global_painter;
  Rect_f32 region = global_get_screen_rectangle(app);
  Vec2_f32 center = rect_center(region);
  
  // Face_ID face_id = get_face_id(app, 0);
  Mouse_State mouse = get_mouse_state(app);
  Scratch_Block scratch(app);
  
  
  // Collect an array of points when mouse was clicked
  // collect the minimum of points when mouse was not clicked
  // to determine when to stop drawing
  static bool prev_mouse_state;
  if(mouse.l)
  {
    if(p->brush_strokes_size < painter_max_brush_strokes)
    {
      p->brush_strokes[p->brush_strokes_size].mouse_l = mouse.l;
      p->brush_strokes[p->brush_strokes_size++].p = mouse.p;
    }
  }
  else if(mouse.l == 0 && prev_mouse_state == 1)
  {
    if(p->brush_strokes_size < painter_max_brush_strokes)
    {
      p->brush_strokes[p->brush_strokes_size].mouse_l = mouse.l;
      p->brush_strokes[p->brush_strokes_size++].p = mouse.p;
    }
  }
  prev_mouse_state = mouse.l;
  
  if(p->brush_strokes_size > 1)
  {
    for(i32 j = 1; j < p->brush_strokes_size; j++)
    {
      // Special case for a single dot in the wild
      if(p->brush_strokes[j-1].mouse_l == 0 || p->brush_strokes[j].mouse_l == 1)
      {
        Rect_f32 rect = {(f32)p->brush_strokes[j].p.x - (p->brush_size / 2),
          (f32)p->brush_strokes[j].p.y - (p->brush_size / 2),
          (f32)p->brush_strokes[j].p.x + p->brush_size / 2,
          (f32)p->brush_strokes[j].p.y + p->brush_size / 2};
        draw_rectangle_fcolor(app, rect, 10.f, fcolor_id(global_painter.color));
      }
      // Skip if mouse not pressed
      if(p->brush_strokes[j-1].mouse_l == false || p->brush_strokes[j].mouse_l == false)
        continue;
      
      Vec2_i32 minP = p->brush_strokes[j-1].p;
      Vec2_i32 maxP = p->brush_strokes[j].p;
      
      // Drawing line algorithm, fill every pixel with a rectangle ! ;>
      bool steep = false;
      if (abs(minP.x-maxP.x) < abs(minP.y-maxP.y))
      {
        i32 temp = minP.x;
        minP.x = minP.y;
        minP.y = temp;
        
        temp = maxP.x;
        maxP.x = maxP.y;
        maxP.y = temp;
        steep = true;
      }
      
      if(minP.x > maxP.x)
      {
        Vec2_i32 temp = minP;
        minP = maxP;
        maxP = temp;
      }
      
      for(i32 x = minP.x; x < maxP.x; x++)
      {
        f32 t = (x - minP.x ) / (f32)(maxP.x - minP.x);
        i32 y = (i32)(minP.y * (1. - t) + maxP.y*t);
        Vec2_i32 pos;
        if(steep)
        {
          pos = {y, x};
        }
        else
        {
          pos = {x, y};
        }
        
        Rect_f32 rect = {(f32)pos.x - (p->brush_size / 2),
          (f32)pos.y - (p->brush_size / 2),
          (f32)pos.x + p->brush_size / 2,
          (f32)pos.y + p->brush_size / 2};
        draw_rectangle_fcolor(app, rect, 10.f, fcolor_id(global_painter.color));
      }
      
    }
    
  }
}

CUSTOM_COMMAND_SIG(painter_mode_on)
CUSTOM_DOC("Painter Mode Switch !")
{
  if(!global_painter.initialized) painter_init();
  global_painter.is_painting = !global_painter.is_painting;
}

CUSTOM_COMMAND_SIG(painter_clear_canvas)
CUSTOM_DOC("Clear the paint")
{
  global_painter.brush_strokes_size = 0;
}

CUSTOM_COMMAND_SIG(painter_brush_size_smaller)
CUSTOM_DOC("Brush size lower")
{
  if (global_painter.brush_size > global_painter.brush_size_control) global_painter.brush_size -= global_painter.brush_size_control;
}

CUSTOM_COMMAND_SIG(painter_brush_bigger)
CUSTOM_DOC("Brush size upper")
{
  global_painter.brush_size += global_painter.brush_size_control;
}