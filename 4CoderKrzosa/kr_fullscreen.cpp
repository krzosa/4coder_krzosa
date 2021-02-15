global bool global_is_fullscreen_split;

function View_ID get_next_view_if_compilation(Application_Links *app, View_ID view)
{
  Scratch_Block scratch(app);
  Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer);
  if(string_match(buffer_name, SCu8("*compilation*")))
  {
    view = get_next_view_looped_all_panels(app, view, Access_Always);
  }
  
  return view;
}

CUSTOM_COMMAND_SIG(split_fullscreen_mode)
CUSTOM_DOC("Toggle with 2 modes")
{
  if(!global_is_fullscreen_split)
  {
    // global_config.show_line_number_margins = 1;
    // NOTE: Open second panel
    View_ID view = get_active_view(app, Access_Always);
    view = get_next_view_if_compilation(app, view);
    View_ID new_view = open_view(app, view, ViewSplit_Right);
    new_view_settings(app, new_view);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    view_set_buffer(app, new_view, buffer, 0);
  }
  else
  {
    
    // global_config.show_line_number_margins = 0;
    // NOTE: Close second panel
    View_ID view = get_active_view(app, Access_Always);
    view = get_next_view_looped_all_panels(app, view, Access_Always);
    view = get_next_view_if_compilation(app, view);
    view_close(app, view);
  }
  
  global_is_fullscreen_split = !global_is_fullscreen_split;
  toggle_fullscreen(app);
}