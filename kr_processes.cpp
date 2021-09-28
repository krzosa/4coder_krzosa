#define L(...) string_u8_litexpr(__VA_ARGS__)
#define E(...) string_expand(__VA_ARGS__)
#define _loop_sll(first,node,next) for(auto *node = (first); node; node=node->next)
#define loop_sll(first,node) _loop_sll(first,node,next)
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

function bool
string_starts_with(S8 a, S8 starts_with) {
  if(a.size < starts_with.size) return false;
  for(u64 i = 0; i < starts_with.size; i++) {
    if(a.str[i] != starts_with.str[i]) return false;
  }
  return true;
}

function i64
string_find_first2(S8 string, S8 seek) {
  i64 result = -1;
  for(i64 i = 0; i < (i64)string.size; i++) {
    Range_i64 range = Ii64(i, clamp_top(i+seek.size, string.size));
    if(string_match(string_substring(string, range), seek)) {
      result = i;
      break;
    }
  }
  return result;
}

function FileInfoForCommand
get_file_info(Application_Links *app, Arena *scratch)
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

internal void
exec_commandf(Application_Links *app, S8 cmd) {
  u8 buff[2048];
  String_u8 str = {buff, 0, 2048};
  Scratch_Block scratch(app);
  FileInfoForCommand info = get_file_info(app, scratch);
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
  
  print_message(app, str.string);
  print_message(app, string_u8_litexpr("\n"));
  if(cmd.size)
  {
    exec_system_command(app, global_compilation_view, info.buffer_id, info.hot_dir, str.string, CLI_OverlapWithConflict|CLI_CursorAtEnd|CLI_SendEndSignal);
  }
}

CUSTOM_UI_COMMAND_SIG(D1_debugger_open_file)
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
    S8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s", string_expand(path), string_expand(file_name));
    
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

CUSTOM_COMMAND_SIG(D2_debugger_open_file_at_cursor)
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
  save_all_dirty_buffers(app);
  exec_commandf(app, command);
}

CUSTOM_COMMAND_SIG(compile_current_file)
CUSTOM_DOC("Compile current file with clang")
{
  Scratch_Block scratch(app);
  S8 command = def_get_config_string(scratch, vars_save_string_lit("compile_command"));
  exec_commandf(app, command);
}

function void 
open_file_in_4coder_dir(Application_Links *app, S8 file)
{
  View_ID active_view = get_active_view(app, Access_Always);
  Scratch_Block scratch(app);
  S8 binary = system_get_path(scratch, SystemPath_Binary);
  String_u8 path = string_u8_push(scratch, 255);
  string_append(&path, binary);
  string_append(&path, file);
  view_open_file(app, active_view, path.string, false);
}

CUSTOM_COMMAND_SIG(C1_open_bindings)
CUSTOM_DOC("Open hotkey file")
{
  open_file_in_4coder_dir(app, SCu8("4coder_krzosa/bindings.4coder"));
}

CUSTOM_COMMAND_SIG(C2_open_snippets)
CUSTOM_DOC("Open hotkey file")
{
  open_file_in_4coder_dir(app, SCu8("snippets.txt"));
}

CUSTOM_COMMAND_SIG(C3_open_theme)
CUSTOM_DOC("Open theme file")
{
  open_file_in_4coder_dir(app, SCu8("themes/kr.4coder"));
}

///////////////////////////////////////
// @Section: Project managment

function FILE *
fopen_file_in_4coder_dir(Arena *scratch, S8 filename, char *op) {
  S8 binary = system_get_path(scratch, SystemPath_Binary);
  S8 path = push_stringf(scratch, "%.*s%.*s\0", binary.size, binary.str, filename.size, filename.str);
  FILE *file = fopen((char *)path.str, op);
  return file;
}

CUSTOM_COMMAND_SIG(last_project)
CUSTOM_DOC("Opens last project")
{
  Scratch_Block scratch(app);
  FILE *file = fopen_file_in_4coder_dir(scratch, L("projects.txt"), "rb");
  if(file) {
    S8 data = dump_file_handle(scratch, file);
    List_String_Const_u8 list = string_split(scratch, data, (u8 *)"\n", 1);
    Node_String_Const_u8 *first = list.first;
    if(first && first->string.size) {
      String8 string = string_skip_chop_whitespace(first->string);
      if(string.size) {
        close_all_code(app);
        set_hot_directory(app, string);
        open_all_code_recursive(app);
      }
    }
    fclose(file);
  }
}

CUSTOM_COMMAND_SIG(projects_list)
CUSTOM_DOC("Opens a project list")
{
  Scratch_Block scratch(app);
  FILE *file = fopen_file_in_4coder_dir(scratch, L("projects.txt"), "rb");
  if(file) {
    S8 data = dump_file_handle(scratch, file);
    List_String_Const_u8 list = string_split(scratch, data, (u8 *)"\n", 1);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, L("project path: "));
    lister_set_default_handlers(lister);
    
    loop_sll(list.first, node) {
      S8 str = node->string;
      lister_add_item(lister, str, str, (void *)node, 0);
    }
    Lister_Result result = run_lister(app, lister);
    if(!result.canceled) {
      Node_String_Const_u8* ptr = (Node_String_Const_u8 *)result.user_data;
      if(ptr) {
        ptr->string = string_skip_chop_whitespace(ptr->string);
        FILE *write = fopen_file_in_4coder_dir(scratch, L("projects.txt"), "w");
        if(write) { // @Note: Reorder nodes and write so that opened node is first
          List_String_Const_u8 save_list = {};
          string_list_push(scratch, &save_list, ptr->string);
          loop_sll(list.first, node) {
            if(node != ptr) string_list_push(scratch, &save_list, node->string);
          }
          S8 new_order = string_list_flatten(scratch, save_list, 0, L("\n"), 0, StringFill_NoTerminate);
          fwrite(new_order.str, 1, new_order.size, write);
          fclose(write);
        }
        if(ptr->string.size) {
          close_all_code(app);
          set_hot_directory(app, ptr->string);
          open_all_code_recursive(app);
        }
      }
    }
    fclose(file);
  }
  else {
    Lister_Block lister(app, scratch);
    lister_set_query(lister, L("projects.txt doesn't exist! Add projects using add_folder_to_project_list"));
    lister_set_default_handlers(lister);
    run_lister(app, lister);
  }
}

CUSTOM_COMMAND_SIG(add_folder_to_project_list)
CUSTOM_DOC("Opens a project list")
{
  Scratch_Block scratch(app);
  FILE *file = fopen_file_in_4coder_dir(scratch, L("projects.txt"), "rb");
  S8 data = L("");
  if(file) {
    data = dump_file_handle(scratch, file);
    fclose(file);
  }
  
  file = fopen_file_in_4coder_dir(scratch, L("projects.txt"), "w");
  if(file) {
    S8 hot = push_hot_directory(app, scratch);
    S8 combined_power_of_will = push_stringf(scratch, "%.*s\n%.*s", E(data), E(hot));
    fwrite(combined_power_of_will.str, 1, combined_power_of_will.size, file);
    fclose(file);
  }
}

CUSTOM_COMMAND_SIG(restart_4coder)
CUSTOM_DOC("Kill current instance and make a new 4coder")
{
  exec_commandf(app, string_u8_litexpr("4ed.bat"));
  exit_4coder(app);
}

CUSTOM_COMMAND_SIG(run_console_command)
CUSTOM_DOC("Run console command")
{
  Scratch_Block scratch(app);
  FILE *file = fopen_file_in_4coder_dir(scratch, L("console_commands.txt"), "rb");
  S8 data = L("");
  if(file) {
    data = dump_file_handle(scratch, file);
    fclose(file);
  }
  List_String_Const_u8 list = string_split(scratch, data, (u8 *)"\n", 1);
  Lister_Block lister(app, scratch);
  lister_set_query(lister, L("Command:"));
  lister_set_default_handlers(lister);
  loop_sll(list.first,node) {
    node->string = string_skip_chop_whitespace(node->string);
    lister_add_item(lister, node->string, node->string, (void *)node, 0);
  }
  
  Lister_Result result = run_lister(app, lister);
  if(!result.canceled) {
    Node_String_Const_u8 *ptr = (Node_String_Const_u8 *)result.user_data;
    if(!(ptr == 0 && result.text_field.size == 0)) {
      String_Const_u8 command = L("");
      FILE *write = fopen_file_in_4coder_dir(scratch, L("console_commands.txt"), "w");
      if(write) {
        S8 new_order = L("");
        if(ptr) {
          command = ptr->string;
          List_String_Const_u8 save_list = {};
          string_list_push(scratch, &save_list, ptr->string);
          loop_sll(list.first, node) {
            if(node != ptr) {
              string_list_push(scratch, &save_list, node->string);
            }
          }
          new_order = string_list_flatten(scratch, save_list, 0, L("\n"), 0, StringFill_NoTerminate);
        }
        else {
          command = result.text_field;
          List_String_Const_u8 save_list = {};
          string_list_push(scratch, &save_list, command);
          string_list_push(&save_list, &list);
          new_order = string_list_flatten(scratch, save_list, 0, L("\n"), 0, StringFill_NoTerminate);
        }
        
        if(command.size) {
          exec_commandf(app, command);
        }
        fwrite(new_order.str, 1, new_order.size, write);
        fclose(write);
      }
    }
  }
}

///////////////////////////////////////
// @Section: Snippet from file

function List_String_Const_u8
string_split_needle_dont_include(Arena *arena, String_Const_u8 string, String_Const_u8 needle){
  List_String_Const_u8 list = {};
  for (;string.size > 0;){
    u64 pos = string_find_first(string, needle);
    String_Const_u8 prefix = string_prefix(string, pos);
    if (prefix.size > 0){
      string_list_push(arena, &list, prefix);
    }
    string = string_skip(string, prefix.size + needle.size);
  }
  return(list);
}

struct Snippet2 { // Long awaited continuation of the Snippet
  S8 name;
  S8 text;
  u64 cursor_offset;
  u64 mark_offset;
};

function void
write_snippet(Application_Links *app, View_ID view, Buffer_ID buffer,
              i64 pos, Snippet2 *snippet){
  if (snippet != 0){
    String_Const_u8 snippet_text = snippet->text;
    buffer_replace_range(app, buffer, Ii64(pos), snippet_text);
    i64 new_cursor = pos + snippet->cursor_offset;
    view_set_cursor_and_preferred_x(app, view, seek_pos(new_cursor));
    i64 new_mark = pos + snippet->mark_offset;
    view_set_mark(app, view, seek_pos(new_mark));
    auto_indent_buffer(app, buffer, Ii64_size(pos, snippet_text.size));
  }
}

function Snippet2*
get_snippet_from_user(Application_Links *app, Snippet2 *snippets, i32 snippet_count,
                      String_Const_u8 query){
  Scratch_Block scratch(app);
  Lister_Block lister(app, scratch);
  lister_set_query(lister, query);
  lister_set_default_handlers(lister);
  
  Snippet2 *snippet = snippets;
  for (i32 i = 0; i < snippet_count; i += 1, snippet += 1){
    lister_add_item(lister, snippet->name, snippet->text, snippet, 0);
  }
  Lister_Result l_result = run_lister(app, lister);
  Snippet2 *result = 0;
  if (!l_result.canceled){
    result = (Snippet2*)l_result.user_data;
  }
  return(result);
}

CUSTOM_UI_COMMAND_SIG(snippet_lister_from_file)
CUSTOM_DOC("Opens a snippet lister for inserting whole pre-written snippets of text.")
{
  View_ID view = get_this_ctx_view(app, Access_ReadWrite);
  if (view != 0){
    Scratch_Block scratch(app);
    Scratch_Block array_alloc(app, scratch);
    Snippet2 *input_snippets = 0;
    u32 len = 0;
    FILE *file = fopen_file_in_4coder_dir(scratch, L("snippets.txt"), "rb");
    if(file) {
      S8 data = dump_file_handle(scratch, file);
      *push_array(scratch, u8, 1) = 0; // null_terminate
      List_String_Const_u8 list = string_split_needle_dont_include(scratch, data, L("::"));
      Snippet2 *snippet = push_array(array_alloc, Snippet2, 1);
      input_snippets = snippet;
      len = 0;
      for(Node_String_Const_u8 *node=list.first; node;) {
        if(node==0||node->next==0) break;
        snippet->cursor_offset = (i32)string_find_first2(node->next->string, L("$"));
        snippet->mark_offset = (i32)string_find_first2(node->next->string, L("$$"))-1;
        snippet->name = string_skip_chop_whitespace(node->string); 
        snippet->text = string_replace(scratch, node->next->string, L("$"), L(""));
        push_array(array_alloc, Snippet2, 1);
        node=node->next->next; len++; snippet++;
      }
      fclose(file);
    }
    else {
      Lister_Block lister(app, scratch);
      lister_set_query(lister, L("snippets.txt doesn't exist! Add snippets.txt to 4coder directory."));
      lister_set_default_handlers(lister);
      run_lister(app, lister);
    }
    
    if(len > 0) {
      Snippet2 *snippet = get_snippet_from_user(app, input_snippets, len, L("Snippet:"));
      Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
      i64 pos = view_get_cursor_pos(app, view);
      write_snippet(app, view, buffer, pos, snippet);
    }
  }
}