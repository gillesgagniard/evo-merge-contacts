#ifndef CONFLICT_DIALOG_H__
#define CONFLICT_DIALOG_H__

#include <gtk/gtk.h>

typedef struct
{
  GtkBuilder* builder;
  GtkDialog* dialog;
  GtkListStore* store;
  GtkTreeView* view;
  
  GHashTable* conflicts;
} ECMConflictDialog;

ECMConflictDialog* ecm_conflict_dialog_new();
void ecm_conflict_dialog_free(ECMConflictDialog* dialog);

gboolean ecm_conflict_dialog_populate(ECMConflictDialog* dialog, GHashTable* conflicts);

#endif /* CONFLICT_DIALOG_H__ */
