global f32 global_highlight_transparency = 0.2f;
static f32 MinimumF32(f32 a, f32 b){return a < b ? a : b;}
static f32 MaximumF32(f32 a, f32 b){return a > b ? a : b;}
enum F4_RangeHighlightKind
{
  F4_RangeHighlightKind_Whole,
  F4_RangeHighlightKind_Underline,
};
function void
F4_RenderRangeHighlight(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id,
                        Range_i64 range, F4_RangeHighlightKind kind)
{
  Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
  Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end-1);
  Rect_f32 total_range_rect = {0};
  total_range_rect.x0 = MinimumF32(range_start_rect.x0, range_end_rect.x0);
  total_range_rect.y0 = MinimumF32(range_start_rect.y0, range_end_rect.y0);
  total_range_rect.x1 = MaximumF32(range_start_rect.x1, range_end_rect.x1);
  total_range_rect.y1 = MaximumF32(range_start_rect.y1, range_end_rect.y1);
  if(kind == F4_RangeHighlightKind_Underline)
  {
    total_range_rect.y0 = total_range_rect.y1 - 1.f;
    total_range_rect.y1 += 1.f;
  }
  draw_rectangle(app, total_range_rect, 4.f, fcolor_resolve(fcolor_change_alpha(fcolor_id(defcolor_selection_highlight), global_highlight_transparency)));
}


static void
F4_HighlightCursorMarkRange(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, i64 cursor_pos, i64 mark_pos)
{
  Rect_f32 view_rect = view_get_screen_rect(app, view_id);
  Rect_f32 clip = draw_set_clip(app, view_rect);
  
  Rect_f32 cursor_rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
  Rect_f32 mark_rect = text_layout_character_on_screen(app, text_layout_id, mark_pos);
  f32 lower_bound_y;
  f32 upper_bound_y;
  
  if(cursor_rect.y0 < mark_rect.y0)
  {
    lower_bound_y = cursor_rect.y0;
    upper_bound_y = mark_rect.y1;
  }
  else
  {
    lower_bound_y = mark_rect.y0;
    upper_bound_y = cursor_rect.y1;
  }
  
  draw_rectangle(app, Rf32(view_rect.x0, lower_bound_y, view_rect.x0 + 4, upper_bound_y), 0,
                 fcolor_resolve(fcolor_change_alpha(fcolor_id(defcolor_selection_highlight), global_highlight_transparency)));
  draw_set_clip(app, clip);
}

function void 
HighlightSelectionMatches(Application_Links *app, View_ID view, 
                          Text_Layout_ID text_layout_id, Buffer_ID buffer, 
                          Range_i64 visible_range, i64 cursor_pos, i64 mark_pos)
{
  // This function draws all the matches of current selection (from mark to cursor)
  // also highlights the mark, cursor vertical difference, highlighted by column on the left
  Scratch_Block scratch(app);
  
  Range_i64 selection = 
  {
    Min(cursor_pos, mark_pos),
    Max(cursor_pos, mark_pos)
  };
  
  i64 line_min = get_line_number_from_pos(app, buffer, selection.min);
  i64 line_max = get_line_number_from_pos(app, buffer, selection.max);
  
  if(line_min == line_max)
  {
    
    String_Const_u8 selected_string = push_buffer_range(app, scratch, 
                                                        buffer, selection);
    
    local_persist Character_Predicate *pred = &character_predicate_alpha_numeric_underscore_utf8;
    
    String_Match_List matches = buffer_find_all_matches(app, scratch, buffer,  0, 
                                                        visible_range, 
                                                        selected_string, pred, 
                                                        Scan_Forward);
    String_Match_Flag must_have_flags = StringMatch_CaseSensitive;
    string_match_list_filter_flags(&matches, must_have_flags, 0);
    
    for (String_Match *node = matches.first; node != 0; node = node->next){
      /* 
            F4_RenderRangeHighlight(app, view, text_layout_id, node->range, F4_RangeHighlightKind_Underline);  
       */
      draw_character_block(app, text_layout_id, 
                           node->range, 0, 
                           fcolor_resolve(fcolor_change_alpha(fcolor_id(defcolor_selection_highlight), global_highlight_transparency)));
    }
  }
  else F4_HighlightCursorMarkRange(app, view, text_layout_id, cursor_pos, mark_pos);
}
