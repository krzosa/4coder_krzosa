

function Buffer_ID
_create_or_switch_to_buffer_and_clear_by_name(Application_Links *app, String_Const_u8 name_string, View_ID default_target_view){
  Buffer_ID search_buffer = get_buffer_by_name(app, name_string, Access_Always);
  if (search_buffer != 0){
    View_ID target_view = default_target_view;
    
    View_ID view_with_buffer_already_open = get_first_view_with_buffer(app, search_buffer);
    if (view_with_buffer_already_open != 0){
      target_view = view_with_buffer_already_open;
      // TODO(allen): there needs to be something like
      // view_exit_to_base_context(app, target_view);
      //view_end_ui_mode(app, target_view);
    }
    else{
      view_set_buffer(app, target_view, search_buffer, 0);
    }
    view_set_active(app, target_view);
    
    clear_buffer(app, search_buffer);
    buffer_send_end_signal(app, search_buffer);
  }
  else{
    search_buffer = create_buffer(app, name_string, BufferCreate_AlwaysNew);
    view_set_buffer(app, default_target_view, search_buffer, 0);
    view_set_active(app, default_target_view);
  }
  
  return(search_buffer);
}


CUSTOM_COMMAND_SIG(test1)
CUSTOM_DOC("Search all matches and allow to replace them in search buffer")
{
  Scratch_Block scratch(app);
  Query_Bar_Group group(app);
  u8 string_space[256];
  Query_Bar bar = {};
  bar.prompt = string_u8_litexpr("Search: ");
  bar.string = SCu8(string_space, (u64)0);
  bar.string_capacity = sizeof(string_space);
  if (query_user_string(app, &bar)){
    String_Match_List matches = find_all_matches_all_buffers(app, scratch, bar.string, StringMatch_CaseSensitive, 0);
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = _create_or_switch_to_buffer_and_clear_by_name(app, string_u8_litexpr("*search_replace*"), view);
    print_string_match_list_to_buffer(app, buffer, matches);
    buffer_set_setting(app, buffer, BufferSetting_ReadOnly, false);
    buffer_set_setting(app, buffer, BufferSetting_Unimportant, true);
  }
}

CUSTOM_COMMAND_SIG(test2)
CUSTOM_DOC("Replace all lines in search buffer")
{
  Scratch_Block scratch(app);
  View_ID view = get_active_view(app, Access_Always);
  Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  String_Const_u8 name = push_buffer_base_name(app, scratch, buffer);
  print_message(app, name);
  if(!string_match(name, string_u8_litexpr("*search_replace*"))) return;
  String_Const_u8 string = push_whole_buffer(app, scratch, buffer);
  
  
  for(u64 i = 0; i < string.size;) {
    
    String_Const_u8 file = {&string.str[i]};
    b64 is_path_delimiter = true;
    while(i < string.size) {
      if(string.str[i] == ':') {
        if(is_path_delimiter) is_path_delimiter = false;
        else break;
      }
      i++;
      file.size++;
    }
    i++;
    
    String_Const_u8 line = {&string.str[i]};
    while(string.str[i] != ':' && i < string.size) {
      i++;
      line.size++;
    }
    i++;
    
    String_Const_u8 column = {&string.str[i]};
    while(string.str[i] != ':' && i < string.size) {
      column.size++;
      i++;
    }
    i++;
    
    if(string.str[i] == ' ') i++;
    
    String_Const_u8 content = {&string.str[i]};
    while(string.str[i] != '\n' && i < string.size) {
      i++;
      content.size++;
    }
    
    while(i < string.size && (string.str[i] == '\n' || string.str[i] == ' ' || string.str[i] == '\t')) {
      i++;
    }
    
    if(file.str[0] != '*') {
      Buffer_ID replace_buff = get_buffer_by_file_name(app, file, AccessFlag_Write);
      u64 line_number = string_to_integer(line, 10);
      Range_i64 range = get_line_pos_range(app, replace_buff, line_number);
      buffer_replace_range(app, replace_buff, range, content);
    }
    
    
  }
}