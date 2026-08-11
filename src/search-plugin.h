#ifndef SEARCH_PLUGIN_H
#define SEARCH_PLUGIN_H

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define SEARCH_PLUGIN_ENTRY_MARGIN 2
#define SEARCH_PLUGIN_ENTRY_WIDTH 200
#define SEARCH_PLUGIN_ENTRY_HEIGHT -1

#define SEARCH_PLUGIN_POPUP_WIDTH 320
#define SEARCH_PLUGIN_POPUP_HEIGHT 240
#define SEARCH_PLUGIN_POPUP_OPACITY 1.0
#define SEARCH_PLUGIN_POPUP_OFFSET 2
#define SEARCH_PLUGIN_POPUP_BORDER_WIDTH 1

#define SEARCH_PLUGIN_TREE_VIEW_WIDTH 220
#define SEARCH_PLUGIN_TREE_VIEW_HEIGHT 120

#define SEARCH_PLUGIN_ICON_SIZE 32
#define SEARCH_PLUGIN_CELL_PADDING_X 7
#define SEARCH_PLUGIN_CELL_PADDING_Y 10

#define SEARCH_PLUGIN_COLUMN_COUNT 4
#define SEARCH_PLUGIN_COLUMN_TITLE 0
#define SEARCH_PLUGIN_COLUMN_APP_INFO 1
#define SEARCH_PLUGIN_COLUMN_PIXBUF 2
#define SEARCH_PLUGIN_COLUMN_COMMAND 3

#define SEARCH_PLUGIN_ENTRY_ICON_NAME "edit-find-symbolic"
#define SEARCH_PLUGIN_PLACEHOLDER_TEXT "Search"
#define SEARCH_PLUGIN_TOOLTIP_TEXT "Type an app name or shell command"
#define SEARCH_PLUGIN_ICON_TOOLTIP_TEXT "Search apps or run command"

typedef struct
{
    XfcePanelPlugin *plugin;
    GtkWidget *search_entry;
    GtkWidget *popup_window;
    GtkWidget *popup_list;
    gint popup_width;
    gint popup_height;
    gint entry_width;
    gdouble popup_opacity;
    gboolean hide_icon;
} SearchPluginData;

void search_plugin_clear_search(SearchPluginData *data);

gboolean search_plugin_handle_panel_click(GtkWidget *widget,
                                          GdkEventButton *event,
                                          SearchPluginData *data);

void search_plugin_handle_focus_out(GtkWidget *widget,
                                    GdkEventFocus *event,
                                    SearchPluginData *data);

void search_plugin_style_search_entry(GtkWidget *entry);

void search_plugin_style_first_row(GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer user_data);

gboolean search_plugin_matches_query(GAppInfo *app_info,
                                     const gchar *query);

GDesktopAppInfo *search_plugin_find_app(const gchar *query);

GdkPixbuf *search_plugin_load_app_icon(GAppInfo *app_info);

void search_plugin_add_result(GtkListStore *store,
                              const gchar *title,
                              GAppInfo *app_info,
                              const gchar *command,
                              GdkPixbuf *pixbuf);

void search_plugin_launch_query(SearchPluginData *data,
                                const gchar *query);

void search_plugin_position_popup(SearchPluginData *data);

void search_plugin_refresh_popup(SearchPluginData *data);

void search_plugin_handle_row_activation(GtkTreeView *tree_view,
                                         GtkTreePath *path,
                                         GtkTreeViewColumn *column,
                                         SearchPluginData *data);

gboolean search_plugin_handle_row_click(GtkWidget *widget,
                                        GdkEventButton *event,
                                        SearchPluginData *data);

gboolean search_plugin_focus_entry_on_click(GtkWidget *widget,
                                             GdkEventButton *event,
                                             SearchPluginData *data);

void search_plugin_run_entry_query(GtkEntry *entry,
                                   SearchPluginData *data);

gboolean search_plugin_handle_entry_key(GtkWidget *widget,
                                         GdkEventKey *event,
                                         SearchPluginData *data);

void search_plugin_handle_entry_change(GtkEditable *editable,
                                       SearchPluginData *data);
     
void search_plugin_free_plugin_data(SearchPluginData *data);

void search_plugin_construct(XfcePanelPlugin *plugin);

#endif
