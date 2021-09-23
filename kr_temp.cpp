
// --------- Mouse fix (for bottom bar) and select on click --------- //
// fix: add one line to line_number
function i64
krz_fix_view_pos_from_xy(Application_Links *app, View_ID view, Vec2_f32 p){
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  Rect_f32 region = view_get_buffer_region(app, view);
  f32 width = rect_width(region);
  Face_ID face_id = get_face_id(app, buffer);
  Buffer_Scroll scroll_vars = view_get_buffer_scroll(app, view);
  i64 line = scroll_vars.position.line_number + 1;
  p = (p - region.p0) + scroll_vars.position.pixel_shift;
  return(buffer_pos_at_relative_xy(app, buffer, width, face_id, line, p));
}

global bool is_selected;
CUSTOM_COMMAND_SIG(krz_click_set_cursor_and_mark)
CUSTOM_DOC("Sets the cursor position and mark to the mouse position.")
{
  View_ID view = get_active_view(app, Access_ReadVisible);
  Mouse_State mouse = get_mouse_state(app);
  i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
  
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  
  // NOTE: Select on click yeee
  Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
  
  select_scope(app, view, range);
  is_selected = true;
}

CUSTOM_COMMAND_SIG(krz_click_set_cursor_if_lbutton)
CUSTOM_DOC("If the mouse left button is pressed, sets the cursor position to the mouse position.")
{
  View_ID view = get_active_view(app, Access_ReadVisible);
  Mouse_State mouse = get_mouse_state(app);
  if (mouse.l){
    i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
    
    // NOTE: Select on click yeee
    if(is_selected)
    {
      view_set_mark(app, view, seek_pos(pos));
      is_selected = false;
    }
  }
  no_mark_snap_to_cursor(app, view);
  set_next_rewrite(app, view, Rewrite_NoChange);
}
