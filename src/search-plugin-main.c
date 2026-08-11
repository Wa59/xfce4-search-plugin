#include "search-plugin.h"

#include <libxfce4panel/xfce-panel-macros.h>
#include <xfconf/xfconf.h>

static void search_plugin_apply_settings(SearchPluginData *data);
static void search_plugin_load_settings(SearchPluginData *data);
static void search_plugin_save_settings(SearchPluginData *data);
static void search_plugin_show_options_dialog(XfcePanelPlugin *plugin, gpointer user_data);
static void search_plugin_show_about_dialog(XfcePanelPlugin *plugin, gpointer user_data);
static void search_plugin_options_response(GtkDialog *dialog, gint response_id, gpointer user_data);

void search_plugin_free_plugin_data(SearchPluginData *data)
{
    if (data == NULL)
        return;

    g_free(data);
}

void search_plugin_construct(XfcePanelPlugin *plugin)
{
    SearchPluginData *data = g_new0(SearchPluginData, 1);
    GtkWidget *search_entry;
    GtkWidget *popup_window;
    GtkWidget *scrolled_window;
    GtkWidget *tree_view;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    data->plugin = plugin;
    data->popup_width = SEARCH_PLUGIN_POPUP_WIDTH;
    data->popup_height = SEARCH_PLUGIN_POPUP_HEIGHT;
    data->entry_width = SEARCH_PLUGIN_ENTRY_WIDTH;
    data->popup_opacity = SEARCH_PLUGIN_POPUP_OPACITY;
    data->hide_icon = FALSE;

    search_plugin_load_settings(data);

    search_entry = gtk_entry_new();
    gtk_widget_set_name(search_entry, "xfce-search-entry");
    gtk_widget_add_events(search_entry, GDK_BUTTON_PRESS_MASK);
    gtk_entry_set_has_frame(GTK_ENTRY(search_entry), TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), SEARCH_PLUGIN_PLACEHOLDER_TEXT);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(search_entry), GTK_ENTRY_ICON_PRIMARY, SEARCH_PLUGIN_ENTRY_ICON_NAME);
    gtk_entry_set_icon_tooltip_text(GTK_ENTRY(search_entry), GTK_ENTRY_ICON_PRIMARY, SEARCH_PLUGIN_ICON_TOOLTIP_TEXT);
    gtk_widget_set_tooltip_text(search_entry, SEARCH_PLUGIN_TOOLTIP_TEXT);
    gtk_widget_set_can_focus(search_entry, TRUE);
    gtk_widget_set_receives_default(search_entry, TRUE);
    gtk_widget_set_focus_on_click(search_entry, TRUE);
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_widget_set_size_request(search_entry, data->entry_width, SEARCH_PLUGIN_ENTRY_HEIGHT);
    gtk_container_add(GTK_CONTAINER(plugin), search_entry);
    data->search_entry = search_entry;

    popup_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_type_hint(GTK_WINDOW(popup_window), GDK_WINDOW_TYPE_HINT_COMBO);
    gtk_window_set_transient_for(GTK_WINDOW(popup_window), GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))));
    gtk_window_set_resizable(GTK_WINDOW(popup_window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(popup_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(popup_window), data->popup_width, data->popup_height);
    gtk_widget_set_opacity(popup_window, data->popup_opacity);
    gtk_container_set_border_width(GTK_CONTAINER(popup_window), SEARCH_PLUGIN_POPUP_BORDER_WIDTH);
    gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_NONE);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(popup_window), scrolled_window);

    store = gtk_list_store_new(SEARCH_PLUGIN_COLUMN_COUNT, G_TYPE_STRING, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

    renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer, "xpad", SEARCH_PLUGIN_CELL_PADDING_X, "ypad", SEARCH_PLUGIN_CELL_PADDING_Y, NULL);
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, "Icon");
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", SEARCH_PLUGIN_COLUMN_PIXBUF);
    gtk_tree_view_column_set_cell_data_func(column, renderer, search_plugin_style_first_row, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xpad", SEARCH_PLUGIN_CELL_PADDING_X, "ypad", SEARCH_PLUGIN_CELL_PADDING_Y, NULL);
    column = gtk_tree_view_column_new_with_attributes("Result", renderer, "text", SEARCH_PLUGIN_COLUMN_TITLE, NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer, search_plugin_style_first_row, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    data->popup_list = tree_view;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

    gtk_widget_set_size_request(tree_view, SEARCH_PLUGIN_TREE_VIEW_WIDTH, SEARCH_PLUGIN_TREE_VIEW_HEIGHT);
    gtk_widget_hide(popup_window);
    gtk_widget_set_name(popup_window, "xfce-search-popup");
    gtk_widget_add_events(popup_window, GDK_BUTTON_PRESS_MASK);

    data->popup_window = popup_window;
    search_plugin_style_search_entry(search_entry);
    search_plugin_apply_settings(data);

    gtk_widget_add_events(GTK_WIDGET(plugin), GDK_BUTTON_PRESS_MASK);
    xfce_panel_plugin_add_action_widget(plugin, search_entry);

    g_signal_connect(search_entry, "button-press-event", G_CALLBACK(search_plugin_focus_entry_on_click), data);
    g_signal_connect(search_entry, "activate", G_CALLBACK(search_plugin_run_entry_query), data);
    g_signal_connect(search_entry, "changed", G_CALLBACK(search_plugin_handle_entry_change), data);
    g_signal_connect(search_entry, "focus-out-event", G_CALLBACK(search_plugin_handle_focus_out), data);
    g_signal_connect(search_entry, "key-press-event", G_CALLBACK(search_plugin_handle_entry_key), data);
    g_signal_connect(tree_view, "button-press-event", G_CALLBACK(search_plugin_handle_row_click), data);
    g_signal_connect(tree_view, "row-activated", G_CALLBACK(search_plugin_handle_row_activation), data);
    g_signal_connect_swapped(plugin, "destroy", G_CALLBACK(search_plugin_free_plugin_data), data);
    g_signal_connect(plugin, "configure-plugin", G_CALLBACK(search_plugin_show_options_dialog), data);
    g_signal_connect(plugin, "about", G_CALLBACK(search_plugin_show_about_dialog), data);

    xfce_panel_plugin_menu_show_configure(plugin);
    xfce_panel_plugin_menu_show_about(plugin);

    g_signal_connect_after(GTK_WIDGET(plugin), "button-press-event", G_CALLBACK(search_plugin_handle_panel_click), data);

    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(plugin));
    if (toplevel != NULL && toplevel != GTK_WIDGET(plugin))
    {
        gtk_widget_add_events(toplevel, GDK_BUTTON_PRESS_MASK);
        g_signal_connect_after(toplevel, "button-press-event", G_CALLBACK(search_plugin_handle_panel_click), data);
    }

    g_signal_connect_after(popup_window, "button-press-event", G_CALLBACK(search_plugin_handle_panel_click), data);

    gtk_widget_show_all(GTK_WIDGET(plugin));
    xfce_panel_plugin_focus_widget(plugin, search_entry);
}

static void
search_plugin_apply_settings(SearchPluginData *data)
{
    if (data == NULL)
        return;

    gtk_widget_set_size_request(data->search_entry, data->entry_width, SEARCH_PLUGIN_ENTRY_HEIGHT);
    gtk_window_resize(GTK_WINDOW(data->popup_window), data->popup_width, data->popup_height);
    gtk_widget_set_opacity(data->popup_window, data->popup_opacity);

    if (data->hide_icon)
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(data->search_entry), GTK_ENTRY_ICON_PRIMARY, NULL);
    else
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(data->search_entry), GTK_ENTRY_ICON_PRIMARY, SEARCH_PLUGIN_ENTRY_ICON_NAME);
}

static void
search_plugin_options_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    SearchPluginData *data = user_data;
    if (data == NULL)
        return;

    if (response_id == GTK_RESPONSE_CANCEL || response_id == GTK_RESPONSE_DELETE_EVENT)
    {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        return;
    }

    GtkWidget *grid = g_object_get_data(G_OBJECT(dialog), "options-grid");
    if (grid != NULL)
    {
        GtkSpinButton *popup_width_button = g_object_get_data(G_OBJECT(dialog), "popup-width");
        GtkSpinButton *popup_height_button = g_object_get_data(G_OBJECT(dialog), "popup-height");
        GtkSpinButton *entry_width_button = g_object_get_data(G_OBJECT(dialog), "entry-width");
        GtkSpinButton *opacity_button = g_object_get_data(G_OBJECT(dialog), "popup-opacity");
        GtkToggleButton *hide_icon_button = GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "hide-icon"));

        if (popup_width_button != NULL)
            data->popup_width = gtk_spin_button_get_value_as_int(popup_width_button);
        if (popup_height_button != NULL)
            data->popup_height = gtk_spin_button_get_value_as_int(popup_height_button);
        if (entry_width_button != NULL)
            data->entry_width = gtk_spin_button_get_value_as_int(entry_width_button);
        if (opacity_button != NULL)
            data->popup_opacity = gtk_spin_button_get_value(opacity_button);
        if (hide_icon_button != NULL)
            data->hide_icon = gtk_toggle_button_get_active(hide_icon_button);

        search_plugin_apply_settings(data);
        search_plugin_save_settings(data);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
search_plugin_show_options_dialog(XfcePanelPlugin *plugin, gpointer user_data)
{
    SearchPluginData *data = user_data;
    if (data == NULL)
        return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Search popup options",
                                                   GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                   _("_OK"), GTK_RESPONSE_OK,
                                                   NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    GtkWidget *popup_width_label = gtk_label_new("Popup width:");
    GtkWidget *popup_width_spin = gtk_spin_button_new_with_range(100, 800, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(popup_width_spin), data->popup_width);
    gtk_widget_set_hexpand(popup_width_spin, TRUE);

    GtkWidget *popup_height_label = gtk_label_new("Popup height:");
    GtkWidget *popup_height_spin = gtk_spin_button_new_with_range(100, 800, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(popup_height_spin), data->popup_height);
    gtk_widget_set_hexpand(popup_height_spin, TRUE);

    GtkWidget *entry_width_label = gtk_label_new("Entry width:");
    GtkWidget *entry_width_spin = gtk_spin_button_new_with_range(120, 420, 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_width_spin), data->entry_width);
    gtk_widget_set_hexpand(entry_width_spin, TRUE);

    GtkWidget *opacity_label = gtk_label_new("Popup transparency:");
    GtkWidget *opacity_spin = gtk_spin_button_new_with_range(0.25, 1.0, 0.05);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(opacity_spin), 2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(opacity_spin), data->popup_opacity);
    gtk_widget_set_hexpand(opacity_spin, TRUE);

    GtkWidget *hide_icon_check = gtk_check_button_new_with_label("Hide entry icon");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_icon_check), data->hide_icon);

    gtk_grid_attach(GTK_GRID(grid), popup_width_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), popup_width_spin, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), popup_height_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), popup_height_spin, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_width_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_width_spin, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), opacity_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), opacity_spin, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hide_icon_check, 0, 4, 2, 1);

    g_object_set_data(G_OBJECT(dialog), "popup-width", popup_width_spin);
    g_object_set_data(G_OBJECT(dialog), "popup-height", popup_height_spin);
    g_object_set_data(G_OBJECT(dialog), "entry-width", entry_width_spin);
    g_object_set_data(G_OBJECT(dialog), "popup-opacity", opacity_spin);
    g_object_set_data(G_OBJECT(dialog), "hide-icon", hide_icon_check);
    g_object_set_data(G_OBJECT(dialog), "options-grid", grid);

    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(search_plugin_options_response), data);
}

static void
search_plugin_show_about_dialog(XfcePanelPlugin *plugin, gpointer user_data)
{
    (void) user_data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(plugin));
    gtk_show_about_dialog(GTK_WINDOW(toplevel),
                          "program-name", "xfce4-search-plugin",
                          "version", "0.1.0",
                          "comments", "XFCE4 search popup plugin",
                          "website", "https://example.com",
                          "authors", (const gchar *[]) { "xfce4-search-plugin", NULL },
                          NULL);
}

static void
search_plugin_load_settings(SearchPluginData *data)
{
    if (data == NULL || data->plugin == NULL)
        return;

    XfconfChannel *channel = xfce_panel_plugin_xfconf_channel_new(data->plugin);
    if (channel == NULL)
        return;

    data->popup_width = xfconf_channel_get_int(channel, "popup-width", data->popup_width);
    data->popup_height = xfconf_channel_get_int(channel, "popup-height", data->popup_height);
    data->entry_width = xfconf_channel_get_int(channel, "entry-width", data->entry_width);
    data->popup_opacity = xfconf_channel_get_double(channel, "popup-opacity", data->popup_opacity);
    data->hide_icon = xfconf_channel_get_bool(channel, "hide-icon", data->hide_icon);
}

static void
search_plugin_save_settings(SearchPluginData *data)
{
    if (data == NULL || data->plugin == NULL)
        return;

    XfconfChannel *channel = xfce_panel_plugin_xfconf_channel_new(data->plugin);
    if (channel == NULL)
        return;

    xfconf_channel_set_int(channel, "popup-width", data->popup_width);
    xfconf_channel_set_int(channel, "popup-height", data->popup_height);
    xfconf_channel_set_int(channel, "entry-width", data->entry_width);
    xfconf_channel_set_double(channel, "popup-opacity", data->popup_opacity);
    xfconf_channel_set_bool(channel, "hide-icon", data->hide_icon);
}

XFCE_PANEL_PLUGIN_REGISTER(search_plugin_construct);
