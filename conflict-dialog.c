#include "conflict-dialog.h"
#include "conflict.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
  NAME_COLUMN,
  CHOSEN_STRING_COLUMN,
  CONFLICT_COLUMN,
  LIST_CONFLICT_MODEL,
  N_MODEL_CONFLICTS
} MODEL_CONFLICTS_COLUMNS;

typedef enum
{
  CHOICE_STRING_COLUMN,
  CHOICE_ATTR_COLUMN,
  N_MODEL_CHOICE
} MODEL_CHOICE;

static void ecm_conflict_dialog_choice_changed(GtkCellRendererCombo *combo, gchar *path_string, GtkTreeIter *new_iter, gpointer user_data)
{
  printf("choice changed !\n");

  ECMConflictDialog* dialog = (ECMConflictDialog*)user_data;
  GtkTreeSelection* selection = gtk_tree_view_get_selection(dialog->view);
  GtkTreeIter iter;
  GtkTreeModel* model;
  if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
  {
    printf("strange ... no line is selected ...\n");
    return;
  }
  GtkTreeModel* combo_model = NULL; ECMConflict* conflict = NULL;
  gtk_tree_model_get(model, &iter, LIST_CONFLICT_MODEL, &combo_model, CONFLICT_COLUMN, &conflict, -1);
  char* selected_string = NULL; EVCardAttribute* selected_attr = NULL;
  gtk_tree_model_get(combo_model, new_iter, CHOICE_STRING_COLUMN, &selected_string, CHOICE_ATTR_COLUMN, &selected_attr, -1);
  printf("selected element : %s\n", selected_string);
  gtk_list_store_set((GtkListStore*)model, &iter, CHOSEN_STRING_COLUMN, selected_string, -1);
  free(selected_string);
  conflict->chosen_attr = selected_attr;
}

static void setup_treeview(ECMConflictDialog* dialog)
{
  dialog->store = gtk_list_store_new(N_MODEL_CONFLICTS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, GTK_TYPE_TREE_MODEL);

  gtk_tree_view_set_model(dialog->view, (GtkTreeModel*)dialog->store);

  GtkCellRenderer* renderer;
  GtkTreeViewColumn* column;

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", NAME_COLUMN, NULL);
  gtk_tree_view_append_column(dialog->view, column);

  renderer = gtk_cell_renderer_combo_new();
  g_object_set(renderer, "text-column", CHOICE_STRING_COLUMN, "editable", TRUE, "has-entry", FALSE, NULL);
  column = gtk_tree_view_column_new_with_attributes("Choice", renderer, "text", CHOSEN_STRING_COLUMN, "model", LIST_CONFLICT_MODEL, NULL);
  gtk_tree_view_append_column(dialog->view, column);
  g_signal_connect(renderer, "changed", G_CALLBACK(ecm_conflict_dialog_choice_changed), (void*)dialog);

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(dialog->view), GTK_SELECTION_SINGLE);
}

ECMConflictDialog* ecm_conflict_dialog_new()
{
  ECMConflictDialog* d = (ECMConflictDialog*)malloc(sizeof(ECMConflictDialog));
  d->builder = gtk_builder_new();
  char* filename = g_strconcat(PLUGIN_PATH, "/org-gnome-contacts-merge.ui", NULL);
  printf("loading ui %s\n", filename);
  GError *error = NULL;
  unsigned int ret = gtk_builder_add_from_file(d->builder, filename, &error);
  g_free(filename);
  if (ret == 0)
  {
    printf("unable to load ui : %s\n", error->message);
    g_error_free(error);
    ecm_conflict_dialog_free(d);
    return NULL;
  }

  d->dialog = (GtkDialog*)gtk_builder_get_object(d->builder, "dialog_conflicts");
  d->view = (GtkTreeView*)gtk_builder_get_object(d->builder, "treeview_conflicts");
  if (d->dialog == NULL || d->view == NULL)
  {
    printf("unable to find ids in ui file\n");
    ecm_conflict_dialog_free(d);
    return NULL;
  }

  setup_treeview(d);

  gtk_builder_connect_signals(d->builder, d);
  gtk_widget_show_all((GtkWidget*)d->dialog);

  d->conflicts = NULL;

  return d;
}

void ecm_conflict_dialog_free(ECMConflictDialog* dialog)
{
  if (dialog->dialog != NULL)
  {
    gtk_widget_destroy((GtkWidget*)dialog->dialog);
  }
  if (dialog->builder != NULL)
  {
    g_object_unref((GObject*)dialog->builder);
  }
  if (dialog->store != NULL)
  {
    g_object_unref((GObject*)dialog->store);
  }
  free(dialog);
}

gboolean ecm_conflict_dialog_populate(ECMConflictDialog* dialog, GHashTable* conflicts)
{
  unsigned int nb_conflicts = 0;

  dialog->conflicts = conflicts;

  GHashTableIter iter;
  char* key_attr_name; ECMConflict* value_conflict;
  g_hash_table_iter_init(&iter, dialog->conflicts);
  while (g_hash_table_iter_next(&iter, (void**)&key_attr_name, (void**)&value_conflict))
  {
    if (value_conflict->is_unique == TRUE && value_conflict->attr_array->len > 1)
    {
      nb_conflicts++;

      GtkListStore* list_values = gtk_list_store_new(N_MODEL_CHOICE, G_TYPE_STRING, G_TYPE_POINTER);
      int i;
      for (i = 0; i < value_conflict->attr_array->len; ++i)
      {
        GtkTreeIter storeIter;
        gtk_list_store_append(list_values, &storeIter);
        char* s = ecm_conflict_attr_value_string(g_ptr_array_index(value_conflict->attr_array, i));
        gtk_list_store_set(list_values, &storeIter, CHOICE_STRING_COLUMN, s, CHOICE_ATTR_COLUMN, g_ptr_array_index(value_conflict->attr_array, i), -1);
        free(s);
      }

      GtkTreeIter storeIter;
      gtk_list_store_append(dialog->store, &storeIter);
      EContactField ecf = e_contact_field_id_from_vcard(key_attr_name);
      const char* pretty_name = e_contact_pretty_name(ecf);
      if (strlen(pretty_name) < 1)
      {
        pretty_name = key_attr_name;
      }
      gtk_list_store_set(dialog->store, &storeIter, NAME_COLUMN, pretty_name, CHOSEN_STRING_COLUMN, "(default)", CONFLICT_COLUMN, value_conflict, LIST_CONFLICT_MODEL, list_values, -1);

      g_object_unref(list_values);
    }
  }

  if (nb_conflicts > 0)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

