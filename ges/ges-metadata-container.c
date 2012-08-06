#include <glib-object.h>
#include <gst/gst.h>

#include "ges-metadata-container.h"

/**
* SECTION: ges-metadata-container
* @short_description: An interface for storing metadata
*
* FIXME: Long description needed
*/

static GQuark ges_taglist_key;

typedef struct
{
  GstTagMergeMode mode;
  GstTagList *list;
  GMutex lock;
} GESMetadata;

#define GES_METADATA_LOCK(data) g_mutex_lock(&data->lock)
#define GES_METADATA_UNLOCK(data) g_mutex_unlock(&data->lock)

G_DEFINE_INTERFACE_WITH_CODE (GESMetadataContainer, ges_metadata_container,
    G_TYPE_OBJECT, ges_taglist_key =
    g_quark_from_static_string ("ges-metadata-container-data");
    );

static void
ges_metadata_container_default_init (GESMetadataContainerInterface * iface)
{

}

static void
ges_metadata_free (gpointer p)
{
  GESMetadata *data = (GESMetadata *) p;

  if (data->list)
    gst_tag_list_unref (data->list);

  g_mutex_clear (&data->lock);

  g_slice_free (GESMetadata, data);
}

static GESMetadata *
ges_metadata_container_get_data (GESMetadataContainer * container)
{
  GESMetadata *data;

  data = g_object_get_qdata (G_OBJECT (container), ges_taglist_key);
  if (!data) {
    /* make sure no other thread is creating a GstTagData at the same time */
    static GMutex create_mutex; /* no initialisation required */
    g_mutex_lock (&create_mutex);

    data = g_object_get_qdata (G_OBJECT (container), ges_taglist_key);
    if (!data) {
      data = g_slice_new (GESMetadata);
      g_mutex_init (&data->lock);
      data->list = NULL;
      data->mode = GST_TAG_MERGE_KEEP;
      g_object_set_qdata_full (G_OBJECT (container), ges_taglist_key, data,
          ges_metadata_free);
    }

    g_mutex_unlock (&create_mutex);
  }

  return data;
}

/**
 * ges_metadata_container_set_value
 * @container: Target container
 * @metadata_item: Name of the metadata item to set
 * @value: Value to set
 * Sets the value of a given metadata item
 */
void
ges_metadata_container_set_value (GESMetadataContainer * container,
    const gchar * metadata_item, const GValue * value)
{
  return;
}

/**
 * ges_metadata_container_set_char
 * @container: Target container
 * @metadata_item: Name of the metadata item to set
 * @value: Value to set
 * Sets the value of a given metadata item
 */
void
ges_metadata_container_set_char (GESMetadataContainer * container,
    const gchar * metadata_item, gchar value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  if (!data->list)
    data->list = gst_tag_list_new_empty ();

  ges_metadata_register (metadata_item, G_TYPE_CHAR);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);

  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_uchar (GESMetadataContainer * container,
    const gchar * metadata_item, guchar value)
{

}

void
ges_metadata_container_set_int (GESMetadataContainer * container,
    const gchar * metadata_item, gint value)
{

}

void
ges_metadata_container_set_uint (GESMetadataContainer * container,
    const gchar * metadata_item, guint value)
{

}

void
ges_metadata_container_set_int64 (GESMetadataContainer * container,
    const gchar * metadata_item, gint64 value)
{

}

void
ges_metadata_container_set_uint64 (GESMetadataContainer * container,
    const gchar * metadata_item, guint64 value)
{

}

void
ges_metadata_container_set_long (GESMetadataContainer * container,
    const gchar * metadata_item, glong value)
{

}

void
ges_metadata_container_set_ulong (GESMetadataContainer * container,
    const gchar * metadata_item, gulong value)
{

}

void
ges_metadata_container_set_float (GESMetadataContainer * container,
    const gchar * metadata_item, gfloat value)
{

}

void
ges_metadata_container_set_double (GESMetadataContainer * container,
    const gchar * metadata_item, gdouble value)
{

}

void
ges_metadata_container_set_date (GESMetadataContainer * container,
    const gchar * metadata_item, const GDate * value)
{

}

void
ges_metadata_container_set_date_time (GESMetadataContainer * container,
    const gchar * metadata_item, const GstDateTime * value)
{

}

void
ges_metadata_container_set_string (GESMetadataContainer * container,
    const gchar * metadata_item, const gchar * value)
{

}

/**
 * ges_metadata_container_get_value
 * @container: Target container
 * @metadata_item: Name of the metadata item to get
 * @dest: Destination to which value of metadata item will be copied
 * Gets the value of a given metadata item
 *
 * Returns: TRUE if value was present in object metadata, FALSE if there is no
 * such metadata item
 */
gboolean
ges_metadata_container_get_value (GESMetadataContainer * container,
    const gchar * metadata_item, GValue ** dest)
{
  return FALSE;
}


gboolean
ges_metadata_container_get_char (GESMetadataContainer * container,
    const gchar * metadata_item, gchar * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_uchar (GESMetadataContainer * container,
    const gchar * metadata_item, guchar * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_int (GESMetadataContainer * container,
    const gchar * metadata_item, gint * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_uint (GESMetadataContainer * container,
    const gchar * metadata_item, guint * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_int64 (GESMetadataContainer * container,
    const gchar * metadata_item, gint64 * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_uint64 (GESMetadataContainer * container,
    const gchar * metadata_item, guint64 * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_long (GESMetadataContainer * container,
    const gchar * metadata_item, glong * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_ulong (GESMetadataContainer * container,
    const gchar * metadata_item, gulong * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_float (GESMetadataContainer * container,
    const gchar * metadata_item, gfloat * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_double (GESMetadataContainer * container,
    const gchar * metadata_item, gdouble * dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_date (GESMetadataContainer * container,
    const gchar * metadata_item, GDate ** dest)
{
  return FALSE;
}

gboolean
ges_metadata_container_get_date_time (GESMetadataContainer * container,
    const gchar * metadata_item, GstDateTime ** dest)
{
  return FALSE;
}

void
ges_metadata_container_foreach (GESMetadataContainer * container,
    GESMetadataContainerForeachFunc func, gpointer user_data)
{

}

gchar *
ges_metadata_container_to_string (GESMetadataContainer * container)
{
  return NULL;
}

/**
 * ges_metadata_container_new_from_string:
 * @str: a string created with ges_metadata_container_to_string()
 *
 * Deserializes a metadata container.
 *
 * Returns: a new #GESMetadataContainer, or NULL in case of an error.
 */
GESMetadataContainer *
ges_metadata_container_new_from_string (const gchar * str)
{
  return NULL;
}

void
ges_metadata_register (const gchar * name, GType type)
{
  gst_tag_register (name, GST_TAG_FLAG_META, type, name, name, NULL);
}
