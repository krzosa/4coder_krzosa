#define L(...) string_u8_litexpr(__VA_ARGS__)
#define E(...) string_expand(__VA_ARGS__)
typedef String_Const_u8 S8;
struct FileInfoForCommand
{
  View_ID view;
  Buffer_ID buffer;
  Buffer_Identifier buffer_id;
  S8 curr_file;
  S8 hot_dir;
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
string_starts_with(S8 a, S8 starts_with) {
  if(a.size < starts_with.size) return false;
  for(u64 i = 0; i < starts_with.size; i++) {
    if(a.str[i] != starts_with.str[i]) return false;
  }
  return true;
}

internal void
exec_commandf(Application_Links *app, S8 cmd) {
  u8 buff[2048];
  String_u8 str = {buff, 0, 2048};
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  for(u64 i = 0; i < cmd.size; i++) {
    S8 matcher = {cmd.str+i, cmd.size-i};
    if(string_starts_with(matcher, string_u8_litexpr("{file}"))) {
      string_append(&str, info.curr_file);
      i+=5;
    } 
    else if(string_starts_with(matcher, string_u8_litexpr("{line}"))) {
      S8 number = push_stringf(scratch, "%d", info.line_number);
      string_append(&str, number);
      i+=5;
    }
    else {
      string_append_character(&str, cmd.str[i]);
    }
  }
  
  //S8 cmd = push_stringf(scratch, "clang -W -Wall -g %.*s", string_expand(info.curr_file));
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
    
    S8 file_name = result.file_name_activated;
    if (file_name.size == 0) break;
    
    S8 path = result.path_in_text_field;
    S8 full_file_name =
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
    
    
    S8 cmd = push_stringf(scratch, "rbg.exe %.*s", string_expand(full_file_name));
    print_message(app, cmd);
    print_message(app, string_u8_litexpr("\n"));
    Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
    Buffer_Identifier buffer_identi = buffer_identifier(buffer);
    if(cmd.size)
    {
      S8 hot_dir = push_hot_directory(app, scratch);
      exec_system_command(app, global_compilation_view, buffer_identi, hot_dir, cmd, 0);
    }
    break;
  }
}

CUSTOM_COMMAND_SIG(compile_current_file)
CUSTOM_DOC("Compile current file with clang")
{
  Scratch_Block scratch(app);
  S8 command = def_get_config_string(scratch, vars_save_string_lit("compile_command"));
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
  S8 command = def_get_config_string(scratch, vars_save_string_lit("build_command"));
  exec_commandf(app, command);
}

function void open_file_in_4coder_dir(Application_Links *app, S8 file)
{
  View_ID active_view = get_active_view(app, Access_Always);
  Scratch_Block scratch(app);
  S8 binary = system_get_path(scratch, SystemPath_Binary);
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

CUSTOM_COMMAND_SIG(projects_list)
CUSTOM_DOC("Opens a project list")
{
  Scratch_Block scratch(app);
  S8 binary = system_get_path(scratch, SystemPath_Binary);
  S8 path = push_stringf(scratch, "%.*sprojects.txt\0", binary.size, binary.str);
  FILE *file = fopen((char *)path.str, "rb");
  if(file) {
    S8 data = dump_file_handle(scratch, file);
    List_String_Const_u8 list = string_split(scratch, data, (u8 *)"\n", 1);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, L("project path: "));
    lister_set_default_handlers(lister);
    
    for(Node_String_Const_u8 *node = list.first; node; node=node->next) {
      S8 str = node->string;
      lister_add_item(lister, str, str, (void *)&node->string, 0);
    }
    Lister_Result result = run_lister(app, lister);
    if(!result.canceled) {
      S8 *ptr = (String8 *)result.user_data;
      close_all_code(app);
      set_hot_directory(app, *ptr);
      open_all_code_recursive(app);
    }
  }
  else {
    log_string(app, L("Failed to fetch project file list for reading"));
  }
}

CUSTOM_COMMAND_SIG(add_folder_to_project_list)
CUSTOM_DOC("Opens a project list")
{
  Scratch_Block scratch(app);
  S8 binary = system_get_path(scratch, SystemPath_Binary);
  S8 path = push_stringf(scratch, "%.*sprojects.txt\0", binary.size, binary.str);
  FILE *file = fopen((char *)path.str, "rb");
  S8 data = L("");
  if(file) {
    data = dump_file_handle(scratch, file);
    fclose(file);
  }
  
  file = fopen((char *)path.str, "w");
  if(file) {
    S8 hot = push_hot_directory(app, scratch);
    S8 combined_power_of_will = push_stringf(scratch, "%.*s\n%.*s", E(data), E(hot));
    fwrite(combined_power_of_will.str, 1, combined_power_of_will.size, file);
    fclose(file);
  }
}
