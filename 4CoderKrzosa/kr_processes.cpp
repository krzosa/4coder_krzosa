struct FileInfoForCommand
{
  View_ID view;
  Buffer_ID buffer;
  Buffer_Identifier buffer_id;
  String_Const_u8 curr_file;
  String_Const_u8 hot_dir;
  i64 line_number;
};

function FileInfoForCommand
FileInfoForCommandGet(Application_Links *app, Arena *scratch)
{
  FileInfoForCommand result;
  result.view = get_active_view(app, Access_Always);
  result.buffer = view_get_buffer(app, global_compilation_view, Access_Always);
  result.buffer_id = buffer_identifier(result.buffer);
  Buffer_ID buffer_curr_file = view_get_buffer(app, result.view, Access_Always);
  
  result.curr_file = push_buffer_file_name(app, scratch, buffer_curr_file);
  i64 cursor_pos = view_get_cursor_pos(app, result.view);
  result.line_number = get_line_number_from_pos(app, buffer_curr_file, cursor_pos);
  result.hot_dir = push_hot_directory(app, scratch);
  return result;
}

internal bool
string_starts_with(String_Const_u8 a, String_Const_u8 starts_with) {
  if(a.size < starts_with.size) return false;
  for(u64 i = 0; i < starts_with.size; i++) {
    if(a.str[i] != starts_with.str[i]) return false;
  }
  return true;
}

internal void
exec_commandf(Application_Links *app, String_Const_u8 cmd) {
  u8 buff[2048];
  String_u8 str = {buff, 0, 2048};
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  for(u64 i = 0; i < cmd.size; i++) {
    String_Const_u8 matcher = {cmd.str+i, cmd.size-i};
    if(string_starts_with(matcher, string_u8_litexpr("{file}"))) {
      string_append(&str, info.curr_file);
      i+=5;
    } 
    else if(string_starts_with(matcher, string_u8_litexpr("{line}"))) {
      String_Const_u8 number = push_stringf(scratch, "%d", info.line_number);
      string_append(&str, number);
      i+=5;
    }
    else {
      string_append_character(&str, cmd.str[i]);
    }
  }
  
  //String_Const_u8 cmd = push_stringf(scratch, "clang -W -Wall -g %.*s", string_expand(info.curr_file));
  print_message(app, str.string);
  print_message(app, string_u8_litexpr("\n"));
  if(cmd.size)
  {
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, str.string, CLI_OverlapWithConflict|CLI_CursorAtEnd|CLI_SendEndSignal);
  }
}

CUSTOM_UI_COMMAND_SIG(open_debugger)
CUSTOM_DOC("Interactively opens a debugger.")
{
  for (;;){
    Scratch_Block scratch(app);
    View_ID view = get_this_ctx_view(app, Access_Always);
    File_Name_Result result = get_file_name_from_user(app, scratch, "Open:", view);
    if (result.canceled) break;
    
    String_Const_u8 file_name = result.file_name_activated;
    if (file_name.size == 0) break;
    
    String_Const_u8 path = result.path_in_text_field;
    String_Const_u8 full_file_name =
      push_u8_stringf(scratch, "%.*s/%.*s",
                      string_expand(path), string_expand(file_name));
    
    if (result.is_folder){
      set_hot_directory(app, full_file_name);
      continue;
    }
    
    if (character_is_slash(file_name.str[file_name.size - 1])){
      File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
      if (HasFlag(attribs.flags, FileAttribute_IsDirectory)){
        set_hot_directory(app, full_file_name);
        continue;
      }
      if (query_create_folder(app, file_name)){
        set_hot_directory(app, full_file_name);
        continue;
      }
      break;
    }
    
    
    String_Const_u8 cmd = push_stringf(scratch, "rbg.exe %.*s", string_expand(full_file_name));
    print_message(app, cmd);
    print_message(app, string_u8_litexpr("\n"));
    Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
    Buffer_Identifier buffer_identi = buffer_identifier(buffer);
    if(cmd.size)
    {
      String_Const_u8 hot_dir = push_hot_directory(app, scratch);
      exec_system_command(app, global_compilation_view, buffer_identi, hot_dir, cmd, 0);
    }
    break;
  }
}

CUSTOM_COMMAND_SIG(compile_current_file)
CUSTOM_DOC("Compile current file with clang")
{
  Scratch_Block scratch(app);
  String_Const_u8 command = def_get_config_string(scratch, vars_save_string_lit("compile_command"));
  exec_commandf(app, command);
}

CUSTOM_COMMAND_SIG(debugger_start_debug)
CUSTOM_DOC("Open the app in remedybg")
{
  exec_commandf(app, string_u8_litexpr("rbg.exe start-debugging"));
}

CUSTOM_COMMAND_SIG(debugger_stop_debug)
CUSTOM_DOC("Open the app in remedybg")
{
  exec_commandf(app, string_u8_litexpr("rbg.exe stop-debugging"));
}

CUSTOM_COMMAND_SIG(debugger_continue)
CUSTOM_DOC("Continue execution")
{
  exec_commandf(app, string_u8_litexpr("rbg.exe continue-execution"));
}

CUSTOM_COMMAND_SIG(debugger_open_file_at_cursor)
CUSTOM_DOC("Open current file in debugger")
{
  exec_commandf(app, string_u8_litexpr("rbg.exe open-file {file} {line}"));
}

CUSTOM_COMMAND_SIG(debugger_breakpoint_at_cursor)
CUSTOM_DOC("Set remedybg breakpoint at cursor")
{
  exec_commandf(app, string_u8_litexpr("rbg.exe add-breakpoint-at-file {file} {line}"));
}

CUSTOM_COMMAND_SIG(debugger_remove_breakpoint_at_cursor)
CUSTOM_DOC("Remove remedybg breakpoint at cursor")
{
  exec_commandf(app, string_u8_litexpr("rbg.exe remove-breakpoint-at-file {file} {line}"));
}

CUSTOM_COMMAND_SIG(run_build_cpp)
CUSTOM_DOC("Compile project using build.cpp")
{
  Scratch_Block scratch(app);
  String_Const_u8 command = def_get_config_string(scratch, vars_save_string_lit("build_command"));
  exec_commandf(app, command);
}

function void open_file_in_4coder_dir(Application_Links *app, String_Const_u8 file)
{
  View_ID active_view = get_active_view(app, Access_Always);
  Scratch_Block scratch(app);
  String_Const_u8 binary = system_get_path(scratch, SystemPath_Binary);
  String_u8 path = string_u8_push(scratch, 255);
  string_append(&path, binary);
  string_append(&path, file);
  view_open_file(app, active_view, path.string, false);
}

CUSTOM_COMMAND_SIG(open_bindings)
CUSTOM_DOC("Open hotkey file")
{
  open_file_in_4coder_dir(app, SCu8("bindings.4coder"));
}

CUSTOM_COMMAND_SIG(open_theme)
CUSTOM_DOC("Open theme file")
{
  open_file_in_4coder_dir(app, SCu8("themes/kr.4coder"));
}

CUSTOM_COMMAND_SIG(explorer_here)
CUSTOM_DOC("runs explorer in current dir")
{
  exec_commandf(app, string_u8_litexpr("explorer.exe ."));
}

CUSTOM_COMMAND_SIG(open_4coder_source)
CUSTOM_DOC("Opens 4coder in 4coder source directory")
{
  Scratch_Block scratch(app);
  String_Const_u8 prev_dir = push_hot_directory(app, scratch);
  String_Const_u8 binary = system_get_path(scratch, SystemPath_Binary);
  set_hot_directory(app, binary);
  exec_commandf(app, string_u8_litexpr("4ed.exe"));
  //String_Const_u8 cmd = push_stringf(scratch, "start cmd.exe %s", binary.size, binary.str);
  //exec_commandf(app, cmd);
  set_hot_directory(app, prev_dir);
}