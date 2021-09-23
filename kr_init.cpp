// KRFIX 4coder_lister_base.cpp:298
// i32 max_count = first_index + lister->visible_count + 4;
// count = clamp_top(lister->filtered.count, max_count);

// TODO(Krzosa):
// @replace_in_range needs an identifier variant!
// @

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

static float global_margin = 0.f;
#include "4coder_default_include.cpp"

CUSTOM_ID( colors, defcolor_type );
CUSTOM_ID( colors, defcolor_function );
CUSTOM_ID( colors, defcolor_macro );
CUSTOM_ID( colors, defcolor_selection_highlight );
CUSTOM_ID( colors, defcolor_compilation_buffer );

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

#include "kr_text_editing.cpp"
#include "kr_render.cpp"
#include "kr_search_replace_buffer.cpp"
#include "kr_buffers.cpp"
#include "kr_painter_mode.cpp"
#include "kr_fullscreen.cpp"
#include "kr_search.cpp"
#include "kr_processes.cpp"

CUSTOM_COMMAND_SIG(AppStart)
CUSTOM_DOC("Default command for responding to a startup event")
{
  ProfileScope(app, "default startup");
  User_Input input = get_current_input(app);
  if (match_core_code(&input, CoreCode_Startup)){
    String_Const_u8_Array file_names = input.event.core.file_names;
    load_themes_default_folder(app);
    default_4coder_initialize(app, file_names);
    
    
    // NOTE(Krzosa): Triple split with compilation view on the bottom
    //default_4coder_side_by_side_panels(app, file_names);
    InitPanels(app);
    
    b32 auto_load = def_get_config_b32(vars_save_string_lit("automatically_load_project"));
    if (auto_load){
      load_project(app);
      Variable_Handle prj_var = vars_read_key(vars_get_root(), vars_save_string_lit("prj_config"));
      if(vars_is_nil(prj_var)){
        open_all_code_recursive(app);
      }
    }
    split_fullscreen_mode(app);
    
  }
  
#if 0 // Audio memes
  {
    def_audio_init();
    
    Scratch_Block scratch(app);
    FILE *file = def_search_normal_fopen(scratch, "audio_test/raygun_zap.wav", "rb");
    if (file != 0){
      Audio_Clip test_clip = audio_clip_from_wav_FILE(&global_permanent_arena, file);
      fclose(file);
      
      local_persist Audio_Control test_control = {};
      test_control.channel_volume[0] = 1.f;
      test_control.channel_volume[1] = 1.f;
      def_audio_play_clip(test_clip, &test_control);
    }
  }
#endif
  
  {
    def_enable_virtual_whitespace = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
    clear_all_layouts(app);
  }
}

function void
SetupEssentialMapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id){
  MappingScope();
  SelectMapping(mapping);
  
  SelectMap(global_id);
  BindCore(AppStart, CoreCode_Startup);
  BindCore(default_try_exit, CoreCode_TryExit);
  BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);
  BindMouseWheel(mouse_wheel_scroll);
  BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
  
  SelectMap(file_id);
  ParentMap(global_id);
  BindTextInput(write_text_input);
  BindMouse(click_set_cursor_and_mark, MouseCode_Left);
  BindMouseRelease(click_set_cursor, MouseCode_Left);
  BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
  BindMouseMove(click_set_cursor_if_lbutton);
  
  SelectMap(code_id);
  ParentMap(file_id);
  BindTextInput(write_text_and_auto_indent);
}

void
custom_layer_init(Application_Links *app){
  Thread_Context *tctx = get_thread_context(app);
  
  // NOTE(allen): setup for default framework
  default_framework_init(app);
  
  // NOTE(allen): default hooks and command maps
  set_all_default_hooks(app);
  set_custom_hook(app, HookID_RenderCaller, RenderCaller);
  set_custom_hook(app, HookID_WholeScreenRenderCaller, painter_whole_screen_render_caller);
  mapping_init(tctx, &framework_mapping);
  String_ID global_map_id = vars_save_string_lit("keys_global");
  String_ID file_map_id = vars_save_string_lit("keys_file");
  String_ID code_map_id = vars_save_string_lit("keys_code");
#if OS_MAC
  setup_mac_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#else
  setup_default_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#endif
	//setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
  SetupEssentialMapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

