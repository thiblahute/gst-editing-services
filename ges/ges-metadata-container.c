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
