
CUSTOM_COMMAND_SIG(replace_in_buffer_identifier)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in the active buffer.")
{
  Scratch_Block scratch(app);
  View_ID view = get_active_view(app, Access_ReadWriteVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
  String_Const_u8 query = push_token_or_word_under_active_cursor(app, scratch);
  if(query.size)
  {
    Query_Bar_Group group(app);
    Query_Bar string_bar = {};
    String_Const_u8 prompt = push_stringf(scratch, "Replace %.*s with: ", string_expand(query));
    string_bar.prompt = prompt;
    u8 string_buffer[KB(1)];
    string_bar.string.str = string_buffer;
    string_bar.string_capacity = sizeof(string_buffer);
    if (query_user_string(app, &string_bar))
    {
      if(string_bar.string.size > 0)
      {
        String_Const_u8 msg = push_stringf(scratch, "%.*s", string_expand(string_bar.string));
        Range_i64 range = buffer_range(app, buffer);
        replace_in_range(app, buffer, range, query, msg);
      }
    }
  }
}

CUSTOM_COMMAND_SIG(replace_in_all_buffers_fixed)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in all editable buffers.")
{
  Scratch_Block scratch(app);
  Query_Bar_Group group(app);
  String_Pair pair = query_user_replace_pair(app, scratch);
  if(pair.valid)
  {
    global_history_edit_group_begin(app);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible)){
      Range_i64 range = buffer_range(app, buffer);
      replace_in_range(app, buffer, range, pair.a, pair.b);
    }
    global_history_edit_group_end(app);
  }
  
}

CUSTOM_COMMAND_SIG(replace_in_all_buffers_fixed_identifier)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in all editable buffers.")
{
  global_history_edit_group_begin(app);
  Scratch_Block scratch(app);
  String_Const_u8 query = push_token_or_word_under_active_cursor(app, scratch);
  if(query.size)
  {
    Query_Bar_Group group(app);
    Query_Bar string_bar = {};
    String_Const_u8 prompt = push_stringf(scratch, "ReplaceInAllBuff %.*s with: ", string_expand(query));
    string_bar.prompt = prompt;
    u8 string_buffer[KB(1)];
    string_bar.string.str = string_buffer;
    string_bar.string_capacity = sizeof(string_buffer);
    if (query_user_string(app, &string_bar))
    {
      if(string_bar.string.size > 0)
      {
        for (Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
             buffer != 0;
             buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible)){
          Range_i64 range = buffer_range(app, buffer);
          replace_in_range(app, buffer, range, query, string_bar.string);
        }
      }
    }
  }
  
  global_history_edit_group_end(app);
}
