#ifndef CONFLICT_H_
#define CONFLICT_H_

#include <libebook/e-contact.h>

typedef struct
{
  GPtrArray* attr_array;
  int is_unique;
  EVCardAttribute* chosen_attr;
} ECMConflict;

int ecm_conflict_attr_is_unique(EVCardAttribute* attr);
char* ecm_conflict_attr_value_string(EVCardAttribute* attr);
gboolean ecm_conflict_attr_is_equal(EVCardAttribute* attr1, EVCardAttribute* attr2);

ECMConflict* ecm_conflict_new();
void ecm_conflict_free(ECMConflict* c);

GHashTable* ecm_conflict_table_new();
void ecm_conflict_table_register_attr(GHashTable* conflictsTable, EVCardAttribute* attr);

void ecm_conflict_table_merge_in_contact(GHashTable* conflictsTable, EVCard* contact);

#endif /* CONFLICT_H_ */

