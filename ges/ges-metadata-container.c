#include <glib-object.h>

#include "ges-metadata-container.h"

/**
* SECTION: ges-metadata-container
* @short_description: An interface for storing metadata
*
* FIXME: Long description needed
*/

G_DEFINE_INTERFACE (GESMetadataContainer, ges_metadata_container,
    G_TYPE_OBJECT);

static void
ges_metadata_container_default_init (GESMetadataContainerInterface * iface)
{

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

void
ges_metadata_container_set_char (GESMetadataContainer * container,
    const gchar * metadata_item, gchar value)
{

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
gst_metadata_container_register (const gchar * name, GType type,
    const gchar * nick, const gchar * blurb, GstTagMergeFunc func)
{

}
