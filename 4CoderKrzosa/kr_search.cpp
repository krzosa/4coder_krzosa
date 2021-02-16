
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
