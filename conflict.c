#include "conflict.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* attr_conflicts_multiple[] = {
  EVC_EMAIL, EVC_X_MSN, EVC_X_AIM, EVC_X_ICQ, EVC_X_SKYPE, EVC_X_JABBER, EVC_X_GROUPWISE, EVC_X_SIP, EVC_X_YAHOO, EVC_TEL,
  EVC_X_GADUGADU, EVC_X_TELEX, EVC_ADR, EVC_LABEL,
  NULL};

int ecm_conflict_attr_is_unique(EVCardAttribute* attr)
{
  const char* attr_name = e_vcard_attribute_get_name(attr);
  const char** cc_current = attr_conflicts_multiple;
  while (*cc_current != NULL)
  {
    if (strcmp(attr_name, *cc_current) == 0)
    {
      return FALSE;
    }
    cc_current++;
  }
  return TRUE;
}

char* ecm_conflict_attr_value_string(EVCardAttribute* attr)
{
  GList* current_value = e_vcard_attribute_get_values(attr);
  GString* valstr = NULL;
  while (current_value)
  {
    if (valstr == NULL)
    {
      valstr = g_string_new(current_value->data);
    }
    else
    {
      valstr = g_string_append_c(valstr, ',');
      valstr = g_string_append(valstr, current_value->data);
    }
    current_value = g_list_next(current_value);
  }
  return g_string_free(valstr, FALSE);
}

gboolean ecm_conflict_attr_is_equal(EVCardAttribute* attr1, EVCardAttribute* attr2)
{
  // TODO : compare on params in addition to values
  GList* current_value_1 = e_vcard_attribute_get_values(attr1);
  GList* current_value_2 = e_vcard_attribute_get_values(attr2);

  while (current_value_1 && current_value_2)
  {
    if (g_strcmp0(current_value_1->data, current_value_2->data) != 0)
    {
      return FALSE;
    }
    current_value_1 = g_list_next(current_value_1);
    current_value_2 = g_list_next(current_value_2);
  }
  if (current_value_1 || current_value_2)
  {
    return FALSE;
  }
  return TRUE;
}

ECMConflict* ecm_conflict_new()
{
  ECMConflict* c = (ECMConflict*)malloc(sizeof(ECMConflict));
  c->attr_array = g_ptr_array_new();
  c->is_unique = FALSE;
  c->chosen_attr = NULL;
  return c;
}

void ecm_conflict_free(ECMConflict* c)
{
  g_ptr_array_free(c->attr_array, TRUE);
  free(c);
}

GHashTable* ecm_conflict_table_new()
{
  GHashTable* t = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)ecm_conflict_free);
  return t;
}

void ecm_conflict_table_register_attr(GHashTable* conflictsTable, EVCardAttribute* attr)
{
  const char* attr_name = e_vcard_attribute_get_name(attr);
  ECMConflict* c = (ECMConflict*)g_hash_table_lookup(conflictsTable, attr_name);
  if (c == NULL)
  {
    printf("new entry in conflict table : %s\n", attr_name);
    c = ecm_conflict_new();
    c->is_unique = ecm_conflict_attr_is_unique(attr);
    g_hash_table_insert(conflictsTable, (char*)attr_name, c);
  }
  int i;
  for (i = 0; i < c->attr_array->len; ++i)
  {
    if (ecm_conflict_attr_is_equal(attr, g_ptr_array_index(c->attr_array, i)) == TRUE)
    {
      // we already have the exact same attribute ... skip it
      printf("same attribute already in conflict\n");
      return;
    }
  }
  printf("add attribute in conflict\n");
  g_ptr_array_add(c->attr_array, attr);
}

void ecm_conflict_table_merge_in_contact(GHashTable* conflictsTable, EVCard* contact)
{
  GList* current_conflict = g_hash_table_get_values(conflictsTable); // no need to free this one according to the doc
  while (current_conflict)
  {
    ECMConflict* c = current_conflict->data;
    if (c->is_unique == TRUE)
    {
      printf("solving conflict : unique\n");
      // check if chosen_attr is set. if yes, use it, otherwise use the first item
      EVCardAttribute* new_attr = NULL;
      if (c->chosen_attr != NULL)
      {
        new_attr = e_vcard_attribute_copy(c->chosen_attr);
      }
      else
      {
        new_attr = e_vcard_attribute_copy(g_ptr_array_index(c->attr_array, 0));
      }
      e_vcard_add_attribute(contact, new_attr); // contact is now the owner of new_attr, no need to free it
    }
    else
    {
      printf("solving conflict : multiple\n");
      // add all of them
      int i = 0;
      for (i = 0; i < c->attr_array->len; ++i)
      {
        EVCardAttribute* attr_copy = e_vcard_attribute_copy(g_ptr_array_index(c->attr_array, i));
        e_vcard_add_attribute(contact, attr_copy); // contact is now the owner of attr_copy, no need to free it
      }
    }
    current_conflict = g_list_next(current_conflict);
  }
}

