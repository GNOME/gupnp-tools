#include <config.h>

#include "entry-completion.h"

#include <libgupnp-av/gupnp-av.h>

static void
entry_completion_constructed (GObject *self);

struct _EntryCompletion
{
        GtkEntryCompletion parent_instance;

        GtkListStore *store;
};


G_DEFINE_TYPE (EntryCompletion, entry_completion, GTK_TYPE_ENTRY_COMPLETION)

GtkEntryCompletion *
entry_completion_new (void)
{
        return g_object_new (ENTRY_TYPE_COMPLETION, NULL);
}

static void
entry_completion_finalize (GObject *object)
{
        G_OBJECT_CLASS (entry_completion_parent_class)->finalize (object);
}

static void
entry_completion_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
        switch (prop_id)
        {
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
entry_completion_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static gboolean
entry_completion_on_match_selected (GtkEntryCompletion *self,
                                    GtkTreeModel       *model,
                                    GtkTreeIter        *iter)
{
        char *match = NULL;
        GtkEntry *entry;

        entry = GTK_ENTRY (gtk_entry_completion_get_entry (self));
        gtk_tree_model_get (model, iter, 0, &match, -1);
        
        char *old_text = g_strdup (gtk_entry_get_text (entry));
        char *needle = strrchr (old_text, ' ');
        if (needle != NULL) {
                needle++;
                *needle = '\0';
        }

        char *new_text = g_strconcat (needle == NULL ? ""
                                                     : old_text,
                                      match,
                                      NULL);
        gtk_entry_set_text (entry, new_text);
        gtk_editable_set_position (GTK_EDITABLE (entry), strlen (new_text));
        g_free (new_text);
        g_free (old_text);

        return TRUE;
}

static void
entry_completion_class_init (EntryCompletionClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->constructed = entry_completion_constructed;
        object_class->finalize = entry_completion_finalize;
        object_class->get_property = entry_completion_get_property;
        object_class->set_property = entry_completion_set_property;

        GtkEntryCompletionClass *entry_class = GTK_ENTRY_COMPLETION_CLASS (klass);
        entry_class->match_selected = entry_completion_on_match_selected;
}

static gboolean
entry_completion_on_match (GtkEntryCompletion *self, const char *key, GtkTreeIter *iter, gpointer user_data)
{
        GtkTreeModel *model = gtk_entry_completion_get_model (self);
        char *candidate;
        GtkWidget *entry;
        gboolean retval = FALSE;

        entry  = gtk_entry_completion_get_entry (self);
        gtk_tree_model_get (model, iter, 0, &candidate, -1);

        const char *needle = strrchr (key, ' ');
        if (needle != NULL) {
                if ((needle - key)> gtk_editable_get_position (GTK_EDITABLE (entry))) {
                        goto out;
                }
                needle++;
        } else {
                needle = key;
        }

        if (strlen(needle) == 0) {
                goto out;
        }

        retval = g_str_has_prefix (candidate, needle);

out:
        g_free (candidate);

        return retval;
}

static void
entry_completion_init (EntryCompletion *self)
{
}

static void
entry_completion_constructed (GObject *object)
{
        EntryCompletion *self = ENTRY_COMPLETION (object);

        if (G_OBJECT_CLASS (entry_completion_parent_class)->constructed != NULL) {
                G_OBJECT_CLASS (entry_completion_parent_class)->constructed (G_OBJECT (self));
        }

        gtk_entry_completion_set_match_func (GTK_ENTRY_COMPLETION (self),
                                             entry_completion_on_match,
                                             NULL,
                                             NULL);

        self->store = gtk_list_store_new (1, G_TYPE_STRING);

        gtk_entry_completion_set_model (GTK_ENTRY_COMPLETION (self), GTK_TREE_MODEL (self->store));
        gtk_entry_completion_set_text_column (GTK_ENTRY_COMPLETION (self), 0);
}

static const char *keywords[] = {
        "and",
        "or",
        "contains",
        "doesNotContain",
        "derivedFrom",
        "exists",
        "true",
        "false",
        ">", ">=", "<", "<=", "=", "!=", "*",
        NULL
};

void
entry_completion_set_search_criteria (EntryCompletion *self, char const * const * criteria)
{
        GtkTreeIter iter;
        gtk_list_store_clear (self->store);

        // Prefill ListStore with the search expression keywords
        char const * const *it = keywords;
        while (*it != NULL) {
                gtk_list_store_insert_with_values (self->store,
                                                   &iter,
                                                   -1,
                                                   0,
                                                   *it,
                                                   -1);
                it++;
        }

        // Add the properties supported by the search criteria
        it = criteria;
        while (*it != NULL) {
                gtk_list_store_insert_with_values (self->store,
                                                   &iter,
                                                   -1,
                                                   0,
                                                   *it,
                                                   -1);
                it++;
        }
}
