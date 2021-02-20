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

CUSTOM_COMMAND_SIG(debugger_start_debug)
CUSTOM_DOC("Open the app in remedybg")
{
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  String_Const_u8 cmd = push_stringf(scratch, "rbg.exe start-debugging");
  if(cmd.size)
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, cmd, 0);
}

CUSTOM_COMMAND_SIG(debugger_stop_debug)
CUSTOM_DOC("Open the app in remedybg")
{
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  String_Const_u8 cmd = push_stringf(scratch, "rbg.exe stop-debugging");
  if(cmd.size)
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, cmd, 0);
}

CUSTOM_COMMAND_SIG(debugger_continue)
CUSTOM_DOC("Continue execution")
{
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  String_Const_u8 cmd = push_stringf(scratch, "rbg.exe continue-execution");
  if(cmd.size)
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, cmd, 0);
}

CUSTOM_COMMAND_SIG(debugger_open_file_at_cursor)
CUSTOM_DOC("Open current file in debugger")
{
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  String_Const_u8 cmd = push_stringf(scratch, "rbg.exe open-file %.*s %d",
                                     string_expand(info.curr_file), info.line_number);
  if(cmd.size)
  {
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, cmd, 0);
  }
}

CUSTOM_COMMAND_SIG(debugger_breakpoint_at_cursor)
CUSTOM_DOC("Set remedybg breakpoint at cursor")
{
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  String_Const_u8 cmd = push_stringf(scratch, "rbg.exe add-breakpoint-at-file %.*s %d",
                                     string_expand(info.curr_file), info.line_number);
  if(cmd.size)
  {
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, cmd, 0);
  }
}

CUSTOM_COMMAND_SIG(debugger_remove_breakpoint_at_cursor)
CUSTOM_DOC("Remove remedybg breakpoint at cursor")
{
  Scratch_Block scratch(app);
  FileInfoForCommand info = FileInfoForCommandGet(app, scratch);
  String_Const_u8 cmd = push_stringf(scratch, "rbg.exe remove-breakpoint-at-file %.*s %d",
                                     string_expand(info.curr_file), info.line_number);
  if(cmd.size)
  {
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, cmd, 0);
  }
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
  Scratch_Block scratch(app);
  Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
  Buffer_Identifier id = buffer_identifier(buffer);
  String_Const_u8 hot = push_hot_directory(app, scratch);
  String_Const_u8 cmd = SCu8("explorer.exe .");
  exec_system_command(app, global_compilation_view, id, hot, cmd, 0);
}
