#include <stdio.h>
#include <string.h>

#include <libebook/e-contact.h>
#include <libebook/e-book.h>

#include <shell/e-shell-view.h>

#include "conflict.h"
#include "conflict-dialog.h"

/*
static char* vcard_rev_new()
{
  GTimeVal now;
  g_get_current_time(&now);
  char* now_iso = g_time_val_to_iso8601(&now);
  printf("generated new rev : %s\n", now_iso);
  return now_iso;
}

static char* vcard_uid_new()
{
  char* uid = (char*)malloc(20);
  snprintf(uid, 20, "evo-mc-%i", rand());
  printf("generated new uid : %s\n", uid);
  return uid;
}


static EContact* merged_econtact_new()
{
  EContact* c = e_contact_new();
  e_contact_set(c, E_CONTACT_UID, vcard_uid_new());
  e_contact_set(c, E_CONTACT_REV, vcard_rev_new());
  return c;
}
*/

/*
void ecm_merge_create_contact(GHashTable* conflicts, EABPopupTargetSelect* target)
{
  EContact* mergedContact = merged_econtact_new();
  EVCard* mergedContactVcard = (EVCard*)mergedContact;

  printf("---\nmerging :\n");
  ecm_conflict_table_merge_in_contact(conflicts, mergedContactVcard);

  // add new merged contact

  printf("---\nmerged contact :\n");
  e_vcard_dump_structure(mergedContactVcard);

  GError* error = NULL;
  e_book_add_contact(target->book, mergedContact, &error); // freeing mergedContact afterwards makes evo coredumps ...
  if (error != NULL)
  {
    fprintf(stderr, "error while adding contact : %s\n", error->message);
    g_error_free(error);
  }

  // delete old contacts

  int i;
  for (i = 0; i < target->cards->len; i++)
  {
    EContact *c = g_ptr_array_index(target->cards, i);
    char *id = e_contact_get(c, E_CONTACT_UID);
    GError *error = NULL;
    printf("remove contact id %s\n", id);
    e_book_remove_contact(target->book, id, &error);
    if (error != NULL)
    {
      fprintf(stderr, "an error occured while removing a contact : %s\n", error->message);
      g_error_free(error);
    }
    free(id);
  }
}
*/


//
// ENTRY POINT FOR THE PLUGIN
// as declared in the eplug definition
//

static void ecm_do_merge_cb(GtkAction *action, EShellView *shell_view)
{
  EShellContent* shell_content = e_shell_view_get_shell_content(shell_view);

  unsigned int np;
  GParamSpec** ps = g_object_class_list_properties(G_OBJECT_GET_CLASS(shell_content), &np);
  unsigned int i;
  for (i = 0; i < np; ++i)
  {
    printf("%s %s\n", ps[i]->name, g_type_name(ps[i]->value_type));
  }
  free(ps);
  printf("------------------\n");

  GObject* v;
  g_object_get(shell_content, "current-view", &v, NULL);

  ps = g_object_class_list_properties(G_OBJECT_GET_CLASS(v), &np);
  for (i = 0; i < np; ++i)
  {
    printf("%s %s\n", ps[i]->name, g_type_name(ps[i]->value_type));
  }
  free(ps);
  printf("------------------\n");

  GObject* m;
  g_object_get(v, "model", &m, NULL);

  ps = g_object_class_list_properties(G_OBJECT_GET_CLASS(m), &np);
  for (i = 0; i < np; ++i)
  {
    printf("%s %s\n", ps[i]->name, g_type_name(ps[i]->value_type));
  }
  free(ps);
  printf("------------------\n");

  EBook* book;
  g_object_get(m, "book", &book, NULL);

  g_object_unref(book);
  g_object_unref(m);
  g_object_unref(v);
}

static GtkActionEntry popup_entries[] = {
  {"ecm-do-merge", NULL, "Merge contacts ...", NULL, "Merge selected contacts", G_CALLBACK(ecm_do_merge_cb)}
};

gboolean org_gnome_contacts_merge(GtkUIManager *ui_manager, EShellView *shell_view)
{
  EShellWindow* shell_window = e_shell_view_get_shell_window(shell_view);
  gtk_action_group_add_actions(e_shell_window_get_action_group(shell_window, "contacts"), popup_entries, 1, shell_view);
  return TRUE;
}

/*
void org_gnome_contacts_merge(EPlugin *plugin, EABPopupTargetSelect* target)
{
  printf("number of cards in the selection : %i\n", target->cards->len);

  // build the conflicts table
  GHashTable* conflicts = ecm_conflict_table_new();

  int i;
  for (i = 0; i < target->cards->len; ++i)
  {
    printf("\n---\ncard %i\n", i);
    EContact* card = g_ptr_array_index(target->cards, i);
    EVCard* cardVcard = (EVCard*)card;
    e_vcard_dump_structure(cardVcard);

    GList* current = e_vcard_get_attributes(cardVcard); // no need to free this one according to the doc
    while (current)
    {
      EVCardAttribute* attr = current->data;
      const char* attr_name = e_vcard_attribute_get_name(attr);

      if (strcmp(attr_name, EVC_REV) == 0 || strcmp(attr_name, EVC_UID) == 0 || strcmp(attr_name, EVC_VERSION) == 0)
      {
        // skip those fields because they are freshly generated in the new merged contact
        printf("skip %s\n", attr_name);
        current = g_list_next(current);
        continue;
      }

      // register the attribute in the conflict table
      ecm_conflict_table_register_attr(conflicts, attr);

      current = g_list_next(current);
    }
  }

  // make the user chose in the conflict table

  ECMConflictDialog* dialog = ecm_conflict_dialog_new();
  if (dialog == NULL)
  {
    printf("error creating dialog\n");
    return;
  }

  gboolean is_dialog_needed = ecm_conflict_dialog_populate(dialog, conflicts);

  if (is_dialog_needed == TRUE)
  {
    // wait for user interaction : after that, choices have been made in conflicts table
    unsigned int dialog_result = gtk_dialog_run(dialog->dialog);

    if (dialog_result == 1) // merge button pressed
    {
      printf("user chose merge\n");
      ecm_merge_create_contact(conflicts, target);
    }
    else
    {
      printf("user chose cancel\n");
    }
  }
  else
  {
    printf("bypass conflict dialog, create merged contact directly\n");
    ecm_merge_create_contact(conflicts, target);
  }

  // final cleanup

  ecm_conflict_dialog_free(dialog);
  g_hash_table_destroy(conflicts);
}
*/
