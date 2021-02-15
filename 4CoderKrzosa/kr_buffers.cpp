b32 global_compilation_view_expanded = 0;
View_ID global_compilation_view = 0;
f32 global_compilation_small_buffer_size = 4.f;

CUSTOM_COMMAND_SIG(f4_toggle_compilation_expand) // 
CUSTOM_DOC("Expand the compilation window.") 
{
  Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
  Face_ID face_id = get_face_id(app, buffer);
  Face_Metrics metrics = get_face_metrics(app, face_id);
  if(global_compilation_view_expanded ^= 1)
  {
    view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*32.f));
  }
  else
  {
    view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*global_compilation_small_buffer_size));
  }
}

function void
InitPanels(Application_Links *app) // Needs to be called in the startup function, sets up the compilation panel and panel split
{
  Buffer_ID comp_buffer = create_buffer(app, string_u8_litexpr("*compilation*"),
                                        BufferCreate_NeverAttachToFile |
                                        BufferCreate_AlwaysNew);
  buffer_set_setting(app, comp_buffer, BufferSetting_Unimportant, true);
  buffer_set_setting(app, comp_buffer, BufferSetting_ReadOnly, true);
  
  Buffer_Identifier comp = buffer_identifier(string_u8_litexpr("*compilation*"));
  Buffer_Identifier left  = buffer_identifier(string_u8_litexpr("*messages*"));
  //Buffer_Identifier right = buffer_identifier(string_u8_litexpr("*scratch*"));
  Buffer_ID comp_id = buffer_identifier_to_id(app, comp);
  Buffer_ID left_id = buffer_identifier_to_id(app, left);
  //Buffer_ID right_id = buffer_identifier_to_id(app, right);
  
  // NOTE(rjf): Left Panel
  View_ID view = get_active_view(app, Access_Always);
  new_view_settings(app, view);
  view_set_buffer(app, view, left_id, 0);
  
  // NOTE(rjf): Bottom panel
  View_ID compilation_view = 0;
  {
    compilation_view = open_view(app, view, ViewSplit_Bottom);
    new_view_settings(app, compilation_view);
    Buffer_ID buffer = view_get_buffer(app, compilation_view, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    view_set_split_pixel_size(app, compilation_view, (i32)(metrics.line_height*global_compilation_small_buffer_size));
    view_set_passive(app, compilation_view, true);
  }
  view_set_buffer(app, compilation_view, comp_id, 0);
  view_set_active(app, view);
  global_compilation_view = compilation_view;
  
  // NOTE(rjf): Right Panel
  //open_panel_vsplit(app);
  
  //View_ID right_view = get_active_view(app, Access_Always);
  //view_set_buffer(app, right_view, right_id, 0);
  
  // NOTE(rjf): Restore Active to Left
  view_set_active(app, view);
}
