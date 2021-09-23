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
    
    String_Const_u8 selected_string = push_buffer_range(app, scratch, buffer, selection);
    
    local_persist Character_Predicate *pred = &character_predicate_alpha_numeric_underscore_utf8;
    
    String_Match_List matches = buffer_find_all_matches(app, scratch, buffer,  0, 
                                                        visible_range, 
                                                        selected_string, pred, 
                                                        Scan_Forward);
    String_Match_Flag must_have_flags = StringMatch_CaseSensitive;
    string_match_list_filter_flags(&matches, must_have_flags, 0);
    
    for (String_Match *node = matches.first; node != 0; node = node->next){
      if(selection == node->range)
      {
        F4_RenderRangeHighlight(app, view, text_layout_id, node->range, F4_RangeHighlightKind_Underline);  
      }
      else 
        draw_character_block(app, text_layout_id, node->range, 0, 
                             fcolor_resolve(fcolor_change_alpha(fcolor_id(defcolor_selection_highlight), global_highlight_transparency)));
    }
  }
  else F4_HighlightCursorMarkRange(app, view, text_layout_id, cursor_pos, mark_pos);
}

#define HIGHLIGH_SELECTION_MATCH 1

function void
RenderBuffer(Application_Links *app, View_ID view_id, Face_ID face_id,
             Buffer_ID buffer, Text_Layout_ID text_layout_id,
             Rect_f32 rect){
  ProfileScope(app, "render buffer");
  
  View_ID active_view = get_active_view(app, Access_Always);
  b32 is_active_view = (active_view == view_id);
  Rect_f32 prev_clip = draw_set_clip(app, rect);
  
  Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
  
  // NOTE(allen): Cursor shape
  Face_Metrics metrics = get_face_metrics(app, face_id);
  u64 cursor_roundness_100 = def_get_config_u64(app, vars_save_string_lit("cursor_roundness"));
  f32 cursor_roundness = metrics.normal_advance*cursor_roundness_100*0.01f;
  f32 mark_thickness = (f32)def_get_config_u64(app, vars_save_string_lit("mark_thickness"));
  
  // NOTE(allen): Token colorizing
  Token_Array token_array = get_token_array_from_buffer(app, buffer);
  if (token_array.tokens != 0){
    draw_cpp_token_colors(app, text_layout_id, &token_array);
    
    // NOTE(allen): Scan for TODOs and NOTEs
    b32 use_comment_keyword = def_get_config_b32(vars_save_string_lit("use_comment_keyword"));
    if (use_comment_keyword){
      Comment_Highlight_Pair pairs[] = {
        {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
        {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
      };
      draw_comment_highlights(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
    }
    
    Scratch_Block scratch(app);
    Token_Iterator_Array it = token_iterator_pos(0, &token_array, visible_range.first);
    for (;;){
      if (!token_it_inc_non_whitespace(&it)){
        break;
      }
      Token *token = token_it_read(&it);
      String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);
      Code_Index_Note *note = code_index_note_from_string(lexeme);
      if (note != 0){
        if(note->note_kind == CodeIndexNote_Function){
          paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), finalize_color(defcolor_function, 0));
        }
        else if(note->note_kind == CodeIndexNote_Type){
          paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), finalize_color(defcolor_type, 0));
        }
        else if(note->note_kind == CodeIndexNote_Macro){
          paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), finalize_color(defcolor_macro, 0));
        }
      }
    }
  }
  else{
    paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
  }
  
  i64 cursor_pos = view_correct_cursor(app, view_id);
  i64 mark_pos = view_correct_mark(app, view_id);
  
  HighlightSelectionMatches(app, view_id, text_layout_id, buffer, visible_range, cursor_pos, mark_pos);
  // NOTE(allen): Scope highlight
  b32 use_scope_highlight = def_get_config_b32(vars_save_string_lit("use_scope_highlight"));
  if (use_scope_highlight){
    Color_Array colors = finalize_color_array(defcolor_back_cycle);
    draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
  }
  
  b32 use_error_highlight = def_get_config_b32(vars_save_string_lit("use_error_highlight"));
  b32 use_jump_highlight = def_get_config_b32(vars_save_string_lit("use_jump_highlight"));
  if (use_error_highlight || use_jump_highlight){
    // NOTE(allen): Error highlight
    String_Const_u8 name = string_u8_litexpr("*compilation*");
    Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
    if (use_error_highlight){
      draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                           fcolor_id(defcolor_highlight_junk));
    }
    
    // NOTE(allen): Search highlight
    if (use_jump_highlight){
      Buffer_ID jump_buffer = get_locked_jump_buffer(app);
      if (jump_buffer != compilation_buffer){
        draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                             fcolor_id(defcolor_highlight_white));
      }
    }
  }
  
  // NOTE(allen): Color parens
  b32 use_paren_helper = def_get_config_b32(vars_save_string_lit("use_paren_helper"));
  if (use_paren_helper){
    Color_Array colors = finalize_color_array(defcolor_text_cycle);
    draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
  }
  
  // NOTE(allen): Line highlight
  b32 highlight_line_at_cursor = def_get_config_b32(vars_save_string_lit("highlight_line_at_cursor"));
  if (highlight_line_at_cursor && is_active_view){
    i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
    draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
  }
  
  // NOTE(allen): Whitespace highlight
  b64 show_whitespace = false;
  view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
  if (show_whitespace){
    if (token_array.tokens == 0){
      draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
    }
    else{
      draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
    }
  }
  
  // NOTE(allen): Cursor
  switch (fcoder_mode){
    case FCoderMode_Original:
    {
      draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
    }break;
    case FCoderMode_NotepadLike:
    {
      draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
    }break;
  }
  
  
  // NOTE(allen): Fade ranges
  paint_fade_ranges(app, text_layout_id, buffer);
  
  // NOTE(allen): put the actual text on the actual screen
  draw_text_layout_default(app, text_layout_id);
  
  draw_set_clip(app, prev_clip);
}

/* 
function Rect_f32
(Application_Links *app, View_ID view_id, b32 is_active_view){
  u64 config_margin_size = def_get_config_u64(app, vars_save_string_lit("margin_size"));
  FColor margin_color = get_panel_margin_color(is_active_view?UIHighlight_Active:UIHighlight_None);
  Rect_f32 region = draw_background_and_margin(app, view_id, margin_color, fcolor_id(defcolor_back), (f32)config_margin_size);
  return region;
}
 */

function Rect_f32
DrawBGAndMargin(Application_Links *app, View_ID view_id, 
                Buffer_ID buffer, b32 is_active_view){
  FColor margin_color = get_panel_margin_color(is_active_view?UIHighlight_Active:UIHighlight_None);
  u64 config_margin_size = def_get_config_u64(app, vars_save_string_lit("margin_size"));
  
  Scratch_Block scratch(app);
  String_Const_u8 string = push_buffer_base_name(app, scratch, buffer);
  b32 matches = string_match(string, string_u8_litexpr("*compilation*"));
  
  
  FColor back = fcolor_id(defcolor_back);
  if(matches) back = fcolor_id(defcolor_compilation_buffer);
  
  Rect_f32 region = draw_background_and_margin(app, view_id, margin_color, 
                                               back, (f32)config_margin_size);
  
  return region;
}


function void
krDrawFileBar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar, String_Const_u8 *additional_string = 0){
  Scratch_Block scratch(app);
  
  draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
  
  FColor base_color = fcolor_id(defcolor_base);
  FColor pop2_color = fcolor_id(defcolor_pop2);
  
  i64 cursor_position = view_get_cursor_pos(app, view_id);
  Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));
  
  Fancy_Line list = {};
  String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
  push_fancy_string(scratch, &list, base_color, unique_name);
  push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);
  
  Managed_Scope scope = buffer_get_managed_scope(app, buffer);
  Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                   Line_Ending_Kind);
  switch (*eol_setting){
    case LineEndingKind_Binary:
    {
      push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
    }break;
    
    case LineEndingKind_LF:
    {
      push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
    }break;
    
    case LineEndingKind_CRLF:
    {
      push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
    }break;
  }
  
  u8 space[3];
  {
    Dirty_State dirty = buffer_get_dirty_state(app, buffer);
    String_u8 str = Su8(space, 0, 3);
    if (dirty != 0){
      string_append(&str, string_u8_litexpr(" "));
    }
    if (HasFlag(dirty, DirtyState_UnsavedChanges)){
      string_append(&str, string_u8_litexpr("*"));
    }
    if (HasFlag(dirty, DirtyState_UnloadedChanges)){
      string_append(&str, string_u8_litexpr("!"));
    }
    push_fancy_string(scratch, &list, pop2_color, str.string);
  }
  
  if(additional_string){
    push_fancy_string(scratch, &list, pop2_color, *additional_string);
  }
  
  Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
  draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}


function void
RenderCaller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
  ProfileScope(app, "default render caller");
  View_ID active_view = get_active_view(app, Access_Always);
  b32 is_active_view = (active_view == view_id);
  
  Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
  Rect_f32 region = DrawBGAndMargin(app, view_id, buffer, is_active_view);
  Rect_f32 prev_clip = draw_set_clip(app, region);
  
  Face_ID face_id = get_face_id(app, buffer);
  Face_Metrics face_metrics = get_face_metrics(app, face_id);
  f32 line_height = face_metrics.line_height;
  f32 digit_advance = face_metrics.decimal_digit_advance;
  
  // NOTE(allen): file bar
  b64 showing_file_bar = false;
  if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
    Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
    
    String_Const_u8 macro = SCu8(" MACRO!");
    if(global_keyboard_macro_is_recording) krDrawFileBar(app, view_id, buffer, face_id, pair.min, &macro);
    else krDrawFileBar(app, view_id, buffer, face_id, pair.min);
    region = pair.max;
  }
  
  Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
  
  Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                frame_info.animation_dt, scroll);
  if (!block_match_struct(&scroll.position, &delta.point)){
    block_copy_struct(&scroll.position, &delta.point);
    view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
  }
  if (delta.still_animating){
    animate_in_n_milliseconds(app, 0);
  }
  
  // NOTE(allen): query bars
  region = default_draw_query_bars(app, region, view_id, face_id);
  
  // NOTE(allen): FPS hud
  if (show_fps_hud){
    Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
    draw_fps_hud(app, frame_info, face_id, pair.max);
    region = pair.min;
    animate_in_n_milliseconds(app, 1000);
  }
  
  // NOTE(allen): layout line numbers
  b32 show_line_number_margins = def_get_config_b32(vars_save_string_lit("show_line_number_margins"));
  Rect_f32 line_number_rect = {};
  if (show_line_number_margins){
    Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
    line_number_rect = pair.min;
    region = pair.max;
  }
  
  // NOTE(allen): begin buffer render
  Buffer_Point buffer_point = scroll.position;
  Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
  
  // NOTE(allen): draw line numbers
  if (show_line_number_margins){
    draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
  }
  
  // NOTE(allen): draw the buffer
  RenderBuffer(app, view_id, face_id, buffer, text_layout_id, region);
  
  text_layout_free(app, text_layout_id);
  draw_set_clip(app, prev_clip);
}

