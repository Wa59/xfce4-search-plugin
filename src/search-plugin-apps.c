#include "search-plugin.h"

gboolean search_plugin_matches_query(GAppInfo *app_info, const gchar *query)
{
    if (app_info == NULL || query == NULL || query[0] == '\0')
        return FALSE;

    GDesktopAppInfo *desktop_info = G_DESKTOP_APP_INFO(app_info);
    const gchar *name = g_app_info_get_name(app_info);
    const gchar *generic_name = g_desktop_app_info_get_generic_name(desktop_info);
    const gchar *const *keywords = g_desktop_app_info_get_keywords(desktop_info);
    gchar *lower_name = g_utf8_strdown(name, -1);
    gchar *lower_generic = g_utf8_strdown(generic_name != NULL ? generic_name : "", -1);
    gchar *normalized_query = g_utf8_strdown(query, -1);
    gchar *trimmed_query = g_strstrip(normalized_query);
    gboolean matched = FALSE;

    if (g_strstr_len(lower_name, -1, trimmed_query) != NULL ||
        g_strstr_len(lower_generic, -1, trimmed_query) != NULL)
    {
        matched = TRUE;
    }
    else if (keywords != NULL)
    {
        for (gint i = 0; keywords[i] != NULL; i++)
        {
            gchar *lower_keyword = g_utf8_strdown(keywords[i], -1);
            if (g_strstr_len(lower_keyword, -1, trimmed_query) != NULL)
            {
                matched = TRUE;
                g_free(lower_keyword);
                break;
            }
            g_free(lower_keyword);
        }
    }

    g_free(lower_name);
    g_free(lower_generic);
    g_free(trimmed_query);
    return matched;
}

GDesktopAppInfo *search_plugin_find_app(const gchar *query)
{
    if (query == NULL || query[0] == '\0')
        return NULL;

    GDesktopAppInfo *best_match = NULL;
    GList *app_infos = g_app_info_get_all();
    for (GList *iter = app_infos; iter != NULL; iter = iter->next)
    {
        GAppInfo *app_info = G_APP_INFO(iter->data);
        if (!g_app_info_should_show(app_info) || !G_IS_DESKTOP_APP_INFO(app_info))
            continue;

        if (search_plugin_matches_query(app_info, query))
        {
            best_match = G_DESKTOP_APP_INFO(app_info);
            g_object_ref(best_match);
            break;
        }
    }

    g_list_free_full(app_infos, g_object_unref);
    return best_match;
}

GdkPixbuf *search_plugin_load_app_icon(GAppInfo *app_info)
{
    if (app_info == NULL)
        return NULL;

    GdkPixbuf *pixbuf = NULL;
    GIcon *icon = g_app_info_get_icon(app_info);
    if (icon != NULL)
    {
        GtkIconTheme *theme = gtk_icon_theme_get_default();
        GtkIconInfo *icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, SEARCH_PLUGIN_ICON_SIZE, GTK_ICON_LOOKUP_FORCE_SIZE);
        if (icon_info != NULL)
        {
            pixbuf = gtk_icon_info_load_icon(icon_info, NULL);
            g_object_unref(icon_info);
        }
    }

    if (pixbuf == NULL)
    {
        GtkIconTheme *theme = gtk_icon_theme_get_default();
        pixbuf = gtk_icon_theme_load_icon(theme, "application-x-executable", SEARCH_PLUGIN_ICON_SIZE, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    }

    return pixbuf;
}

void search_plugin_add_result(GtkListStore *store, const gchar *title, GAppInfo *app_info, const gchar *command, GdkPixbuf *pixbuf)
{
    GtkTreeIter iter_row;
    gtk_list_store_append(store, &iter_row);
    gtk_list_store_set(store, &iter_row,
                       SEARCH_PLUGIN_COLUMN_TITLE, title,
                       SEARCH_PLUGIN_COLUMN_APP_INFO, app_info != NULL ? g_object_ref(app_info) : NULL,
                       SEARCH_PLUGIN_COLUMN_PIXBUF, pixbuf,
                       SEARCH_PLUGIN_COLUMN_COMMAND, command != NULL ? g_strdup(command) : NULL,
                       -1);
}

void search_plugin_launch_query(SearchPluginData *data, const gchar *query)
{
    (void) data;

    if (query == NULL || g_strstrip((gchar *) query)[0] == '\0')
        return;

    GDesktopAppInfo *app_info = search_plugin_find_app(query);
    if (app_info != NULL)
    {
        GError *error = NULL;
        if (!g_app_info_launch(G_APP_INFO(app_info), NULL, NULL, &error))
        {
            g_warning("Unable to launch app: %s", error != NULL ? error->message : "unknown error");
            if (error != NULL)
                g_error_free(error);
        }
        g_object_unref(app_info);
        return;
    }

    GError *error = NULL;
    if (g_spawn_command_line_async(query, &error))
    {
        return;
    }

    g_warning("Unable to run command: %s", query);
    if (error != NULL)
    {
        g_error_free(error);
        error = NULL;
    }

    /* Fallback: open a web search for the query using the default browser */
    gchar *escaped = g_uri_escape_string(query, NULL, TRUE);
    gchar *url = g_strdup_printf("https://www.google.com/search?q=%s", escaped);
    gchar *quoted = g_shell_quote(url);
    gchar *cmd = g_strdup_printf("xdg-open %s", quoted);

    if (!g_spawn_command_line_async(cmd, NULL))
        g_warning("Unable to open web search for: %s", query);

    g_free(escaped);
    g_free(url);
    g_free(quoted);
    g_free(cmd);
}

void search_plugin_refresh_popup(SearchPluginData *data)
{
    const gchar *query = gtk_entry_get_text(GTK_ENTRY(data->search_entry));
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->popup_list)));
    gtk_list_store_clear(store);

    if (query == NULL || query[0] == '\0')
    {
        if (data->popup_window != NULL)
            gtk_widget_hide(data->popup_window);
        return;
    }

    GList *app_infos = g_app_info_get_all();
    gboolean found_match = FALSE;

    for (GList *iter = app_infos; iter != NULL; iter = iter->next)
    {
        GAppInfo *app_info = G_APP_INFO(iter->data);
        if (!g_app_info_should_show(app_info) || !G_IS_DESKTOP_APP_INFO(app_info))
            continue;

        if (search_plugin_matches_query(app_info, query))
        {
            const gchar *name = g_app_info_get_name(app_info);
            GdkPixbuf *pixbuf = search_plugin_load_app_icon(app_info);
            search_plugin_add_result(store, name, app_info, NULL, pixbuf);
            if (pixbuf != NULL)
                g_object_unref(pixbuf);
            found_match = TRUE;
        }
    }

    g_list_free_full(app_infos, g_object_unref);

    if (!found_match)
    {
        GtkIconTheme *theme = gtk_icon_theme_get_default();
        GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(theme, "system-run-symbolic", SEARCH_PLUGIN_ICON_SIZE, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
        search_plugin_add_result(store, g_strdup_printf("Run: %s", query), NULL, query, pixbuf);
        if (pixbuf != NULL)
            g_object_unref(pixbuf);
    }

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL) > 0)
    {
        if (data->popup_window != NULL)
        {
            search_plugin_position_popup(data);
            gtk_widget_show_all(data->popup_window);
        }
    }
    else if (data->popup_window != NULL)
    {
        gtk_widget_hide(data->popup_window);
    }
}

void search_plugin_handle_row_activation(GtkTreeView *tree_view,
                                         GtkTreePath *path,
                                         GtkTreeViewColumn *column,
                                         SearchPluginData *data)
{
    (void) tree_view;
    (void) path;
    (void) column;

    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->popup_list));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        GAppInfo *app_info = NULL;
        gchar *command = NULL;
        gtk_tree_model_get(model, &iter,
                           SEARCH_PLUGIN_COLUMN_APP_INFO, &app_info,
                           SEARCH_PLUGIN_COLUMN_COMMAND, &command,
                           -1);
        if (app_info != NULL)
        {
            GError *error = NULL;
            if (!g_app_info_launch(app_info, NULL, NULL, &error))
            {
                g_warning("Unable to launch app: %s", error != NULL ? error->message : "unknown error");
                if (error != NULL)
                    g_error_free(error);
            }
            g_object_unref(app_info);
        }
        else if (command != NULL)
        {
            search_plugin_launch_query(data, command);
        }

        g_free(command);
        gtk_entry_set_text(GTK_ENTRY(data->search_entry), "");
        gtk_widget_hide(data->popup_window);
    }
}

gboolean
search_plugin_handle_row_click(GtkWidget *widget,
                               GdkEventButton *event,
                               SearchPluginData *data)
{
    if (event == NULL || event->type != GDK_BUTTON_PRESS || event->button != GDK_BUTTON_PRIMARY || data == NULL)
        return FALSE;

    GtkTreePath *path = NULL;
    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL))
    {
        search_plugin_handle_row_activation(GTK_TREE_VIEW(widget), path, NULL, data);
        gtk_tree_path_free(path);
        return TRUE;
    }

    return FALSE;
}

gboolean search_plugin_focus_entry_on_click(GtkWidget *widget,
                                             GdkEventButton *event,
                                             SearchPluginData *data)
{
    (void) event;
    if (data->plugin != NULL)
        xfce_panel_plugin_focus_widget(data->plugin, widget);
    return FALSE;
}

void search_plugin_run_entry_query(GtkEntry *entry, SearchPluginData *data)
{
    const gchar *query = gtk_entry_get_text(entry);
    search_plugin_launch_query(data, query);
    gtk_entry_set_text(entry, "");
    gtk_widget_hide(data->popup_window);
}

gboolean search_plugin_handle_entry_key(GtkWidget *widget,
                                         GdkEventKey *event,
                                         SearchPluginData *data)
{
    (void) widget;

    if (event != NULL && event->keyval == GDK_KEY_Escape)
    {
        search_plugin_clear_search(data);
        return TRUE;
    }

    return FALSE;
}

void search_plugin_handle_entry_change(GtkEditable *editable, SearchPluginData *data)
{
    (void) editable;
    search_plugin_refresh_popup(data);
}
