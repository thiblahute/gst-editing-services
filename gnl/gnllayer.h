/* GStreamer
 * Copyright (C) 2001 Wim Taymans <wim.taymans@chello.be>
 *
 * gnllayer.h: Header for base GnlLayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __GNL_LAYER_H__
#define __GNL_LAYER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GNL_TYPE_LAYER \
  (gnl_layer_get_type())
#define GNL_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GNL_TYPE_LAYER,GnlLayer))
#define GNL_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GNL_TYPE_LAYER,GnlLayerClass))
#define GNL_IS_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GNL_TYPE_LAYER))
#define GNL_IS_LAYER_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GNL_TYPE_LAYER))

typedef struct _GnlLayer GnlLayer;
typedef struct _GnlLayerClass GnlLayerClass;

struct _GnlLayer {
  GnlSource	source;

  GList		*sources;
};

struct _GnlLayerClass {
  GnlSourceClass	parent_class;
};

GType		gnl_layer_get_type		(void);
GnlLayer*	gnl_layer_new			(void);

void		gnl_layer_add_source 		(GnlLayer *layer, GnlSource *source, guint64 start);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GNL_LAYER_H__ */

