/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Faker Moatamri <faker.moatamri@sophia.inria.fr>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "gtk-config-store.h"

#include "display-functions.h"
#include "raw-text-config.h"

#include "ns3/log.h"

#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GtkconfigStore");

GtkConfigStore::GtkConfigStore()
{
}

void
GtkConfigStore::ConfigureDefaults()
{
    // this function should be called before running the script to enable the user
    // to configure the default values for the objects he wants to use
    GtkWidget* window;
    GtkWidget* view;
    GtkWidget* scroll;

    gtk_init(nullptr, nullptr);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ns-3 Default attributes.");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);

    g_signal_connect(window, "delete_event", (GCallback)delete_event_callback, window);
    GtkTreeStore* model = gtk_tree_store_new(COL_LAST, G_TYPE_POINTER);
    ModelTypeidCreator creator;
    creator.Build(model);

    view = create_view_config_default(model);
    scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scroll), view);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    GtkWidget* save = gtk_button_new_with_label("Save");
    g_signal_connect(save, "clicked", (GCallback)save_clicked_default, window);
    gtk_box_pack_end(GTK_BOX(hbox), save, FALSE, FALSE, 0);
    GtkWidget* load = gtk_button_new_with_label("Load");
    g_signal_connect(load, "clicked", (GCallback)load_clicked_default, window);
    gtk_box_pack_end(GTK_BOX(hbox), load, FALSE, FALSE, 0);
    GtkWidget* exit = gtk_button_new_with_label("Run Simulation");
    g_signal_connect(exit, "clicked", (GCallback)exit_clicked_callback, window);
    gtk_box_pack_end(GTK_BOX(hbox), exit, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);

    gtk_main();

    gtk_tree_model_foreach(GTK_TREE_MODEL(model), clean_model_callback_config_default, nullptr);

    gtk_widget_destroy(window);
}

void
GtkConfigStore::ConfigureAttributes()
{
    GtkWidget* window;
    GtkWidget* view;
    GtkWidget* scroll;

    gtk_init(nullptr, nullptr);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ns-3 Object attributes.");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);

    g_signal_connect(window, "delete_event", (GCallback)delete_event_callback, window);

    GtkTreeStore* model = gtk_tree_store_new(COL_LAST, G_TYPE_POINTER);
    ModelCreator creator;
    creator.Build(model);

    view = create_view(model);
    scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scroll), view);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    GtkWidget* save = gtk_button_new_with_label("Save");
    g_signal_connect(save, "clicked", (GCallback)save_clicked_attribute, window);
    gtk_box_pack_end(GTK_BOX(hbox), save, FALSE, FALSE, 0);
    GtkWidget* load = gtk_button_new_with_label("Load");
    g_signal_connect(load, "clicked", (GCallback)load_clicked_attribute, window);
    gtk_box_pack_end(GTK_BOX(hbox), load, FALSE, FALSE, 0);
    GtkWidget* exit = gtk_button_new_with_label("Run Simulation");
    g_signal_connect(exit, "clicked", (GCallback)exit_clicked_callback, window);
    gtk_box_pack_end(GTK_BOX(hbox), exit, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);

    gtk_main();

    gtk_tree_model_foreach(GTK_TREE_MODEL(model), clean_model_callback, nullptr);

    gtk_widget_destroy(window);
}

} // namespace ns3
