/* Guide

1. You need to #include this file in your custom layer (4coder_default_bindings.cpp by default) below the line: "#include "4coder_default_include.cpp""

2. In your "void custom_layer_init(Application_Links *app)",
set "painter_whole_screen_render_caller" as as HookID_WholeScreenRenderCaller

"""
  // NOTE(allen): default hooks and command maps
  set_all_default_hooks(app);
  set_custom_hook(app, HookID_RenderCaller, RenderCaller);
  set_custom_hook(app, HookID_WholeScreenRenderCaller, painter_whole_screen_render_caller);
  mapping_init(tctx, &framework_mapping);
"""

3. Bind painting commands to hotkeys (Those are at the end of the file)
4. Done!*/
struct brush_in_time
{
  Vec2_i32 p;
  b8 mouse_l;
};

#define max_size_of_array 10000
global b32 painter_initialized = false;
global b32 painter_mode = false;
global brush_in_time *brush_strokes;
global i64 brush_strokes_size;
global i32 brush_size = 20;
global i32 brush_size_control = 5;

function void
painter_init()
{
  brush_strokes = (brush_in_time *)heap_allocate(&global_heap, sizeof(brush_in_time) * max_size_of_array);
  painter_initialized = true;
}


function void
painter_whole_screen_render_caller(Application_Links *app, Frame_Info frame_info){
  if(!painter_mode) return;
  
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
    if(brush_strokes_size < max_size_of_array)
    {
      brush_strokes[brush_strokes_size].mouse_l = mouse.l;
      brush_strokes[brush_strokes_size++].p = mouse.p;
    }
  }
  else if(mouse.l == 0 && prev_mouse_state == 1)
  {
    if(brush_strokes_size < max_size_of_array)
    {
      brush_strokes[brush_strokes_size].mouse_l = mouse.l;
      brush_strokes[brush_strokes_size++].p = mouse.p;
    }
  }
  prev_mouse_state = mouse.l;
  
  if(brush_strokes_size > 1)
  {
    for(i32 j = 1; j < brush_strokes_size; j++)
    {
      // Special case for a single dot in the wild
      if(brush_strokes[j-1].mouse_l == 0 || brush_strokes[j].mouse_l == 1)
      {
        Rect_f32 rect = {(f32)brush_strokes[j].p.x - (brush_size / 2),
          (f32)brush_strokes[j].p.y - (brush_size / 2),
          (f32)brush_strokes[j].p.x + brush_size / 2,
          (f32)brush_strokes[j].p.y + brush_size / 2};
        draw_rectangle_fcolor(app, rect, 10.f, fcolor_id(defcolor_text_default));
      }
      // Skip if mouse not pressed
      if(brush_strokes[j-1].mouse_l == false || brush_strokes[j].mouse_l == false)
        continue;
      
      Vec2_i32 minP = brush_strokes[j-1].p;
      Vec2_i32 maxP = brush_strokes[j].p;
      
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
        
        Rect_f32 rect = {(f32)pos.x - (brush_size / 2),
          (f32)pos.y - (brush_size / 2),
          (f32)pos.x + brush_size / 2,
          (f32)pos.y + brush_size / 2};
        draw_rectangle_fcolor(app, rect, 10.f, fcolor_id(defcolor_text_default));
      }
      
    }
    
  }
}

CUSTOM_COMMAND_SIG(painter_mode_switch)
CUSTOM_DOC("Painter Mode Switch !")
{
  if(painter_initialized == false) painter_init();
  painter_mode = !painter_mode;
}

CUSTOM_COMMAND_SIG(painter_mode_clear)
CUSTOM_DOC("Clear the paint")
{
  brush_strokes_size = 0;
}

CUSTOM_COMMAND_SIG(painter_mode_brush_size_lower)
CUSTOM_DOC("Brush size lower")
{
  if (brush_size > brush_size_control) brush_size -= brush_size_control;
}

CUSTOM_COMMAND_SIG(painter_mode_brush_size_upper)
CUSTOM_DOC("Brush size upper")
{
  brush_size += brush_size_control;
}
