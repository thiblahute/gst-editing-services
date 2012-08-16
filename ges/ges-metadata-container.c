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
      data->list = gst_tag_list_new_empty ();
      data->mode = GST_TAG_MERGE_KEEP;
      g_object_set_qdata_full (G_OBJECT (container), ges_taglist_key, data,
          ges_metadata_free);
    }

    g_mutex_unlock (&create_mutex);
  }

  return data;
}

void
ges_metadata_container_set_boolean (GESMetadataContainer * container,
    const gchar * metadata_item, gboolean value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_BOOLEAN);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_int (GESMetadataContainer * container,
    const gchar * metadata_item, gint value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_INT);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_uint (GESMetadataContainer * container,
    const gchar * metadata_item, guint value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_UINT);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_int64 (GESMetadataContainer * container,
    const gchar * metadata_item, gint64 value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_INT64);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_uint64 (GESMetadataContainer * container,
    const gchar * metadata_item, guint64 value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_UINT64);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_float (GESMetadataContainer * container,
    const gchar * metadata_item, gfloat value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_FLOAT);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_set_double (GESMetadataContainer * container,
    const gchar * metadata_item, gdouble value)
{
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_DOUBLE);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
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
  GESMetadata *data;

  g_return_if_fail (GES_IS_METADATA_CONTAINER (container));
  g_return_if_fail (metadata_item != NULL);

  data = ges_metadata_container_get_data (container);

  GES_METADATA_LOCK (data);
  ges_metadata_register (metadata_item, G_TYPE_STRING);
  gst_tag_list_add (data->list, data->mode, metadata_item, value, NULL);
  GES_METADATA_UNLOCK (data);
}

void
ges_metadata_container_foreach (GESMetadataContainer * container,
    GESMetadataContainerForeachFunc func, gpointer user_data)
{

}


/**
 * ges_metadata_container_to_string:
 * @container: a #GESMetadataContainer
 *
 * Serializes a metadata container to a string.
 *
 * Returns: a newly-allocated string, or NULL in case of an error. The
 *    string must be freed with g_free() when no longer needed.
 */
gchar *
ges_metadata_container_to_string (GESMetadataContainer * container)
{
  GESMetadata *data;

  g_return_val_if_fail (GES_IS_METADATA_CONTAINER (container), NULL);

  data = ges_metadata_container_get_data (container);

  return gst_tag_list_to_string (data->list);
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

/* Copied from gsttaglist.c */
/***** evil macros to get all the *_get_* functions right *****/

#define CREATE_GETTER(name,type,ret)                                       \
gboolean                                                                   \
ges_metadata_container_get_ ## name (GESMetadataContainer *container,      \
                           const gchar *metadata_item, type *value)        \
{                                                                          \
  GValue v = { 0, };                                                       \
  GESMetadata *data;                                                       \
                                                                           \
  g_return_val_if_fail (GES_IS_METADATA_CONTAINER (container), FALSE);     \
  g_return_val_if_fail (metadata_item != NULL, FALSE);                     \
  g_return_val_if_fail (value != NULL, FALSE);                             \
                                                                           \
  data = ges_metadata_container_get_data (container);                      \
                                                                           \
  if (!gst_tag_list_copy_value (&v, data->list, metadata_item))            \
      return FALSE;                                                        \
  *value = COPY_FUNC (g_value_get_ ## name (&v));                          \
  g_value_unset (&v);                                                      \
  return ret;                                                              \
}

#define COPY_FUNC /**/
CREATE_GETTER (boolean, gboolean, TRUE);
CREATE_GETTER (int, gint, TRUE);
CREATE_GETTER (uint, guint, TRUE);
CREATE_GETTER (int64, gint64, TRUE);
CREATE_GETTER (float, gfloat, TRUE);
CREATE_GETTER (double, gdouble, TRUE);
CREATE_GETTER (uint64, guint64, TRUE);

static inline gchar *
_gst_strdup0 (const gchar * s)
{
  if (s == NULL || *s == '\0')
    return NULL;

  return g_strdup (s);
}

#undef COPY_FUNC
#define COPY_FUNC _gst_strdup0
CREATE_GETTER (string, gchar *, (*value != NULL));


gboolean
ges_metadata_container_get_date (GESMetadataContainer * container,
    const gchar * tag, GDate ** value)
{
  GValue v = { 0, };
  GESMetadata *data;

  g_return_val_if_fail (GES_IS_METADATA_CONTAINER (container), FALSE);
  g_return_val_if_fail (tag != NULL, FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  data = ges_metadata_container_get_data (container);

  if (!gst_tag_list_copy_value (&v, data->list, tag))
    return FALSE;
  *value = (GDate *) g_value_dup_boxed (&v);
  g_value_unset (&v);
  return (*value != NULL);
}

gboolean
ges_metadata_container_get_date_time (GESMetadataContainer * container,
    const gchar * tag, GstDateTime ** value)
{
  GValue v = { 0, };
  GESMetadata *data;

  g_return_val_if_fail (GES_IS_METADATA_CONTAINER (container), FALSE);
  g_return_val_if_fail (tag != NULL, FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  data = ges_metadata_container_get_data (container);

  if (!gst_tag_list_copy_value (&v, data->list, tag))
    return FALSE;

  g_return_val_if_fail (GST_VALUE_HOLDS_DATE_TIME (&v), FALSE);

  *value = (GstDateTime *) g_value_dup_boxed (&v);
  g_value_unset (&v);
  return (*value != NULL);
}
