b32 global_compilation_view_expanded = 0;
View_ID global_compilation_view = 0;
f32 global_compilation_small_buffer_size = 8.f;

CUSTOM_COMMAND_SIG(expand_compilation_window) 
CUSTOM_DOC("Expand the compilation window.") 
{
  Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
  Face_ID face_id = get_face_id(app, buffer);
  Face_Metrics metrics = get_face_metrics(app, face_id);
  if(global_compilation_view_expanded ^= 1)
  {
    view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*32.f));
    view_set_active(app, global_compilation_view);
  }
  else
  {
    view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*global_compilation_small_buffer_size));
    change_active_panel_send_command(app, 0);
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

function bool is_lower_case(char c){ return(c >= 'a' && c <= 'z'); }
function bool is_upper_case(char c){ return(c >= 'A' && c <= 'Z'); }
char to_lower_case_c(char c){ if(is_upper_case(c)) { c += 32; } return c; }
char to_upper_case_c(char c){ if(is_lower_case(c)) { c -= 32; } return c; }
function void SnakeCaseToPascalCase(String_Const_u8 *string)
{
  u64 length = string->size;
  string->str[0] = to_upper_case_c(string->str[0]);
  u64 deleted_letters = 0 ;
  for(u64 i = 0; string->str[i] != '\0'; i++)
  {
    if(string->str[i] == '_')
    {
      if(i > 0 && is_upper_case(string->str[i-1])) continue;
      if(string->str[i+1] != '\0' && is_upper_case(string->str[i+1])) continue;
      
      string->str[i+1] = to_upper_case_c(string->str[i+1]);
      block_copy(&string->str[i], &string->str[i+1], length - i);
      deleted_letters+=1;
    }
  }
  string->size -= deleted_letters;
}

CUSTOM_COMMAND_SIG(snake_case_to_pascal_case)
CUSTOM_DOC("Snake case to pascal case")
{
  View_ID view = get_active_view(app, Access_Always);
  Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  Scratch_Block scratch(app);
  Token_Array token_array = get_token_array_from_buffer(app, buffer);
  for(int i = 0; i < token_array.count; i++)
  {
    Token *token = &token_array.tokens[i];
    String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);
    Code_Index_Note *note = code_index_note_from_string(lexeme);
    if(note != 0)
    {
      if(note->file && note->note_kind == CodeIndexNote_Function)
      {
        Range_i64 range = {token->pos, token->pos + token->size};
        SnakeCaseToPascalCase(&lexeme);
        buffer_replace_range(app,buffer,range,lexeme);
      }
    }
  }
}

CUSTOM_COMMAND_SIG(keyboard_macro_switch)
CUSTOM_DOC("Start stop macro")
{
  if (get_current_input_is_virtual(app)) return;
  if(global_keyboard_macro_is_recording)
  {
    Buffer_ID buffer = get_keyboard_log_buffer(app);
    global_keyboard_macro_is_recording = false;
    i64 end = buffer_get_size(app, buffer);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(end));
    Buffer_Cursor back_cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line - 1, 1));
    global_keyboard_macro_range.one_past_last = back_cursor.pos;
  }
  else
  {
    Buffer_ID buffer = get_keyboard_log_buffer(app);
    global_keyboard_macro_is_recording = true;
    global_keyboard_macro_range.first = buffer_get_size(app, buffer);
  }
}

