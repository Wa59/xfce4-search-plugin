#include "search-plugin.h"

void search_plugin_clear_search(SearchPluginData *data)
{
    if (data == NULL || data->search_entry == NULL)
        return;

    gtk_entry_set_text(GTK_ENTRY(data->search_entry), "");
    if (data->popup_window != NULL)
        gtk_widget_hide(data->popup_window);
}

gboolean search_plugin_handle_panel_click(GtkWidget *widget,
                                           GdkEventButton *event,
                                           SearchPluginData *data)
{
    (void) widget;

    if (event == NULL || data == NULL || event->type != GDK_BUTTON_PRESS)
        return FALSE;

    GtkWidget *event_widget = gtk_get_event_widget((GdkEvent *) event);
    gboolean should_clear = TRUE;

    if (event_widget != NULL)
    {
        if (event_widget == data->search_entry || gtk_widget_is_ancestor(event_widget, data->search_entry))
            should_clear = FALSE;
        else if (data->popup_window != NULL && (event_widget == data->popup_window || gtk_widget_is_ancestor(event_widget, data->popup_window)))
            should_clear = FALSE;
    }

    if (should_clear)
        search_plugin_clear_search(data);

    return FALSE;
}

void search_plugin_handle_focus_out(GtkWidget *widget, GdkEventFocus *event, SearchPluginData *data)
{
    (void) widget;
    (void) event;

    if (data == NULL)
        return;

    search_plugin_clear_search(data);
}

void search_plugin_style_search_entry(GtkWidget *entry)
{
    if (entry == NULL)
        return;

    gtk_widget_set_name(entry, "xfce-search-entry");
    gtk_widget_set_margin_top(entry, SEARCH_PLUGIN_ENTRY_MARGIN);
    gtk_widget_set_margin_bottom(entry, SEARCH_PLUGIN_ENTRY_MARGIN);
    gtk_widget_queue_resize(entry);

    /*GtkCssProvider *provider = gtk_css_provider_new();
    gchar *css = g_strdup_printf("#xfce-search-entry { margin-top: %dpx; margin-bottom: %dpx; }",
                                 SEARCH_PLUGIN_ENTRY_MARGIN,
                                 SEARCH_PLUGIN_ENTRY_MARGIN);
    if (gtk_css_provider_load_from_data(provider, css, -1, NULL))
    {
        gtk_style_context_add_provider(gtk_widget_get_style_context(entry),
                                        GTK_STYLE_PROVIDER(provider),
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_free(css);
    g_object_unref(provider);*/
    
}

void search_plugin_style_first_row(GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer user_data)
{
    (void) column;
    (void) user_data;

    GtkTreePath *path = gtk_tree_model_get_path(model, iter);
    gboolean is_first_row = FALSE;

    if (path != NULL)
    {
        gint *indices = gtk_tree_path_get_indices(path);
        is_first_row = (indices != NULL && indices[0] == 0);
        gtk_tree_path_free(path);
    }

    if (is_first_row)
    {
        GdkRGBA background;
        gdk_rgba_parse(&background, "#ffffff44");
        g_object_set(renderer, "cell-background-rgba", &background, NULL);
    }
    else
    {
        g_object_set(renderer, "cell-background-rgba", NULL, NULL);
    }
}

void search_plugin_position_popup(SearchPluginData *data)
{
    if (data == NULL || data->popup_window == NULL || data->plugin == NULL)
        return;

    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(data->plugin));
    if (toplevel == NULL || !GTK_IS_WINDOW(toplevel))
        return;

    GtkAllocation panel_allocation;
    gtk_widget_get_allocation(GTK_WIDGET(data->plugin), &panel_allocation);
    GdkWindow *panel_window = gtk_widget_get_window(GTK_WIDGET(data->plugin));
    if (panel_window == NULL)
        return;

    gint root_x = 0;
    gint root_y = 0;
    gdk_window_get_origin(panel_window, &root_x, &root_y);
    gint popup_width = 0;
    gint popup_height = 0;
    gtk_window_get_size(GTK_WINDOW(data->popup_window), &popup_width, &popup_height);

    gint target_x = root_x + panel_allocation.x;
    gint target_y = root_y + panel_allocation.y + panel_allocation.height + SEARCH_PLUGIN_POPUP_OFFSET;
    gint screen_height = gdk_screen_height();

    if (target_y + popup_height > screen_height)
        target_y = root_y + panel_allocation.y - popup_height - SEARCH_PLUGIN_POPUP_OFFSET;

    gtk_window_move(GTK_WINDOW(data->popup_window), target_x, target_y);
}
