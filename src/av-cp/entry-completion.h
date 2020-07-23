#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ENTRY_TYPE_COMPLETION (entry_completion_get_type())

G_DECLARE_FINAL_TYPE (EntryCompletion, entry_completion, ENTRY, COMPLETION, GtkEntryCompletion)

GtkEntryCompletion *entry_completion_new (void);
void entry_completion_set_search_criteria (EntryCompletion *self, char const * const * criteria);

G_END_DECLS
