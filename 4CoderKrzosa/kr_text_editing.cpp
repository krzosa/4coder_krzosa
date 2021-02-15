/* GUIDE

1. Copy and paste this file into your custom layer (4coder_default_bindings.cpp by default) below the line: "#include "4coder_default_include.cpp""
2. Bind commands to keybindings
3. Done! */
#define KR_TEXT_EDITING

struct selected_lines_info
{
  i64 cursor_pos;
  i64 mark_pos;
  
  i64 min_pos;
  i64 max_pos;
  
  i64 min_line;
  i64 max_line;
  
  // all WHOLE selected lines
  Range_i64 entire_selected_lines_pos;
};

function selected_lines_info
get_selected_lines_info(Application_Links *app, View_ID view, Buffer_ID buffer)
{
  selected_lines_info result;
  
  result.cursor_pos = view_get_cursor_pos(app, view);
	result.mark_pos = view_get_mark_pos(app, view);
  
	result.min_pos = Min(result.cursor_pos, result.mark_pos);
	result.max_pos = Max(result.cursor_pos, result.mark_pos);
  
  result.min_line = get_line_number_from_pos(app, buffer, result.min_pos);
  result.max_line = get_line_number_from_pos(app, buffer, result.max_pos);
  
  i64 min_line = get_line_side_pos(app, buffer, result.min_line, Side_Min);
  i64 max_line = get_line_side_pos(app, buffer, result.max_line, Side_Max);
  
  result.entire_selected_lines_pos.min = min_line;
  result.entire_selected_lines_pos.max = max_line;
  
  return result;
}

CUSTOM_COMMAND_SIG(delete_selected_lines)
CUSTOM_DOC("Delete selected lines up")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  
  selected_lines_info selection = get_selected_lines_info(app, view, buffer);
  
  Range_i64 range = selection.entire_selected_lines_pos;
  
  range.start -= 1;
  range.start = clamp_bot(0, range.start);
  
  buffer_replace_range(app, buffer, range, string_u8_litexpr(""));
  
  view_set_cursor(app, view, seek_line_col(selection.min_line, 0));
  
}

CUSTOM_COMMAND_SIG(duplicate_multiple_lines_down)
CUSTOM_DOC("Duplicate selected lines down")
{
  View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  
	selected_lines_info selection = get_selected_lines_info(app, view, buffer);
  Range_i64 range = selection.entire_selected_lines_pos;
  
  Scratch_Block scratch(app);
  
  String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);
  string = push_u8_stringf(scratch, "%.*s\n%.*s", string_expand(string),
                           string_expand(string));
  buffer_replace_range(app, buffer, range, string);
  
  // NOTE(KKrzosa): Select the entire dupicated part
  i64 lines_duplicated = selection.max_line - selection.min_line;
  
  Range_i64 new_range;
  new_range.min = get_line_side_pos(app, buffer, selection.max_line + 1, Side_Min);
  new_range.max = get_line_side_pos(app, buffer, selection.max_line + lines_duplicated + 1, Side_Max);;
  
  if(lines_duplicated > 0) select_scope(app, view, new_range);
  else view_set_cursor(app, view, seek_line_col(selection.max_line + 1, 0));
}


CUSTOM_COMMAND_SIG(duplicate_multiple_lines_up)
CUSTOM_DOC("Duplicate selected lines up")
{
  View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  
	selected_lines_info selection = get_selected_lines_info(app, view, buffer);
  
  Range_i64 range = selection.entire_selected_lines_pos;
  
  Scratch_Block scratch(app);
  
  String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);
  string = push_u8_stringf(scratch, "%.*s\n%.*s", string_expand(string),
                           string_expand(string));
  buffer_replace_range(app, buffer, range, string);
  
  i64 lines_duplicated = selection.max_line - selection.min_line;
  
  if(lines_duplicated > 0) select_scope(app, view, range);
  else view_set_cursor(app, view, seek_line_col(selection.max_line, 0));
}

CUSTOM_COMMAND_SIG(move_lines_up)
CUSTOM_DOC("Move selected lines up")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  
	History_Record_Index history_index = {};
  
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
  
	i64 min_pos = Min(cursor_pos, mark_pos);
	i64 max_pos = Max(cursor_pos, mark_pos);
  
	Buffer_Cursor min_cursor = view_compute_cursor(app, view, seek_pos(min_pos));
	Buffer_Cursor max_cursor = view_compute_cursor(app, view, seek_pos(max_pos));
  
	bool first = true;
	for (i64 line=min_cursor.line; line<=max_cursor.line; line++){
		view_set_cursor(app, view, seek_line_col(line, 0));
		current_view_move_line(app, Scan_Backward);
		if (first){
			history_index = buffer_history_get_current_state_index(app, buffer);
		}
		first = false;
	}
  
	Buffer_Cursor cursor_cursor = min_cursor;
	Buffer_Cursor mark_cursor = max_cursor;
	if (cursor_pos > mark_pos){
		cursor_cursor = max_cursor;
		mark_cursor = min_cursor;
	}
	view_set_cursor(app, view, seek_line_col(cursor_cursor.line-1, cursor_cursor.col));
	view_set_mark(app, view, seek_line_col(mark_cursor.line-1, mark_cursor.col));
	no_mark_snap_to_cursor(app, view);
  
	History_Record_Index history_index_end = buffer_history_get_current_state_index(app, buffer);
	buffer_history_merge_record_range(app, buffer, history_index, history_index_end, RecordMergeFlag_StateInRange_MoveStateForward);
}

CUSTOM_COMMAND_SIG(move_lines_down)
CUSTOM_DOC("Move selected lines down")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
  
	History_Record_Index history_index = {};
  
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
  
	i64 min_pos = Min(cursor_pos, mark_pos);
	i64 max_pos = Max(cursor_pos, mark_pos);
  
	Buffer_Cursor min_cursor = view_compute_cursor(app, view, seek_pos(min_pos));
	Buffer_Cursor max_cursor = view_compute_cursor(app, view, seek_pos(max_pos));
  
	bool first = true;
	for (i64 line=max_cursor.line; line>=min_cursor.line; line--){
		view_set_cursor(app, view, seek_line_col(line, 0));
		current_view_move_line(app, Scan_Forward);
		if (first){
			history_index = buffer_history_get_current_state_index(app, buffer);
		}
		first = false;
	}
  
	Buffer_Cursor cursor_cursor = min_cursor;
	Buffer_Cursor mark_cursor = max_cursor;
	if (cursor_pos > mark_pos){
		cursor_cursor = max_cursor;
		mark_cursor = min_cursor;
	}
	view_set_cursor(app, view, seek_line_col(cursor_cursor.line+1, cursor_cursor.col));
	view_set_mark(app, view, seek_line_col(mark_cursor.line+1, mark_cursor.col));
	no_mark_snap_to_cursor(app, view);
  
	History_Record_Index history_index_end = buffer_history_get_current_state_index(app, buffer);
	buffer_history_merge_record_range(app, buffer, history_index, history_index_end, RecordMergeFlag_StateInRange_MoveStateForward);
}

CUSTOM_COMMAND_SIG(put_new_line_bellow)
CUSTOM_DOC("Pust a new line bellow cursor line")
{
  View_ID view = get_active_view(app, Access_ReadWriteVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
  i64 pos = view_get_cursor_pos(app, view);
  i64 line = get_line_number_from_pos(app, buffer, pos);
  line += 1;
  Scratch_Block scratch(app);
  String_Const_u8 s = push_buffer_line(app, scratch, buffer, line);
  s = push_u8_stringf(scratch, "\n");
  pos = get_line_side_pos(app, buffer, line, Side_Min);
  buffer_replace_range(app, buffer, Ii64(pos), s);
  view_set_cursor_and_preferred_x(app, view, seek_line_col(line, 0));
}

// 1 == left to right or 0 ==right to left
function void
write_on_both_sides_of_selection(Application_Links *app, String_Const_u8 left, String_Const_u8 right, b32 left_to_right){
  View_ID view = get_active_view(app, Access_ReadWriteVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
  selected_lines_info selection = get_selected_lines_info(app, view, buffer);
  History_Group group = history_group_begin(app, buffer);
  
  if(left_to_right){
    buffer_replace_range(app, buffer, Ii64(selection.min_pos), left);
    if(selection.cursor_pos != selection.mark_pos){
      buffer_replace_range(app, buffer, Ii64(selection.max_pos + right.size), right);
    }
    view_set_cursor_and_preferred_x(app, view, seek_pos(selection.cursor_pos + right.size));
  } else {
    buffer_replace_range(app, buffer, Ii64(selection.max_pos), right);
    if(selection.cursor_pos != selection.mark_pos){
      buffer_replace_range(app, buffer, Ii64(selection.min_pos), left);
    }
    view_set_cursor_and_preferred_x(app, view, seek_pos(selection.cursor_pos + 1));
  }
  
  history_group_end(group);
}

CUSTOM_COMMAND_SIG(write_paren_on_selection_left)
CUSTOM_DOC("Encloses the selection with parenthesis")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("("), string_u8_litexpr(")"), true);
}

CUSTOM_COMMAND_SIG(write_paren_on_selection_right)
CUSTOM_DOC("Encloses the selection with parenthesis")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("("), string_u8_litexpr(")"), false);
}

CUSTOM_COMMAND_SIG(write_curly_brace_on_selection_left)
CUSTOM_DOC("Encloses the selection with curly_brace")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("{"), string_u8_litexpr("}"), true);
}

CUSTOM_COMMAND_SIG(write_curly_brace_on_selection_right)
CUSTOM_DOC("Encloses the selection with curly_brace")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("{"), string_u8_litexpr("}"), false);
}

CUSTOM_COMMAND_SIG(write_bracket_on_selection_left)
CUSTOM_DOC("Encloses the selection with bracket")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("["), string_u8_litexpr("]"), true);
}

CUSTOM_COMMAND_SIG(write_bracket_on_selection_right)
CUSTOM_DOC("Encloses the selection with bracket")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("["), string_u8_litexpr("]"), false);
}

CUSTOM_COMMAND_SIG(write_quote_on_selection_right)
CUSTOM_DOC("Encloses the selection with quote")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("["), string_u8_litexpr("]"), false);
}

CUSTOM_COMMAND_SIG(write_quote_on_selection)
CUSTOM_DOC("Encloses the selection with quote")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("\""), string_u8_litexpr("\""), true);
}

CUSTOM_COMMAND_SIG(write_single_quote_on_selection)
CUSTOM_DOC("Encloses the selection with single_quote")
{
  write_on_both_sides_of_selection(app, string_u8_litexpr("\'"), string_u8_litexpr("\'"), true);
}
