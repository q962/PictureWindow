#include "AppWindow.h"
#include "AppPictureView.h"
#include "AppViewNavigator.h"
#include "AppBinBox.h"
#include "AppEntryBufferVerify.h"
#include "AppEntry.h"
#include "AppFolderHistory.h"
#include "AppPictureList.h"
#include "utils/utils.h"

#if 1  // static function

static void _new_window( GtkApplication* app, const char** path )
{
	AppWindow* win = app_window_new( path );

	gtk_window_set_application( ( gpointer )win, app );

	gtk_window_present( ( gpointer )win );
}

static void _on_new_window( GtkWidget* button )
{
	GtkRoot*        root = gtk_widget_get_root( button );
	GtkApplication* app  = gtk_window_get_application( ( gpointer )root );

	_new_window( app, NULL );
}

static void _open_folder( GtkListView* self, guint position, gpointer user_data )
{
	GtkPopover* popover = user_data;

	GtkRoot*        root = gtk_widget_get_root( ( gpointer )self );
	GtkApplication* app  = gtk_window_get_application( ( gpointer )root );

	AppFolderHistory* history = app_folder_history_get();

	const char* paths[ 2 ] = { app_folder_history_index( history, position ), NULL };

	_new_window( app, paths );

	g_object_unref( history );

	gtk_popover_popdown( popover );
}

GtkHeaderBar* _header_bar_new()
{
	GtkHeaderBar* bar = GTK_HEADER_BAR( gtk_header_bar_new() );

	GtkWidget* left_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 4 );
	gtk_widget_set_valign( left_box, GTK_ALIGN_CENTER );
	gtk_widget_set_vexpand( left_box, FALSE );
	gtk_header_bar_pack_start( bar, ( gpointer )left_box );
	{
		GtkWidget* new_window_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 4 );
		gtk_widget_set_valign( new_window_box, GTK_ALIGN_CENTER );
		gtk_widget_set_vexpand( new_window_box, FALSE );
		gtk_widget_add_css_class( new_window_box, "headerbar-item" );
		gtk_widget_set_parent( new_window_box, left_box );

		GtkWidget* new_window_btn = gtk_button_new_from_icon_name( "window-new-symbolic" );
		gtk_widget_set_parent( new_window_btn, new_window_box );
		g_signal_connect( new_window_btn, "clicked", G_CALLBACK( _on_new_window ), NULL );

		GtkPopover* popover = ( gpointer )gtk_popover_new();
		gtk_popover_set_has_arrow( popover, FALSE );
		gtk_widget_set_parent( ( gpointer )popover, new_window_box );

		// GtkWidget* choose_btn = gtk_button_new_from_icon_name( "go-down-symbolic" );
		// gtk_widget_set_parent( choose_btn, new_window_box );
		// g_signal_connect_swapped( choose_btn, "clicked", G_CALLBACK( gtk_popover_popup ), popover );

		// AppFolderHistory*  history         = app_folder_history_get();
		// GtkSelectionModel* selection_model = ( gpointer )gtk_no_selection_new( ( gpointer )history );
		// GtkListView*       list_view       = ( gpointer )gtk_list_view_new(
		//   ( gpointer )selection_model,
		//   gtk_builder_list_item_factory_new_from_resource( NULL, APPRESPREFIX "ui/folder-item.ui" ) );
		// gtk_popover_set_child( popover, ( gpointer )list_view );

		// g_signal_connect( list_view, "activate", G_CALLBACK( _open_folder ), popover );

		// gtk_list_view_set_single_click_activate( ( gpointer )list_view, TRUE );
	}
	{
		GtkWidget* paste_clipboard_btn = gtk_button_new_from_icon_name( "edit-paste-symbolic" );
		gtk_widget_add_css_class( paste_clipboard_btn, "headerbar-item" );
		gtk_widget_set_parent( paste_clipboard_btn, left_box );
		gtk_actionable_set_action_name( ( gpointer )paste_clipboard_btn, "win.paste-clipboard" );
	}

	return bar;
}

#endif

typedef enum {
	PICTURE_LOOP_PLAYING,
	PICTURE_LOOP_PAUSED
} PictureLoopState;

static G_DEFINE_ENUM_TYPE( PictureLoopState,
                           picture_loop_state,
                           G_DEFINE_ENUM_VALUE( PICTURE_LOOP_PLAYING, "playing" ),
                           G_DEFINE_ENUM_VALUE( PICTURE_LOOP_PAUSED, "paused" ) );

#if 1  // gobject definition :: AppWindow

struct _AppWindow {
	GtkApplicationWindow parent_instance;

	GtkRevealer* entity_revealer;
	// AppBinBox*   window_content;

	AppPictureList* picture_list;

	GtkWidget *prev_button, *next_button;

	PictureLoopMode  loop_mode;
	PictureLoopState loop_state;
	guint            interval;

	GtkWidget* controller_bar;

	GtkStack*  picture_view_stack;
	GPtrArray* focus_list;  //! GTK-BUG: https://gitlab.gnome.org/GNOME/gtk/-/issues/8059

	gboolean          view_navigator_visible;
	AppViewNavigator* view_navigator;
	GtkRevealer*      view_navigator_revealer;
	AppBinBox*        view_navigator_container;

	GtkPopover* window_menu_popover;

	gboolean is_motion_leave;

	double mouse_x, mouse_y;

	GMenu* main_menu;

	char* bg_color;

	GtkButton* map_btn;

	guint hide_ui_timer_id;
	guint loop_picture_timer_id;

	GtkWidget* header_bar;

	gboolean prevent_hide_ui;

	gboolean ui_visible;

	GtkRevealer* loop_controller_revealer;

	gboolean can_loop;

	gboolean in_entry;

	gboolean can_move_view;
};

enum {
	PROP_0,
	PROP_INTERVAL,
	PROP_LOOP_MODE,
	PROP_LOOP_STATE,
	PROP_N,
};
static GParamSpec* specsp[ PROP_N ] = {};

static guint SIGNAL_VISIBLE_POPOVER = 0;

G_DEFINE_TYPE( AppWindow, app_window, gtk_application_window_get_type() )

#if 1  // static function

// static void app_window_print_focus_widget( GtkWindow* window, gpointer unuse, AppWindow* self )
// {
// 	GString* str = g_string_new( NULL );
// 	g_string_append_printf( str, "focus: %s(%p)", gtk_widget_get_name( ( gpointer )self ), self );

// 	for ( GtkWidget* focus_widget = gtk_widget_get_focus_child( ( gpointer )self ); focus_widget;
// 	      focus_widget            = gtk_widget_get_focus_child( focus_widget ) ) {
// 		g_string_append_printf( str, " -> %s(%p)", gtk_widget_get_name( focus_widget ), focus_widget );
// 	}

// 	g_message( "%s", str->str );

// 	g_string_free( str, TRUE );
// }

static void _show_picture( AppWindow* self, const char* picture_path )
{
	char* basename = g_path_get_basename( picture_path );
	gtk_window_set_title( ( gpointer )self, basename );
	g_free( basename );

	AppPictureView* picture_view = app_picture_view_new( picture_path, NULL );

	if ( !picture_view ) {
		GtkWidget* load_failed = gtk_stack_get_child_by_name( self->picture_view_stack, "load-failed" );
		gtk_widget_set_visible( load_failed, TRUE );

		gtk_stack_set_visible_child_name( self->picture_view_stack, "load-failed" );

		return;
	}

	app_picture_view_config( picture_view, APP_PICTURE_VIEW_CONFIG_CAN_MOVE, GINT_TO_POINTER( self->can_move_view ) );

	app_picture_view_play( picture_view );

	gtk_stack_add_child( self->picture_view_stack, ( gpointer )picture_view );
	gtk_stack_set_visible_child( self->picture_view_stack, ( gpointer )picture_view );
}

static void _show_prev_picture( AppWindow* self )
{
	const char* picture_path = app_picture_list_prev( self->picture_list );
	if ( !picture_path )
		return;

	if ( app_picture_list_prev( self->picture_list ) ) {
		app_picture_list_next( self->picture_list );
	}
	else {
		gtk_widget_set_sensitive( self->prev_button, FALSE );
	}

	_show_picture( self, picture_path );
}

static void _show_next_picture( AppWindow* self )
{
	const char* picture_path = app_picture_list_next( self->picture_list );
	if ( !picture_path )
		return;

	if ( app_picture_list_can_prev( self->picture_list ) )
		gtk_widget_set_sensitive( self->prev_button, TRUE );

	_show_picture( self, picture_path );
}

static void _first_load_picture( AppWindow* self )
{
	_show_next_picture( self );
}

static void _on_zoom_custom_entry_activate( GtkEntry* entry, AppWindow* self )
{
	double      zoom   = 0;
	const char* buffer = gtk_editable_get_text( ( gpointer )entry );

	sscanf( buffer, "%lf", &zoom );

	gtk_widget_activate_action( ( gpointer )self, "picture.zoom", "d", zoom / 100.0 );
}

static void _on_opacity_custom_entry_activate( GtkEntry* entry, AppWindow* self )
{
	double      opacity = 0;
	const char* buffer  = gtk_editable_get_text( ( gpointer )entry );

	if ( buffer[ 0 ] == '\0' )
		return;

	sscanf( buffer, "%lf", &opacity );

	g_message( "opacity: %lf", opacity );

	gtk_widget_activate_action( ( gpointer )self, "win.opacity", "d", opacity / 100.0 );
}

static void _app_window_popup_menu( AppWindow* self )
{
	if ( app_picture_list_n_items( self->picture_list ) == 0 )
		return;

	if ( !self->main_menu ) {
		self->main_menu = g_menu_new();

		GMenu* picture_menu = g_menu_new();
		{
			g_menu_append( picture_menu, _( "自适应缩放" ), "picture.zoom-default" );
			g_menu_append( picture_menu, _( "居中" ), "picture.center" );
			g_menu_append( picture_menu, _( "缩放至 100%" ), "picture.zoom(1.0)" );

			GMenu* zoom_menu = g_menu_new();
			{
				g_menu_append( zoom_menu, "150%", "picture.zoom(1.5)" );
				g_menu_append( zoom_menu, "200%", "picture.zoom(2.0)" );
				g_menu_append( zoom_menu, "300%", "picture.zoom(3.0)" );
				g_menu_append( zoom_menu, "500%", "picture.zoom(5.0)" );
				g_menu_append( zoom_menu, "700%", "picture.zoom(7.0)" );
				g_menu_append( zoom_menu, "1000%", "picture.zoom(10.0)" );
				g_menu_append( zoom_menu, "2000%", "picture.zoom(20.0)" );

				GMenuItem* zoom_custom = g_menu_item_new( NULL, NULL );
				g_menu_item_set_attribute( zoom_custom, "custom", "s", "zoom-custom" );
				g_menu_append_item( zoom_menu, zoom_custom );
			}

			g_menu_append_submenu( picture_menu, _( "缩放至" ), ( gpointer )zoom_menu );

			g_object_unref( zoom_menu );
		}

		GMenu* window_menu = g_menu_new();
		{
			GMenu* bg_menu = g_menu_new();
			{
				g_menu_append( bg_menu, _( "透明" ), "win.background('#00000000')" );
				g_menu_append( bg_menu, _( "灰色" ), "win.background('#808080')" );
				g_menu_append( bg_menu, _( "白色" ), "win.background('#FFFFFF')" );
				g_menu_append( bg_menu, _( "黑色" ), "win.background('#000000')" );
				g_menu_append( bg_menu, _( "主题色" ), "win.background('#theme-color')" );
				g_menu_append( bg_menu, _( "自定义" ), "win.background-custom" );
			}

			GMenu* opacity_menu = g_menu_new();
			{
				g_menu_append( opacity_menu, "1%", "win.opacity(0.1)" );
				g_menu_append( opacity_menu, "25%", "win.opacity(0.25)" );
				g_menu_append( opacity_menu, "50%", "win.opacity(0.50)" );
				g_menu_append( opacity_menu, "75%", "win.opacity(0.75)" );
				g_menu_append( opacity_menu, "100%", "win.opacity(1.0)" );

				GMenuItem* opacity_custom = g_menu_item_new( NULL, NULL );
				g_menu_item_set_attribute( opacity_custom, "custom", "s", "opacity-custom" );
				g_menu_append_item( opacity_menu, opacity_custom );
			}

			g_menu_append_submenu( window_menu, _( "背景色" ), ( gpointer )bg_menu );
			g_menu_append_submenu( window_menu, _( "窗口不透明度" ), ( gpointer )opacity_menu );
			g_menu_append( window_menu, _( "UI 隐藏/显示" ), "win.toggle-ui-visibility" );

			g_object_unref( bg_menu );
			g_object_unref( opacity_menu );
		}

		g_menu_append_section( self->main_menu, NULL, ( gpointer )picture_menu );
		g_menu_append_section( self->main_menu, NULL, ( gpointer )window_menu );

		g_object_unref( picture_menu );
		g_object_unref( window_menu );

		//////////////////////

		self->window_menu_popover = ( gpointer )gtk_popover_menu_new_from_model( ( gpointer )self->main_menu );
		gtk_widget_add_css_class( ( gpointer )self->window_menu_popover, "menu" );
		gtk_widget_set_parent( ( gpointer )self->window_menu_popover, ( gpointer )self );
		gtk_widget_set_halign( ( gpointer )self->window_menu_popover, GTK_ALIGN_START );
		gtk_popover_set_has_arrow( self->window_menu_popover, FALSE );
		{
			{  // zoom_custom
				GtkEntry* zoom_custom_entry = ( gpointer )gtk_entry_new();
				gtk_entry_set_input_purpose( zoom_custom_entry, GTK_INPUT_PURPOSE_DIGITS );
				gtk_entry_set_placeholder_text( zoom_custom_entry, _( "自定义" ) );
				g_signal_connect( zoom_custom_entry, "activate", G_CALLBACK( _on_zoom_custom_entry_activate ), self );

				AppEntryBufferVerify* verify = app_entry_buffer_verify_new( "^\\d{1,4}$" );
				gtk_entry_set_buffer( zoom_custom_entry, ( gpointer )verify );
				g_object_unref( verify );

				gtk_popover_menu_add_child(
				  ( gpointer )self->window_menu_popover, ( gpointer )zoom_custom_entry, "zoom-custom" );
			}

			{  // opacity_custom
				GtkEntry* opacity_custom_entry = ( gpointer )gtk_entry_new();
				gtk_entry_set_input_purpose( opacity_custom_entry, GTK_INPUT_PURPOSE_DIGITS );
				gtk_entry_set_placeholder_text( opacity_custom_entry, _( "自定义" ) );
				g_signal_connect(
				  opacity_custom_entry, "activate", G_CALLBACK( _on_opacity_custom_entry_activate ), self );

				AppEntryBufferVerify* verify = app_entry_buffer_verify_new( "^(?:[1-9]\\d?|100)$" );
				gtk_entry_set_buffer( opacity_custom_entry, ( gpointer )verify );
				g_object_unref( verify );

				gtk_popover_menu_add_child(
				  ( gpointer )self->window_menu_popover, ( gpointer )opacity_custom_entry, "opacity-custom" );
			}
		}
	}

	GdkRectangle rect = { self->mouse_x, self->mouse_y, 0, 0 };

	if ( self->is_motion_leave ) {
		rect.x = 0;
		rect.y = 0;
	}

	gtk_popover_set_pointing_to( self->window_menu_popover, &rect );
	gtk_popover_popup( self->window_menu_popover );
}

static void _do_loop_picture( AppWindow* self )
{
	if ( app_picture_list_n_items( self->picture_list ) < 2 )
		return;

	if ( self->loop_state == PICTURE_LOOP_PLAYING ) {
		g_object_set( self, "loop-state", GUINT_TO_POINTER( PICTURE_LOOP_PAUSED ), NULL );
	}
	else {
		g_object_set( self, "loop-state", GUINT_TO_POINTER( PICTURE_LOOP_PLAYING ), NULL );
	}
}

static void _stop_loop( AppWindow* self );

static gboolean _on_key_released( GtkEventControllerKey* event,
                                  guint                  keyval,
                                  guint                  keycode,
                                  GdkModifierType        state,
                                  gpointer               user_data )
{
	AppWindow* self = user_data;

	// g_message( "key released: %x", keyval );

	GdkModifierType m_type = gtk_event_controller_get_current_event_state( ( gpointer )event );

	switch ( keyval ) {
		case GDK_KEY_Menu: {
			_app_window_popup_menu( self );
		} break;

		case GDK_KEY_Return:
		case GDK_KEY_KP_Enter: {
			_do_loop_picture( self );
		} break;

		case GDK_KEY_w: {
			if ( m_type & GDK_CONTROL_MASK ) {
				gtk_window_close( ( gpointer )self );
			}
		} break;

		case GDK_KEY_n: {
			if ( m_type & GDK_CONTROL_MASK ) {
				GtkRoot*        root = gtk_widget_get_root( ( gpointer )self );
				GtkApplication* app  = gtk_window_get_application( ( gpointer )root );

				_new_window( app, NULL );
			}
		} break;

		case GDK_KEY_q: {
			if ( m_type & GDK_CONTROL_MASK ) {
				GtkApplication* app = gtk_window_get_application( ( gpointer )self );

				GList* windows = gtk_application_get_windows( app );
				for ( GList* iter = windows; iter; iter = iter->next ) {
					// 退出有延迟
					gtk_widget_set_visible( ( gpointer )iter->data, FALSE );
				}
				g_list_free( windows );

				g_application_quit( ( gpointer )gtk_window_get_application( ( gpointer )self ) );
			}
		} break;

		case GDK_KEY_Left: {
			_stop_loop( self );
			_show_prev_picture( self );
		} break;

		case GDK_KEY_Right: {
			_stop_loop( self );
			_show_next_picture( self );
		} break;

		case GDK_KEY_c: {
			if ( m_type & GDK_CONTROL_MASK ) {
				AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( self->picture_view_stack );
				if ( APP_IS_PICTURE_VIEW( picture_view ) ) {
					GdkClipboard* clipboard = gtk_widget_get_clipboard( ( gpointer )self );

					GdkPaintable* paintable      = app_picture_view_get_paintable( picture_view );
					int           picture_width  = gdk_paintable_get_intrinsic_width( paintable );
					int           picture_height = gdk_paintable_get_intrinsic_height( paintable );

					GtkSnapshot* snapshot = gtk_snapshot_new();

					gdk_paintable_snapshot( paintable, snapshot, picture_width, picture_height );

					GskRenderNode* node = gtk_snapshot_free_to_node( snapshot );

					GskRenderer* renderer = gtk_native_get_renderer( gtk_widget_get_native( ( gpointer )self ) );

					GdkTexture* texture = gsk_renderer_render_texture(
					  renderer, node, &GRAPHENE_RECT_INIT( 0, 0, picture_width, picture_height ) );

					gdk_clipboard_set_texture( clipboard, texture );

					gsk_render_node_unref( node );
					g_object_unref( texture );
				}
			}
		} break;

		case GDK_KEY_v: {
			if ( m_type & GDK_CONTROL_MASK ) {
				gtk_widget_activate_action( ( gpointer )self, "win.paste-clipboard", NULL );
			}
		} break;

		case GDK_KEY_F11: {
			if ( gtk_window_is_fullscreen( ( gpointer )self ) ) {
				gtk_window_unfullscreen( ( gpointer )self );
			}
			else {
				gtk_window_fullscreen( ( gpointer )self );
			}
		} break;

		default: return FALSE;
	}

	return TRUE;
}

static void _show_ui( AppWindow* self )
{
	g_clear_handle_id( &self->hide_ui_timer_id, g_source_remove );

	gtk_widget_set_cursor_from_name( ( gpointer )self, "default" );

	if ( self->ui_visible == FALSE )
		return;

	gtk_revealer_set_reveal_child( self->entity_revealer, TRUE );
}

static void _hide_ui( AppWindow* self )
{
	g_clear_handle_id( &self->hide_ui_timer_id, g_source_remove );

	gtk_widget_set_cursor_from_name( ( gpointer )self, "none" );

	if ( self->ui_visible == FALSE ) {
		/**
		 * 当弹出 popover 时会触发 _on_motion_leave，从而调用 _hide_ui
		 * 但是，项目规定，如果是因为弹出 popover 而导致的隐藏，则不隐藏，从而调用 _show_ui，
		 * 并使用 prevent_hide_ui 阻止隐藏 UI
		 * 所以，当从菜单项触发隐藏 UI 时，则不需要做任何时，因为 UI 已经隐藏
		 * 关闭 popover 会调用 _show_ui，而 ui_visible 已经被置否，所以不会有任何动作
		 */
		return;
	}

	if ( self->prevent_hide_ui )
		return;

	gtk_revealer_set_reveal_child( self->entity_revealer, FALSE );
}

static gboolean _is_entry( GtkWidget* widget, double x, double y )
{
	gboolean is_entity = FALSE;

	for ( GtkWidget* parent = widget; parent; ( parent = gtk_widget_get_parent( parent ) ) ) {
		if ( gtk_widget_has_css_class( parent, "entity" ) ) {
			is_entity = TRUE;
			break;
		}
	}

	return is_entity;
}

static void _on_motion( GtkEventController* motion_controller, gdouble x, gdouble y, gpointer user_data )
{
	AppWindow* self = user_data;

	self->mouse_x = x;
	self->mouse_y = y;

	_show_ui( self );

	if ( !self->prevent_hide_ui ) {
		self->hide_ui_timer_id = g_timeout_add_once( 5000, ( GSourceOnceFunc )_hide_ui, self );
	}

	GtkWidget* pick = gtk_widget_pick( ( gpointer )self, x, y, GTK_PICK_NON_TARGETABLE );
	if ( !pick )
		return;

	self->in_entry = _is_entry( pick, x, y );

	if ( self->in_entry ) {
		gtk_widget_set_can_target( ( gpointer )self->entity_revealer, TRUE );
	}
	else {
		gtk_widget_set_can_target( ( gpointer )self->entity_revealer, FALSE );
	}
}

static void _hide_view_navigator( AppWindow* self );

static void _on_motion_enter( GtkEventController* motion_controller, gpointer user_data )
{
	AppWindow* self = user_data;

	self->is_motion_leave = FALSE;

	_show_ui( self );

	if ( self->view_navigator_visible )
		_hide_view_navigator( self );
}

static void _print_parent( GtkWidget* widget )
{
	;
	GString* string = g_string_new( NULL );

	for ( GtkWidget* parent = widget; parent; parent = gtk_widget_get_parent( parent ) ) {
		g_string_append_printf( string, "-> %s(%p) ", gtk_widget_get_name( parent ), parent );
	}

	g_message( "%s", string->str );
}

static void _on_motion_leave( GtkEventController* motion_controller, gpointer user_data )
{
	AppWindow* self = user_data;

	self->is_motion_leave = TRUE;

	_hide_ui( self );
}

static void _stop_loop( AppWindow* self )
{
	g_object_set( self, "loop-state", GUINT_TO_POINTER( PICTURE_LOOP_PAUSED ), NULL );
}

static void _on_click_pressed( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, AppWindow* self )
{
	switch ( gtk_gesture_single_get_current_button( ( gpointer )event ) ) {
		case GDK_BUTTON_PRIMARY: {
			if ( n_press == 1 ) {
				GtkWindow* window = ( gpointer )self;

				GtkWidget* pick = gtk_widget_pick( ( gpointer )self, x, y, GTK_PICK_NON_TARGETABLE );

				gboolean is_entity = _is_entry( pick, x, y );

				if ( pick && !is_entity ) {
					if ( !self->can_move_view ) {
						GdkToplevel* toplevel = GDK_TOPLEVEL( gtk_native_get_surface( ( gpointer )self ) );

						gdk_toplevel_begin_move( toplevel,
						                         gtk_event_controller_get_current_event_device( ( gpointer )event ),
						                         GDK_BUTTON_PRIMARY,
						                         x,
						                         y,
						                         gtk_event_controller_get_current_event_time( ( gpointer )event ) );
					}
				}
			}
		}
	}
}

static void _on_click_released( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, AppWindow* self )
{
	GdkModifierType m_type = gtk_event_controller_get_current_event_state( ( gpointer )event );

	if ( GDK_BUTTON5_MASK & m_type ) {
		_stop_loop( self );
		// 不这么做会导致控件得不到释放，可能是当前焦点控件
		// 我想这是个 BUG
		gtk_gesture_set_state( ( gpointer )event, GTK_EVENT_SEQUENCE_CLAIMED );
		_show_next_picture( self );
		return;
	}
	if ( GDK_BUTTON4_MASK & m_type ) {
		_stop_loop( self );
		gtk_gesture_set_state( ( gpointer )event, GTK_EVENT_SEQUENCE_CLAIMED );
		_show_prev_picture( self );
		return;
	}

	switch ( gtk_gesture_single_get_current_button( ( gpointer )event ) ) {
		case GDK_BUTTON_PRIMARY: {
			if ( n_press == 2 ) {
				GtkWindow* window = GTK_WINDOW( gtk_widget_get_root( ( gpointer )self ) );

				GtkWidget* pick = gtk_widget_pick( ( gpointer )self, x, y, GTK_PICK_NON_TARGETABLE );

				gboolean is_entity = _is_entry( pick, x, y );

				if ( pick && !is_entity ) {
					if ( gtk_window_is_fullscreen( window ) ) {
						gtk_window_unfullscreen( window );
					}
					else {
						gtk_window_fullscreen( window );
					}
					//
					gtk_gesture_set_state( ( gpointer )event, GTK_EVENT_SEQUENCE_DENIED );
				}
			}
		} break;
		case GDK_BUTTON_SECONDARY: {
			if ( !self->in_entry )
				_app_window_popup_menu( self );
		} break;
		default: return;
	}
}

static void _drag_gesture_update_cb( GtkGestureDrag* gesture, double offset_x, double offset_y, AppWindow* self )
{
	double start_x, start_y;

	AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( self->picture_view_stack );

	if ( !APP_IS_PICTURE_VIEW( picture_view ) )
		return;

	gtk_gesture_drag_get_start_point( gesture, &start_x, &start_y );

	gboolean is_inside_picture = app_picture_view_contains( picture_view, start_x, start_y );

	if ( is_inside_picture ) {
		gtk_gesture_set_state( ( gpointer )gesture, GTK_EVENT_SEQUENCE_DENIED );
		return;
	}

	GdkToplevel* toplevel =
	  GDK_TOPLEVEL( gtk_native_get_surface( ( gpointer )gtk_widget_get_native( ( gpointer )self ) ) );

	gdk_toplevel_begin_move( toplevel,
	                         gtk_gesture_get_device( ( gpointer )gesture ),
	                         GDK_BUTTON_PRIMARY,
	                         offset_x,
	                         offset_y,
	                         gtk_event_controller_get_current_event_time( GTK_EVENT_CONTROLLER( gesture ) ) );
}
static void _on_picture_view_moving( AppPictureView* picture_view, gboolean is_moving, gpointer user_data )
{
	AppWindow* self = user_data;

	if ( self->is_motion_leave )
		return;

	if ( is_moving ) {
		_hide_ui( self );
	}
	else {
		_show_ui( self );
	}
}

static void _update_view_navigator_view_rect( AppWindow* self,
                                              double     position_x,
                                              double     position_y,
                                              double     picture_view_width,
                                              double     picture_view_height,
                                              double     zoom )
{
	double width  = gtk_widget_get_width( ( gpointer )self );
	double height = gtk_widget_get_height( ( gpointer )self );

	float x = fabs( position_x );
	float y = fabs( position_y );
	float w = width;
	float h = height;

	if ( position_x > 0 ) {
		w = width - position_x;
		x = 0;
	}
	if ( position_y > 0 ) {
		h = height - position_y;
		y = 0;
	}

	if ( position_x + picture_view_width < width ) {
		w = position_x + picture_view_width;
	}
	if ( position_y + picture_view_height < height ) {
		h = position_y + picture_view_height;
	}

	w = fminf( w, picture_view_width );
	h = fminf( h, picture_view_height );

	x /= zoom;
	y /= zoom;
	w /= zoom;
	h /= zoom;

	app_view_navigator_set_view_rect( self->view_navigator, &GRAPHENE_RECT_INIT( x, y, w, h ) );
}

static void _compute_picture_view_zoom_to_window_ratio( AppWindow*      self,
                                                        AppPictureView* picture_view,
                                                        double*         width_ratio,
                                                        double*         height_ratio )
{
	double width  = gtk_widget_get_width( ( gpointer )self );
	double height = gtk_widget_get_height( ( gpointer )self );

	double    zoom_target  = 0;
	GVariant* picture_size = NULL;
	g_object_get( ( gpointer )picture_view,
	              APP_PICTURE_VIEW_PROP_PICTURE_SIZE,
	              &picture_size,
	              APP_PICTURE_VIEW_PROP_ZOOM_TARGET,
	              &zoom_target,
	              NULL );

	guint picture_width = 0, picture_height = 0;

	g_variant_get( picture_size, "(uu)", &picture_width, &picture_height );

	double picture_view_width_target  = picture_width * zoom_target;
	double picture_view_height_target = picture_height * zoom_target;

	if ( width_ratio )
		*width_ratio = picture_view_width_target / width;
	if ( height_ratio )
		*height_ratio = picture_view_height_target / height;

	g_variant_unref( picture_size );
}

static void _on_picture_view_zoom_changed( AppPictureView* picture_view, GParamSpec* spec, AppWindow* self )
{
	double zoom        = 0;
	double zoom_target = 0;

	GVariant* picture_view_size  = NULL;
	double    picture_view_width = 0, picture_view_height = 0;

	GVariant* picture_position = NULL;
	double    position_x = 0, position_y = 0;

	g_object_get( ( gpointer )picture_view,
	              APP_PICTURE_VIEW_PROP_ZOOM,
	              &zoom,
	              APP_PICTURE_VIEW_PROP_ZOOM_TARGET,
	              &zoom_target,
	              APP_PICTURE_VIEW_PROP_PICTURE_VIEW_SIZE,
	              &picture_view_size,
	              APP_PICTURE_VIEW_PROP_PICTURE_POSITION,
	              &picture_position,
	              NULL );

	g_variant_get( picture_view_size, "(dd)", &picture_view_width, &picture_view_height );
	g_variant_get( picture_position, "(dd)", &position_x, &position_y );

	double width_ratio = 0, height_ratio = 0;
	_compute_picture_view_zoom_to_window_ratio( self, picture_view, &width_ratio, &height_ratio );

	if ( width_ratio - 1.0 <= 0.000001 && height_ratio - 1.0 <= 0.000001 ) {
		if ( self->view_navigator_visible ) {
			gtk_revealer_set_reveal_child( self->view_navigator_revealer, FALSE );
		}

		gtk_widget_set_visible( ( gpointer )self->map_btn, FALSE );
	}
	else {
		if ( self->view_navigator_visible ) {
			gtk_revealer_set_reveal_child( self->view_navigator_revealer, TRUE );
		}

		gtk_widget_set_visible( ( gpointer )self->map_btn, !self->view_navigator_visible );

		g_object_get( ( gpointer )picture_view, APP_PICTURE_VIEW_PROP_PICTURE_POSITION, &picture_position, NULL );

		_update_view_navigator_view_rect( self, position_x, position_y, picture_view_width, picture_view_height, zoom );
	}

	g_variant_unref( picture_view_size );
	g_variant_unref( picture_position );
}

static void _on_picture_view_position_changed( AppPictureView* picture_view, GParamSpec* spec, AppWindow* self )
{
	double    zoom              = 0;
	GVariant* picture_position  = NULL;
	GVariant* picture_view_size = NULL;
	GVariant* picture_size      = NULL;
	g_object_get( ( gpointer )picture_view,
	              APP_PICTURE_VIEW_PROP_PICTURE_POSITION,
	              &picture_position,
	              APP_PICTURE_VIEW_PROP_PICTURE_VIEW_SIZE,
	              &picture_view_size,
	              APP_PICTURE_VIEW_PROP_PICTURE_SIZE,
	              &picture_size,
	              APP_PICTURE_VIEW_PROP_ZOOM,
	              &zoom,
	              NULL );

	double position_x = 0, position_y = 0;
	guint  picture_width = 0, picture_height = 0;
	double picture_view_width = 0, picture_view_height = 0;

	g_variant_get( picture_position, "(dd)", &position_x, &position_y );
	g_variant_get( picture_view_size, "(dd)", &picture_view_width, &picture_view_height );
	g_variant_get( picture_size, "(uu)", &picture_width, &picture_height );

	_update_view_navigator_view_rect( self, position_x, position_y, picture_view_width, picture_view_height, zoom );

	g_variant_unref( picture_position );
	g_variant_unref( picture_size );
	g_variant_unref( picture_view_size );
}

static void _view_to_position( AppViewNavigator* view_navigator, double x, double y, AppWindow* self )
{
	AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( self->picture_view_stack );

	if ( !APP_IS_PICTURE_VIEW( picture_view ) )
		return;

	app_picture_view_center_to( picture_view, x, y, FALSE );
}

static void _show_view_navigator( GtkButton* btn, AppWindow* self )
{
	self->view_navigator_visible = TRUE;

	gtk_revealer_set_reveal_child( self->view_navigator_revealer, self->view_navigator_visible );
	gtk_widget_set_visible( ( gpointer )self->map_btn, !self->view_navigator_visible );
}

static void _hide_view_navigator( AppWindow* self )
{
	self->view_navigator_visible = FALSE;

	gtk_revealer_set_reveal_child( self->view_navigator_revealer, self->view_navigator_visible );
	gtk_widget_set_visible( ( gpointer )self->map_btn, !self->view_navigator_visible );
}

static void _remove_all_old_picture_view( AppWindow* self )
{
	GtkWidget* picture_view = gtk_stack_get_visible_child( self->picture_view_stack );

	GtkWidget* child = gtk_widget_get_first_child( ( gpointer )self->picture_view_stack );

	while ( child ) {
		GtkWidget* next = gtk_widget_get_next_sibling( child );

		if ( picture_view != child && APP_IS_PICTURE_VIEW( child ) ) {
			g_ptr_array_add( self->focus_list, child );
			{
				gpointer* p_child = &g_ptr_array_index( self->focus_list, self->focus_list->len - 1 );
				g_object_add_weak_pointer( ( gpointer )child, p_child );
			}

			// @see self->focus_list
			gtk_stack_remove( self->picture_view_stack, child );
			gtk_widget_grab_focus( picture_view );
		}

		child = next;
	}
}

static void _on_stack_visible_child_changed( GtkStack* stack, GParamSpec* spec, AppWindow* self )
{
	GtkSettings* settings              = gtk_settings_get_default();
	gboolean     gtk_enable_animations = FALSE;
	g_object_get( settings, "gtk-enable-animations", &gtk_enable_animations, NULL );

	GtkStackTransitionType type = gtk_stack_get_transition_type( self->picture_view_stack );

	if ( !gtk_enable_animations || GTK_STACK_TRANSITION_TYPE_NONE == type ||
	     /** 这表明，动画还为结束就切换至新的控件
	      * 如果 GTK 能够正确处理 focus 则不需要这么做
	      * @see self->focus_list
	      */
	     gtk_stack_get_transition_running( stack ) == TRUE ) {
		_remove_all_old_picture_view( self );
	}

	for ( GtkWidget* child = gtk_widget_get_first_child( ( gpointer )self->picture_view_stack );
	      APP_IS_PICTURE_VIEW( child );
	      child = gtk_widget_get_next_sibling( child ) )  //
	{
		g_signal_handlers_disconnect_by_func( child, G_CALLBACK( _on_picture_view_zoom_changed ), self );
		g_signal_handlers_disconnect_by_func( child, G_CALLBACK( _on_picture_view_position_changed ), self );
		g_signal_handlers_disconnect_by_func( child, G_CALLBACK( _on_picture_view_moving ), self );
	}

	AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( stack );

	if ( APP_IS_PICTURE_VIEW( picture_view ) ) {
		app_view_navigator_set_paintable( self->view_navigator, app_picture_view_get_paintable( picture_view ) );
		gtk_widget_queue_resize( ( gpointer )self->view_navigator );

		g_signal_connect( ( gpointer )picture_view,
		                  "notify::" APP_PICTURE_VIEW_PROP_ZOOM,
		                  G_CALLBACK( _on_picture_view_zoom_changed ),
		                  self );
		g_signal_connect( ( gpointer )picture_view,
		                  "notify::" APP_PICTURE_VIEW_PROP_PICTURE_POSITION,
		                  G_CALLBACK( _on_picture_view_position_changed ),
		                  self );

		g_signal_connect( picture_view, APP_PICTURE_VIEW_SIGNAL_MOVING, G_CALLBACK( _on_picture_view_moving ), self );
	}
	else {
		gtk_widget_set_visible( ( gpointer )self->map_btn, FALSE );
	}

	gtk_widget_action_set_enabled( ( gpointer )self, "picture.zoom-default", APP_IS_PICTURE_VIEW( picture_view ) );
	gtk_widget_action_set_enabled( ( gpointer )self, "picture.center", APP_IS_PICTURE_VIEW( picture_view ) );
	gtk_widget_action_set_enabled( ( gpointer )self, "picture.zoom", APP_IS_PICTURE_VIEW( picture_view ) );
}

static void _on_stack_transition_running( GtkStack* stack, GParamSpec* spec, AppWindow* self )
{
	if ( !gtk_stack_get_transition_running( stack ) ) {
		_remove_all_old_picture_view( self );
	}
}

static AppPictureView* _app_window_get_picture_view( AppWindow* self )
{
	return APP_PICTURE_VIEW( gtk_stack_get_visible_child( self->picture_view_stack ) );
}

static void _app_window_picture_zoom_default( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	AppPictureView* picture_view = _app_window_get_picture_view( self );
	if ( !picture_view )
		return;

	app_picture_view_to_fit_contain( picture_view );
}

static void _app_window_picture_center( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	AppPictureView* picture_view = _app_window_get_picture_view( self );
	if ( !picture_view )
		return;

	app_picture_view_to_center( picture_view );
}

static void _app_window_picture_zoom( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	g_return_if_fail( parameters != NULL );

	AppPictureView* picture_view = _app_window_get_picture_view( self );
	if ( !picture_view )
		return;

	double zoom = g_variant_get_double( parameters );

	app_picture_view_set_zoom( picture_view, zoom, TRUE );
}

static void _interval_increase( AppWindow* self )
{
	g_object_set( ( gpointer )self, "interval", self->interval + 100, NULL );
}

static void _interval_decrease( AppWindow* self )
{
	g_object_set( ( gpointer )self, "interval", self->interval - 100, NULL );
}

static void _on_interval_entry_changed( GtkEntry* entry, AppWindow* self )
{
	const char* buffer   = gtk_editable_get_text( ( gpointer )entry );
	float       interval = 0;

	sscanf( buffer, "%fs", &interval );

	g_object_set( ( gpointer )self, "interval", ( guint )( interval * 1000.0 ), NULL );
}

static void _on_interval_changed( AppWindow* self, GParamSpec* spec, gpointer user_data )
{
	GtkEntry* entry = user_data;

	PictureLoopState state = 0;

	g_object_get( self, "loop-state", &state, NULL );
	if ( state == PICTURE_LOOP_PLAYING ) {
		g_object_set( self, "loop-state", GUINT_TO_POINTER( PICTURE_LOOP_PAUSED ), NULL );
		g_object_set( self, "loop-state", GUINT_TO_POINTER( PICTURE_LOOP_PLAYING ), NULL );
	}

	char* format = g_strdup_printf( "%.1f", self->interval / 1000.0 );

	gtk_editable_set_text( ( gpointer )entry, format );
	gtk_widget_set_tooltip_text( ( gpointer )entry, gtk_editable_get_text( ( gpointer )entry ) );

	g_free( format );
}

static void _change_mode( GtkCheckButton* btn, gpointer user_data )
{
	AppWindow* self = user_data;

	if ( !gtk_check_button_get_active( btn ) )
		return;

	PictureLoopMode btn_mode = GPOINTER_TO_UINT( g_object_get_data( ( gpointer )btn, "mode" ) );

	g_object_set( ( gpointer )self, "loop-mode", GUINT_TO_POINTER( btn_mode ), NULL );
}

static void _on_mode_changed( AppWindow* self, GParamSpec* spec, gpointer user_data )
{
	GtkCheckButton* mode_btn = user_data;

	PictureLoopMode btn_mode = GPOINTER_TO_UINT( g_object_get_data( ( gpointer )mode_btn, "mode" ) );

	if ( self->loop_mode == btn_mode ) {
		gtk_check_button_set_active( mode_btn, TRUE );
		app_picture_list_set_mode( self->picture_list, btn_mode );
	}
}

static void _on_can_move_view_check_toggle( GtkCheckButton* check, AppWindow* self )
{
	self->can_move_view = gtk_check_button_get_active( check );

	AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( self->picture_view_stack );
	if ( APP_IS_PICTURE_VIEW( picture_view ) ) {
		app_picture_view_config(
		  picture_view, APP_PICTURE_VIEW_CONFIG_CAN_MOVE, GINT_TO_POINTER( self->can_move_view ) );

		if ( !self->can_move_view ) {
			app_picture_view_to_fit_contain( picture_view );
		}
	}
}

static void _app_window_on_popover_visible_changed( AppWindow* window, GtkPopover* popover, gpointer user_data )
{
	AppWindow* self = ( gpointer )window;

	if ( app_picture_list_n_items( self->picture_list ) == 0 ) {
		return;
	}

	self->prevent_hide_ui = gtk_widget_get_visible( ( gpointer )popover );

	gtk_widget_set_can_target( ( gpointer )self, !self->prevent_hide_ui );

	if ( self->prevent_hide_ui ) {
		_show_ui( self );
	}
	else {
		_hide_ui( self );
	}
}
static gboolean _show_next_picture_wrapper( AppWindow* self )
{
	app_picture_list_truncation( self->picture_list );

	_show_next_picture( self );

	return G_SOURCE_CONTINUE;
}

static void _on_loop_state_changed( AppWindow* self, GParamSpec* pspec, GtkButton* play_button )
{
	if ( self->loop_state == PICTURE_LOOP_PLAYING ) {
		gtk_button_set_icon_name( ( gpointer )play_button, "media-playback-stop-symbolic" );

		self->loop_picture_timer_id = g_timeout_add( self->interval, ( GSourceFunc )_show_next_picture_wrapper, self );
	}
	else {  // self->loop_state == PICTURE_LOOP_PAUSED
		gtk_button_set_icon_name( ( gpointer )play_button, "media-playback-start-symbolic" );

		g_clear_handle_id( &self->loop_picture_timer_id, g_source_remove );
	}
}

static void app_window_set_background_color( AppWindow* self, const char* color )
{
	g_set_str( &self->bg_color, color );

	if ( g_strcmp0( self->bg_color, "#theme-color" ) == 0 ) {
		gtk_widget_add_css_class( ( gpointer )self, "theme-bg" );
	}
	else {
		gtk_widget_remove_css_class( ( gpointer )self, "theme-bg" );
	}

	gtk_widget_queue_draw( ( gpointer )self );
}

static void _fix_free_leak_widget( GtkWindow* window, gpointer unuse, AppWindow* self )
{
	GtkWidget* focus = gtk_window_get_focus( window );

	guint free_count = 0;
	for ( int i = 0; i < self->focus_list->len; i++ ) {
		GtkWidget* widget = g_ptr_array_index( self->focus_list, i );

		if ( widget && widget != focus && !gtk_widget_get_parent( widget ) && APP_IS_PICTURE_VIEW( widget ) ) {
			/**
			 * 结合 GtkGestureClick 会出现 !object_already_finalized 警告
			 *
			 * 手势绑定的 widget 被销毁
			 *
			 * @see: https://gitlab.gnome.org/GNOME/gtk/-/issues/8059#note_2688665
			 */

			free_count += 1;

			// 延迟释放，不然会会出现鼠标事件相关的问题
			g_idle_add_once( ( GSourceOnceFunc )g_object_unref, widget );
			g_ptr_array_index( self->focus_list, i ) = NULL;
		}
	}

	if ( free_count == self->focus_list->len )
		g_ptr_array_set_size( self->focus_list, 0 );
}

static void _app_window_set_opacity( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	g_return_if_fail( parameters != NULL );

	double opacity = 0;
	g_variant_get( parameters, "d", &opacity );

	opacity = MAX( opacity, 0.1 );

	gtk_widget_set_opacity( widget, opacity );
}

static void _app_window_toggle_ui_visibility( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	self->ui_visible = !self->ui_visible;

	if ( self->ui_visible ) {
		_show_ui( self );
	}
	else {
		_hide_ui( self );
	}
}

static void _clipboard_read_file_list_finish( GObject* source, GAsyncResult* result, gpointer user_data )
{
	GdkClipboard* clipboard = ( gpointer )source;
	AppWindow*    self      = ( gpointer )user_data;

	GError*       error = NULL;
	const GValue* value = gdk_clipboard_read_value_finish( clipboard, result, &error );
	if ( error ) {
		g_warning( "Failed to read clipboard: %s", error->message );
		g_clear_error( &error );
		return;
	}

	if ( !G_VALUE_HOLDS( value, GDK_TYPE_FILE_LIST ) ) {
		return;
	}

	GdkFileList* value_var = g_value_get_boxed( value );

	GSList* file_list = gdk_file_list_get_files( value_var );

	gboolean list_count = app_picture_list_n_items( self->picture_list );

	GPtrArray* file_array = g_ptr_array_new();

	for ( GSList* item = file_list; item; item = g_slist_next( item ) ) {
		GFile*      file = item->data;
		const char* path = g_file_peek_path( file );

		if ( _is_glycin_format( path ) ) {
			g_ptr_array_add( file_array, ( gpointer )path );
		}
	}

	app_picture_list_append_nexts( self->picture_list, ( const char** )file_array->pdata, file_array->len );

	if ( list_count != 0 ) {
		_show_next_picture( self );
	}

	g_slist_free( file_list );
	g_ptr_array_unref( file_array );
}

static void _clipboard_read_texture_finish( GObject* source, GAsyncResult* result, gpointer user_data )
{
	GdkClipboard* clipboard = ( gpointer )source;
	AppWindow*    self      = ( gpointer )user_data;

	GError*     error   = NULL;
	GdkTexture* texture = gdk_clipboard_read_texture_finish( clipboard, result, &error );
	if ( error ) {
		g_warning( "Failed to read clipboard: %s", error->message );
		g_error_free( error );
		return;
	}

	char* temp_path = NULL;

	int fileno = g_file_open_tmp( "clipboard-XXXXXX.png", &temp_path, NULL );

	g_message( "%s", temp_path );

	if ( fileno ) {
		close( fileno );

		if ( gdk_texture_save_to_png( texture, temp_path ) ) {
			_stop_loop( self );

			app_picture_list_append_next( self->picture_list, temp_path );
			if ( app_picture_list_n_items( self->picture_list ) > 1 ) {
				_show_next_picture( self );
			}
		}
	}

	g_free( temp_path );

	g_object_unref( texture );
}

static void _app_window_paste_clipboard( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	gtk_widget_action_set_enabled( ( gpointer )self, "win.paste-clipboard", FALSE );

	GdkClipboard* clipboard = gtk_widget_get_clipboard( widget );

	GdkContentFormats* formats = gdk_clipboard_get_formats( clipboard );

	if ( gdk_content_formats_contain_gtype( formats, GDK_TYPE_FILE_LIST ) ) {
		gdk_clipboard_read_value_async(
		  clipboard, GDK_TYPE_FILE_LIST, G_PRIORITY_DEFAULT, NULL, _clipboard_read_file_list_finish, self );
		return;
	}

	if ( gdk_content_formats_contain_gtype( formats, GDK_TYPE_TEXTURE ) ) {
		gdk_clipboard_read_texture_async( clipboard, NULL, _clipboard_read_texture_finish, self );
		return;
	}
}

static void _app_window_set_background_color( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	g_return_if_fail( parameters != NULL );

	char* rgba = NULL;
	g_variant_get( parameters, "&s", &rgba );

	app_window_set_background_color( self, rgba );
}

static void _app_window_custom_background_finish( GObject* source, GAsyncResult* result, gpointer user_data )
{
	AppWindow* self = user_data;

	GError*  error = NULL;
	GdkRGBA* color = gtk_color_dialog_choose_rgba_finish( ( gpointer )source, result, &error );
	if ( error ) {
		g_message( "%s", error->message );
		g_clear_error( &error );
		return;
	}

	char* color_str = gdk_rgba_to_string( color );
	app_window_set_background_color( self, color_str );

	g_free( color_str );

	gdk_rgba_free( color );
}

static void _app_window_custom_background( GtkWidget* widget, const char* action, GVariant* parameters )
{
	AppWindow* self = ( gpointer )widget;

	GtkColorDialog* dialog = gtk_color_dialog_new();

	gtk_color_dialog_choose_rgba( dialog, ( gpointer )self, NULL, NULL, _app_window_custom_background_finish, self );

	g_object_unref( dialog );
}

static void _on_popover_visible_changed( GtkPopover* popover, gpointer* unuse, gpointer user_data )
{
	AppWindow* self = ( gpointer )gtk_widget_get_root( ( gpointer )popover );

	if ( APP_IS_WINDOW( self ) )
		g_signal_emit( ( gpointer )self, SIGNAL_VISIBLE_POPOVER, 0, popover );
}

static void _on_picture_list_n_items_changed( AppPictureList* list, guint n_itesm, AppWindow* self )
{
	self->can_loop = n_itesm > 1;

	if ( n_itesm > 0 )
		gtk_widget_remove_css_class( ( gpointer )self, "setup" );

	self->prevent_hide_ui = n_itesm == 0;

	gtk_revealer_set_reveal_child( self->loop_controller_revealer, self->can_loop );

	AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( self->picture_view_stack );
	if ( !APP_IS_PICTURE_VIEW( picture_view ) ) {
		_first_load_picture( self );
	}
}

static void app_window_focus_widget_changed( GtkWindow* window, gpointer unuse, AppWindow* self )
{
	{  // fix GtkPopoverMenu bug
	   // @see https://gitlab.gnome.org/GNOME/gtk/-/issues/8063

		GtkWidget* focus = gtk_window_get_focus( window );
		if ( !focus )
			return;

		GtkWidget* popover = ( gpointer )gtk_widget_get_ancestor( focus, GTK_TYPE_POPOVER );
		if ( !popover )
			return;

		if ( !gtk_widget_get_visible( popover ) ) {
			gtk_widget_child_focus( ( gpointer )gtk_widget_get_root( ( focus ) ), GTK_DIR_TAB_FORWARD );
		}
	}
}

static void _load_files( AppWindow* self, GListModel* files )
{
	if ( g_list_model_get_n_items( files ) == 0 )
		return;

	if ( g_list_model_get_n_items( files ) == 1 ) {
		GFile*      file = g_list_model_get_item( files, 0 );
		const char* path = g_file_peek_path( file );

		if ( !g_file_test( path, G_FILE_TEST_EXISTS ) ) {
			g_object_unref( file );
			return;
		}

		GFile* dir = NULL;

		if ( g_file_test( path, G_FILE_TEST_IS_DIR ) ) {
			dir = g_object_ref( file );
		}
		else if ( g_file_test( path, G_FILE_TEST_IS_REGULAR ) ) {
			dir = g_file_get_parent( file );
		}
		else {
			return;
		}

		g_signal_handlers_block_by_func( self->picture_list, G_CALLBACK( _on_picture_list_n_items_changed ), self );

		app_picture_list_load( self->picture_list, g_file_peek_path( dir ) );

		g_signal_handlers_unblock_by_func( self->picture_list, G_CALLBACK( _on_picture_list_n_items_changed ), self );

		PictureLoopMode mode = self->loop_mode;
		app_picture_list_set_mode( self->picture_list, PICTURE_LOOP_SEQUENTIAL );

		for ( guint picture_index = 0;  //
		      picture_index < app_picture_list_n_items( self->picture_list );
		      picture_index++ )  //
		{
			const char* picture_path = app_picture_list_next( self->picture_list );

			if ( g_strcmp0( picture_path, path ) == 0 ) {
				if ( app_picture_list_can_prev( self->picture_list ) ) {
					app_picture_list_prev( self->picture_list );
				}
				else {
					app_picture_list_iter_reset( self->picture_list );
				}
				app_picture_list_truncation( self->picture_list );
				break;
			}
		}

		app_picture_list_set_mode( self->picture_list, mode );

		if ( app_picture_list_n_items( self->picture_list ) > 0 ) {
			_on_picture_list_n_items_changed(
			  self->picture_list, app_picture_list_n_items( self->picture_list ), self );
		}

		g_object_unref( dir );
		g_object_unref( file );
	}
	else {
		for ( int i = 0; i < g_list_model_get_n_items( files ); i++ ) {
			GFile* file = g_list_model_get_item( files, i );

			app_picture_list_append( self->picture_list, g_file_peek_path( file ) );

			g_object_unref( file );
		}
	}
}

static void _show_open_file_dialog_finish( GObject* obj, GAsyncResult* result, gpointer user_data )
{
	AppWindow* self = user_data;

	GListModel* files = gtk_file_dialog_open_multiple_finish( GTK_FILE_DIALOG( obj ), result, NULL );

	if ( files ) {
		_load_files( self, files );
	}

	g_clear_object( &files );
}

static void _show_open_file_dialog( AppWindow* self )
{
	GtkFileFilter* file_filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type( file_filter, "image/*" );

	GtkFileDialog* file_dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_default_filter( file_dialog, file_filter );
	gtk_file_dialog_open_multiple( file_dialog, ( gpointer )self, NULL, _show_open_file_dialog_finish, self );

	g_object_unref( file_dialog );
	g_object_unref( file_filter );
}

static void app_window_first_load( AppWindow* self )
{
	GtkWidget* open_file_button = gtk_button_new_with_mnemonic( C_( "Action", "打开图片(_O)" ) );
	g_signal_connect_swapped( open_file_button, "clicked", G_CALLBACK( _show_open_file_dialog ), self );
	gtk_widget_add_css_class( open_file_button, "entity" );
	gtk_widget_add_css_class( open_file_button, "open-image-file" );
	gtk_widget_set_halign( open_file_button, GTK_ALIGN_CENTER );
	gtk_widget_set_valign( open_file_button, GTK_ALIGN_CENTER );

	gtk_stack_add_child( self->picture_view_stack, open_file_button );
}

static void _on_clipboard_changed( GdkClipboard* clipboard, gpointer user_data )
{
	AppWindow* self = user_data;

	if ( gdk_clipboard_is_local( clipboard ) )
		return;

	gboolean can_paste = FALSE;

	GdkContentFormats* formats = gdk_clipboard_get_formats( clipboard );

	if (                                                                   //
	  gdk_content_formats_contain_gtype( formats, GDK_TYPE_TEXTURE )       //
	  || gdk_content_formats_contain_gtype( formats, GDK_TYPE_FILE_LIST )  //
	) {
		can_paste = TRUE;
	}

	gtk_widget_action_set_enabled( ( gpointer )self, "win.paste-clipboard", can_paste );
}

#endif

#if 1  // base class virtual function

static void app_window_size_allocate( GtkWidget* widget, int width, int height, int baseline )
{
	AppWindow* self = ( AppWindow* )widget;

	GTK_WIDGET_CLASS( app_window_parent_class )->size_allocate( widget, width, height, baseline );

	gtk_widget_size_allocate( ( gpointer )self->entity_revealer, &( GtkAllocation ) { 0, 0, width, height }, baseline );

	AppPictureView* picture_view = ( gpointer )gtk_stack_get_visible_child( self->picture_view_stack );

	if ( APP_IS_PICTURE_VIEW( picture_view ) )
		_on_picture_view_position_changed( picture_view, NULL, self );

	if ( self->window_menu_popover )
		gtk_popover_present( self->window_menu_popover );
}

static void app_window_snapshot( GtkWidget* widget, GtkSnapshot* snapshot )
{
	AppWindow* self = ( gpointer )widget;

	int width  = gtk_widget_get_width( widget );
	int height = gtk_widget_get_height( widget );

	if ( self->bg_color ) {
		GdkRGBA color;
		if ( gdk_rgba_parse( &color, self->bg_color ) ) {
			gtk_snapshot_append_color( snapshot, &color, &GRAPHENE_RECT_INIT( 0, 0, width, height ) );
		}
	}

	GTK_WIDGET_CLASS( app_window_parent_class )->snapshot( widget, snapshot );

	gtk_widget_snapshot_child( widget, ( gpointer )self->entity_revealer, snapshot );
}

// 坏主意，应该使用一个容器作为子控件，而不是自己管理插入的控件。
// 因为，如果子控件时一个复杂的控件。当焦点在其内部走完一圈后，会回到第一个可焦点控件。
// 此时判断 old_focus 和 new_focus 的比对就不可能成立。
// 这里是因为之控件只有一个可焦点控件
static gboolean app_window_focus( GtkWidget* widget, GtkDirectionType direction )
{
	AppWindow* self = ( gpointer )widget;

	gboolean ret = FALSE;

	GtkWidget* old_focus = gtk_window_get_focus( ( gpointer )self );

	ret = GTK_WIDGET_CLASS( app_window_parent_class )->focus( widget, direction );

	GtkWidget* new_focus = gtk_window_get_focus( ( gpointer )self );

	if ( new_focus == old_focus ) {
		ret = gtk_widget_child_focus( ( gpointer )self->entity_revealer, direction );
	}

	return ret;
}

static void ( *_constructed )( GObject* object ) = NULL;

static void constructed( GObject* object )
{
	_constructed( object );

	if ( GTK_IS_POPOVER( object ) ) {
		g_signal_connect( object, "notify::visible", G_CALLBACK( _on_popover_visible_changed ), NULL );
	}
}

static void app_window_set_property( GObject* object, guint property_id, const GValue* value, GParamSpec* pspec )
{
	AppWindow* self = ( gpointer )object;

	switch ( property_id ) {
		case PROP_INTERVAL: {
			self->interval = g_value_get_uint( value );
			g_object_notify_by_pspec( object, pspec );
		} break;
		case PROP_LOOP_MODE: {
			self->loop_mode = g_value_get_enum( value );
			g_object_notify_by_pspec( object, pspec );
		} break;
		case PROP_LOOP_STATE: {
			self->loop_state = g_value_get_enum( value );
			g_object_notify_by_pspec( object, pspec );
		} break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_window_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppWindow* self = ( gpointer )object;

	switch ( property_id ) {
		case PROP_INTERVAL: {
			g_value_set_uint( value, self->interval );
		} break;
		case PROP_LOOP_MODE: {
			g_value_set_enum( value, self->loop_mode );
		} break;
		case PROP_LOOP_STATE: {
			g_value_set_enum( value, self->loop_state );
		} break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_window_constructed( GObject* object )
{
	AppWindow* self = ( AppWindow* )object;

	G_OBJECT_CLASS( app_window_parent_class )->constructed( object );
}

static void app_window_dispose( GObject* object )
{
	AppWindow* self = ( AppWindow* )object;

	GdkClipboard* clipboard = gtk_widget_get_clipboard( ( gpointer )self );

	g_signal_handlers_disconnect_by_func( clipboard, G_CALLBACK( _on_clipboard_changed ), self );

	g_clear_handle_id( &self->loop_picture_timer_id, g_source_remove );
	g_clear_handle_id( &self->hide_ui_timer_id, g_source_remove );

	g_clear_pointer( ( gpointer* )&self->entity_revealer, gtk_widget_unparent );
	g_clear_pointer( ( gpointer* )&self->window_menu_popover, gtk_widget_unparent );

	G_OBJECT_CLASS( app_window_parent_class )->dispose( object );
}

static void app_window_finalize( GObject* object )
{
	AppWindow* self = ( AppWindow* )object;

	g_free( self->bg_color );

	g_object_unref( self->picture_list );

	g_clear_object( &self->main_menu );

	G_OBJECT_CLASS( app_window_parent_class )->finalize( object );
}

static void app_window_init( AppWindow* self )
{
	gtk_widget_action_set_enabled( ( gpointer )self, "picture.zoom-default", FALSE );
	gtk_widget_action_set_enabled( ( gpointer )self, "picture.center", FALSE );
	gtk_widget_action_set_enabled( ( gpointer )self, "picture.zoom", FALSE );
	gtk_widget_action_set_enabled( ( gpointer )self, "win.paste-clipboard", FALSE );

	gtk_widget_add_css_class( ( gpointer )self, "AppWindow" );
	gtk_widget_add_css_class( ( gpointer )self, "setup" );

	gtk_widget_set_size_request( ( gpointer )self, 50, 50 );
	gtk_window_set_default_size( ( gpointer )self, 800, 600 );

	gtk_window_set_title( ( gpointer )self, C_( "AppName", "图窗" ) );

	// g_signal_connect( self, "notify::focus-widget", G_CALLBACK( app_window_print_focus_widget ), self );
	g_signal_connect( self, "notify::focus-widget", G_CALLBACK( app_window_focus_widget_changed ), self );
	g_signal_connect( self, "notify::focus-widget", G_CALLBACK( _fix_free_leak_widget ), self );

	self->can_move_view = TRUE;

	self->loop_mode = PICTURE_LOOP_SEQUENTIAL;

	self->ui_visible = TRUE;

	app_window_set_background_color( self, "#theme-color" );

	self->prevent_hide_ui = TRUE;

	self->focus_list = g_ptr_array_new();

	self->view_navigator = app_view_navigator_new();
	g_signal_connect(
	  self->view_navigator, APP_VIEW_NAVIGATOR_SIGNAL_TO_POSITION, G_CALLBACK( _view_to_position ), self );

	self->view_navigator_container = app_bin_box_new();
	gtk_widget_set_parent( ( gpointer )self->view_navigator, ( gpointer )self->view_navigator_container );

	self->view_navigator_revealer = ( gpointer )gtk_revealer_new();
	gtk_widget_set_visible( ( gpointer )self->view_navigator_revealer, FALSE );
	gtk_widget_set_overflow( ( gpointer )self->view_navigator_revealer, GTK_OVERFLOW_VISIBLE );
	gtk_revealer_set_child( self->view_navigator_revealer, ( gpointer )self->view_navigator_container );
	gtk_revealer_set_transition_type( self->view_navigator_revealer, GTK_REVEALER_TRANSITION_TYPE_CROSSFADE );

	g_object_bind_property(
	  self->view_navigator_revealer, "reveal-child", self->view_navigator_revealer, "visible", G_BINDING_DEFAULT );

	self->picture_view_stack = ( gpointer )gtk_stack_new();
	gtk_stack_set_transition_type( self->picture_view_stack, GTK_STACK_TRANSITION_TYPE_CROSSFADE );
	g_signal_connect(
	  self->picture_view_stack, "notify::visible-child", G_CALLBACK( _on_stack_visible_child_changed ), self );
	g_signal_connect(
	  self->picture_view_stack, "notify::transition-running", G_CALLBACK( _on_stack_transition_running ), self );

	{
		GtkWidget* load_failed = gtk_label_new( _( "🤯 无法加载图片" ) );
		gtk_widget_add_css_class( load_failed, "load-failed" );
		gtk_widget_set_visible( load_failed, FALSE );
		gtk_stack_add_named( self->picture_view_stack, load_failed, "load-failed" );
	}
	gtk_window_set_child( ( gpointer )self, ( gpointer )self->picture_view_stack );

	self->picture_list = app_picture_list_new( self->loop_mode, NULL );

	g_signal_connect(
	  self->picture_list, APP_PICTURE_LIST_SIGNAL_N_ITEMS, G_CALLBACK( _on_picture_list_n_items_changed ), self );

	{
		GtkEventController* motion_controller = ( gpointer )gtk_event_controller_motion_new();
		gtk_event_controller_set_propagation_phase( motion_controller, GTK_PHASE_CAPTURE );
		g_signal_connect( motion_controller, "motion", G_CALLBACK( _on_motion ), self );
		g_signal_connect( motion_controller, "enter", G_CALLBACK( _on_motion_enter ), self );
		g_signal_connect( motion_controller, "leave", G_CALLBACK( _on_motion_leave ), self );
		gtk_widget_add_controller( ( gpointer )self, motion_controller );

		GtkEventController* key_controller = gtk_event_controller_key_new();
		gtk_event_controller_set_propagation_phase( key_controller, GTK_PHASE_CAPTURE );
		g_signal_connect( key_controller, "key-released", G_CALLBACK( _on_key_released ), self );
		gtk_widget_add_controller( ( gpointer )self, key_controller );

		GtkEventController* click_controller = ( gpointer )gtk_gesture_click_new();
		gtk_event_controller_set_propagation_phase( click_controller, GTK_PHASE_CAPTURE );
		gtk_gesture_single_set_button( ( gpointer )click_controller, 0 );
		g_signal_connect( click_controller, "pressed", G_CALLBACK( _on_click_pressed ), self );
		g_signal_connect( click_controller, "released", G_CALLBACK( _on_click_released ), self );
		gtk_widget_add_controller( ( gpointer )self, click_controller );
	}

	self->entity_revealer = ( gpointer )gtk_revealer_new();
	gtk_revealer_set_transition_type( self->entity_revealer, GTK_REVEALER_TRANSITION_TYPE_CROSSFADE );
	gtk_revealer_set_reveal_child( self->entity_revealer, TRUE );

	gtk_widget_insert_after(
	  ( gpointer )self->entity_revealer, ( gpointer )self, ( gpointer )self->picture_view_stack );

	{  // hide default bar
		GtkHeaderBar* bar = _header_bar_new();
		gtk_widget_set_visible( ( gpointer )bar, FALSE );
		gtk_window_set_titlebar( ( gpointer )self, ( gpointer )bar );
	}

	GtkWidget* entify_container = ( gpointer )app_bin_box_new();
	gtk_revealer_set_child( self->entity_revealer, entify_container );
	{
		GtkHeaderBar* bar = _header_bar_new();
		self->header_bar  = ( gpointer )bar;

		gtk_widget_add_css_class( ( gpointer )bar, "entity" );
		gtk_widget_set_parent( ( gpointer )bar, entify_container );
		{
			gtk_widget_set_hexpand( ( gpointer )bar, TRUE );
			gtk_widget_set_vexpand( ( gpointer )bar, FALSE );
			gtk_widget_set_halign( ( gpointer )bar, GTK_ALIGN_FILL );
			gtk_widget_set_valign( ( gpointer )bar, GTK_ALIGN_START );
		}
	}

	self->controller_bar = ( gpointer )app_bin_box_new();
	gtk_widget_add_css_class( ( gpointer )self->controller_bar, "controller_bar" );
	gtk_widget_set_parent( ( gpointer )self->controller_bar, entify_container );
	{
		gtk_widget_set_hexpand( ( gpointer )self->controller_bar, TRUE );
		gtk_widget_set_vexpand( ( gpointer )self->controller_bar, FALSE );
		gtk_widget_set_halign( ( gpointer )self->controller_bar, GTK_ALIGN_FILL );
		gtk_widget_set_valign( ( gpointer )self->controller_bar, GTK_ALIGN_END );
	}

	{
		self->loop_controller_revealer = ( gpointer )gtk_revealer_new();
		gtk_widget_set_parent( ( gpointer )self->loop_controller_revealer, self->controller_bar );
		gtk_revealer_set_transition_type( self->loop_controller_revealer, GTK_REVEALER_TRANSITION_TYPE_CROSSFADE );
		gtk_revealer_set_reveal_child( self->loop_controller_revealer, FALSE );
		{
			gtk_widget_set_hexpand( ( gpointer )self->loop_controller_revealer, FALSE );
			gtk_widget_set_vexpand( ( gpointer )self->loop_controller_revealer, FALSE );
			gtk_widget_set_halign( ( gpointer )self->loop_controller_revealer, GTK_ALIGN_START );
			gtk_widget_set_valign( ( gpointer )self->loop_controller_revealer, GTK_ALIGN_END );
		}

		GtkWidget* box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
		gtk_revealer_set_child( self->loop_controller_revealer, box );
		{  // left & right
			GtkWidget* btn_group = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 6 );
			gtk_widget_add_css_class( btn_group, "button-group" );
			gtk_widget_add_css_class( btn_group, "entity" );
			gtk_widget_set_parent( ( gpointer )btn_group, box );

			GtkWidget* prev_button = self->prev_button = gtk_button_new_from_icon_name( "go-previous-symbolic" );
			gtk_widget_set_sensitive( prev_button, FALSE );
			gtk_widget_set_parent( prev_button, btn_group );
			g_signal_connect_swapped( prev_button, "clicked", G_CALLBACK( _stop_loop ), self );
			g_signal_connect_swapped( prev_button, "clicked", G_CALLBACK( _hide_view_navigator ), self );
			g_signal_connect_swapped( prev_button, "clicked", G_CALLBACK( _show_prev_picture ), self );

			GtkWidget* next_button = self->next_button = gtk_button_new_from_icon_name( "go-next-symbolic" );
			gtk_widget_set_parent( next_button, btn_group );
			g_signal_connect_swapped( next_button, "clicked", G_CALLBACK( _stop_loop ), self );
			g_signal_connect_swapped( next_button, "clicked", G_CALLBACK( _hide_view_navigator ), self );
			g_signal_connect_swapped( next_button, "clicked", G_CALLBACK( _show_next_picture ), self );
		}

		{  // play
			GtkWidget* btn_group = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
			gtk_widget_add_css_class( btn_group, "button-group" );
			gtk_widget_add_css_class( btn_group, "entity" );
			gtk_widget_set_parent( ( gpointer )btn_group, box );

			GtkWidget* play_button = gtk_button_new();
			gtk_widget_set_parent( ( gpointer )play_button, btn_group );
			g_signal_connect_swapped( play_button, "clicked", G_CALLBACK( _do_loop_picture ), self );

			g_signal_connect( self, "notify::loop-state", G_CALLBACK( _on_loop_state_changed ), play_button );
		}

		{  // setting
			GtkWidget* btn_group = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
			gtk_widget_add_css_class( btn_group, "button-group" );
			gtk_widget_set_parent( ( gpointer )btn_group, box );

			GtkWidget* setting_button = gtk_button_new_from_icon_name( "settings-symbolic" );
			gtk_widget_add_css_class( setting_button, "entity" );
			gtk_widget_set_parent( ( gpointer )setting_button, btn_group );

			GtkPopover* setting_popover = ( gpointer )gtk_popover_new();
			g_object_add_toggle_ref(
			  ( gpointer )setting_button, ( GToggleNotify )gtk_widget_unparent, setting_popover );
			gtk_widget_set_parent( ( gpointer )setting_popover, ( gpointer )setting_button );
			gtk_popover_set_position( setting_popover, GTK_POS_TOP );
			g_signal_connect_swapped( setting_button, "clicked", G_CALLBACK( gtk_popover_popup ), setting_popover );
			{
				GtkWidget* popover_content = gtk_grid_new();
				gtk_popover_set_child( setting_popover, popover_content );
				int row_index = 1;
				{
#define _ROW( LABEL )                                                             \
	GtkWidget* box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );                \
	gtk_widget_set_parent( box, popover_content );                                \
                                                                                  \
	GtkWidget* label = gtk_label_new( LABEL );                                    \
	gtk_widget_set_halign( label, GTK_ALIGN_END );                                \
	gtk_grid_attach( ( gpointer )popover_content, label, 0, row_index, 1, 1 );    \
                                                                                  \
	GtkWidget* item_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 2 );           \
	gtk_widget_set_hexpand( item_box, TRUE );                                     \
	gtk_widget_set_halign( item_box, GTK_ALIGN_END );                             \
	gtk_grid_attach( ( gpointer )popover_content, item_box, 1, row_index, 1, 1 ); \
                                                                                  \
	row_index++;

					{  // 间隔
						_ROW( _( "间隔：" ) );

						GtkEntry* entry = ( gpointer )app_entry_new( NULL, NULL, "s" );
						gtk_editable_set_max_width_chars( ( gpointer )entry, 5 );
						gtk_entry_set_input_purpose( entry, GTK_INPUT_PURPOSE_DIGITS );
						gtk_entry_set_placeholder_text( entry, _( "自定义" ) );
						g_signal_connect( entry, "activate", G_CALLBACK( _on_interval_entry_changed ), self );
						gtk_widget_set_parent( ( gpointer )entry, item_box );

						g_signal_connect( self, "notify::interval", G_CALLBACK( _on_interval_changed ), entry );

						AppEntryBufferVerify* verify = app_entry_buffer_verify_new( "^\\d+\\.?\\d?s?$" );
						gtk_entry_set_buffer( entry, ( gpointer )verify );
						g_object_unref( verify );

						GtkWidget* increase_btn = gtk_button_new_from_icon_name( "list-add-symbolic" );
						g_signal_connect_swapped( increase_btn, "clicked", G_CALLBACK( _interval_increase ), self );

						GtkWidget* decrease_btn = gtk_button_new_from_icon_name( "list-remove-symbolic" );
						g_signal_connect_swapped( decrease_btn, "clicked", G_CALLBACK( _interval_decrease ), self );

						gtk_widget_set_parent( ( gpointer )increase_btn, item_box );
						gtk_widget_set_parent( ( gpointer )decrease_btn, item_box );
					}
					{  // 模式
						_ROW( _( "模式：" ) );

						GtkCheckButton* sequential_mode_btn =
						  ( gpointer )gtk_check_button_new_with_label( _( "顺序" ) );
						GtkCheckButton* random_mode_btn = ( gpointer )gtk_check_button_new_with_label( _( "随机" ) );

						g_object_set_data( ( gpointer )sequential_mode_btn,  //
						                   "mode",
						                   GUINT_TO_POINTER( PICTURE_LOOP_SEQUENTIAL ) );
						g_object_set_data( ( gpointer )random_mode_btn,  //
						                   "mode",
						                   GUINT_TO_POINTER( PICTURE_LOOP_RANDOM ) );

						g_signal_connect( sequential_mode_btn, "toggled", G_CALLBACK( _change_mode ), self );
						g_signal_connect( random_mode_btn, "toggled", G_CALLBACK( _change_mode ), self );

						g_signal_connect(
						  self, "notify::loop-mode", G_CALLBACK( _on_mode_changed ), sequential_mode_btn );
						g_signal_connect( self, "notify::loop-mode", G_CALLBACK( _on_mode_changed ), random_mode_btn );

						gtk_check_button_set_group( random_mode_btn, sequential_mode_btn );

						gtk_widget_set_parent( ( gpointer )sequential_mode_btn, item_box );
						gtk_widget_set_parent( ( gpointer )random_mode_btn, item_box );
					}

					{  // 移动图片
						_ROW( _( "移动图片：" ) );

						GtkCheckButton* can_move_view_check = ( gpointer )gtk_check_button_new_with_label( NULL );
						gtk_widget_set_parent( ( gpointer )can_move_view_check, item_box );
						gtk_check_button_set_active( can_move_view_check, self->can_move_view );
						g_signal_connect(
						  can_move_view_check, "toggled", G_CALLBACK( _on_can_move_view_check_toggle ), self );
					}
#undef _ROW
				}
			}
		}
	}

	{  // map
		GtkWidget* btn_group = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
		gtk_widget_add_css_class( btn_group, "button-group" );
		gtk_widget_add_css_class( btn_group, "entity" );
		gtk_widget_set_parent( ( gpointer )btn_group, self->controller_bar );
		{
			gtk_widget_set_hexpand( ( gpointer )btn_group, FALSE );
			gtk_widget_set_vexpand( ( gpointer )btn_group, FALSE );
			gtk_widget_set_halign( ( gpointer )btn_group, GTK_ALIGN_END );
			gtk_widget_set_valign( ( gpointer )btn_group, GTK_ALIGN_END );
		}

		GtkButton* map_btn = self->map_btn = ( gpointer )gtk_button_new_from_icon_name( "map-marker-symbolic" );
		g_signal_connect( map_btn, "clicked", G_CALLBACK( _show_view_navigator ), self );
		gtk_widget_set_parent( ( gpointer )map_btn, btn_group );
		gtk_widget_set_visible( ( gpointer )self->map_btn, FALSE );

		g_object_bind_property( self->map_btn, "visible", btn_group, "visible", G_BINDING_SYNC_CREATE );

		gtk_widget_add_css_class( ( gpointer )self->view_navigator_revealer, "entity" );
		gtk_widget_set_parent( ( gpointer )self->view_navigator_revealer, self->controller_bar );
		{
			gtk_widget_set_hexpand( ( gpointer )self->view_navigator_revealer, FALSE );
			gtk_widget_set_vexpand( ( gpointer )self->view_navigator_revealer, FALSE );
			gtk_widget_set_halign( ( gpointer )self->view_navigator_revealer, GTK_ALIGN_END );
			gtk_widget_set_valign( ( gpointer )self->view_navigator_revealer, GTK_ALIGN_END );
		}

		GtkButton* hide_map_btn = ( gpointer )gtk_button_new_from_icon_name( "window-close-symbolic" );
		gtk_widget_add_css_class( ( gpointer )hide_map_btn, "hide_map_btn" );
		gtk_widget_add_css_class( ( gpointer )hide_map_btn, "circular" );
		g_signal_connect_swapped( hide_map_btn, "clicked", G_CALLBACK( _hide_view_navigator ), self );
		gtk_widget_set_parent( ( gpointer )hide_map_btn, ( gpointer )self->view_navigator_container );
		{
			gtk_widget_set_hexpand( ( gpointer )hide_map_btn, FALSE );
			gtk_widget_set_vexpand( ( gpointer )hide_map_btn, FALSE );
			gtk_widget_set_halign( ( gpointer )hide_map_btn, GTK_ALIGN_END );
			gtk_widget_set_valign( ( gpointer )hide_map_btn, GTK_ALIGN_START );
		}
	}

	GdkClipboard* clipboard = gtk_widget_get_clipboard( ( gpointer )self );
	g_signal_connect( clipboard, "changed", G_CALLBACK( _on_clipboard_changed ), self );

	app_window_first_load( self );
}

static void app_window_class_init( AppWindowClass* klass )
{
	GObjectClass*              base_class   = ( GObjectClass* )klass;
	GtkWidgetClass*            widget_class = ( GtkWidgetClass* )klass;
	GtkApplicationWindowClass* parent_class = ( GtkApplicationWindowClass* )klass;

	base_class->set_property = app_window_set_property;
	base_class->get_property = app_window_get_property;
	base_class->constructed  = app_window_constructed;
	base_class->dispose      = app_window_dispose;
	base_class->finalize     = app_window_finalize;

	widget_class->size_allocate = app_window_size_allocate;
	widget_class->snapshot      = app_window_snapshot;
	widget_class->focus         = app_window_focus;

	specsp[ PROP_INTERVAL ] = g_param_spec_uint( "interval",
	                                             "interval",
	                                             "interval",
	                                             100,
	                                             G_MAXUINT,
	                                             3000,
	                                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT );

	specsp[ PROP_LOOP_MODE ] = g_param_spec_enum( "loop-mode",
	                                              "loop mode",
	                                              "loop mode",
	                                              picture_loop_mode_get_type(),
	                                              PICTURE_LOOP_SEQUENTIAL,
	                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT );

	specsp[ PROP_LOOP_STATE ] = g_param_spec_enum( "loop-state",
	                                               "loop state",
	                                               "loop state",
	                                               picture_loop_state_get_type(),
	                                               PICTURE_LOOP_PAUSED,
	                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT );

	g_object_class_install_properties( base_class, PROP_N, specsp );

	gtk_widget_class_install_action( widget_class, "picture.zoom-default", NULL, _app_window_picture_zoom_default );
	gtk_widget_class_install_action( widget_class, "picture.center", NULL, _app_window_picture_center );
	gtk_widget_class_install_action( widget_class, "picture.zoom", "d", _app_window_picture_zoom );

	gtk_widget_class_install_action( widget_class, "win.background", "s", _app_window_set_background_color );
	gtk_widget_class_install_action( widget_class, "win.background-custom", NULL, _app_window_custom_background );
	gtk_widget_class_install_action( widget_class, "win.opacity", "d", _app_window_set_opacity );
	gtk_widget_class_install_action( widget_class, "win.toggle-ui-visibility", NULL, _app_window_toggle_ui_visibility );
	gtk_widget_class_install_action( widget_class, "win.paste-clipboard", NULL, _app_window_paste_clipboard );

	SIGNAL_VISIBLE_POPOVER = g_signal_new_class_handler( APP_WINDOW_SIGNAL_VISIBLE_POPOVER,
	                                                     G_TYPE_FROM_CLASS( klass ),
	                                                     G_SIGNAL_RUN_LAST,
	                                                     G_CALLBACK( _app_window_on_popover_visible_changed ),
	                                                     NULL,
	                                                     NULL,
	                                                     NULL,
	                                                     G_TYPE_NONE,
	                                                     1,
	                                                     G_TYPE_OBJECT );

	{  // hook
		GtkWidgetClass* _widget_class = g_type_class_peek_static( gtk_widget_get_type() );

		_constructed = G_OBJECT_CLASS( _widget_class )->constructed;

		G_OBJECT_CLASS( _widget_class )->constructed = constructed;
	}
}

#endif

#if 1  // public function

AppWindow* app_window_new( const char** paths )
{
	AppWindow* self = g_object_new( app_window_get_type(), NULL );

	GListStore* files = g_list_store_new( G_TYPE_FILE );

	for ( const char** p = paths; p && *p; p++ ) {
		GFile* file = g_file_new_build_filename( *p, NULL );
		g_list_store_append( files, file );
		g_object_unref( file );
	}

	_load_files( self, ( gpointer )files );

	g_clear_object( &files );

	return self;
}

#endif

#endif
