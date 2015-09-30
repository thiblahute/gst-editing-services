/* GStreamer GES plugin
 *
 * Copyright (C) 2015 Thibault Saunier <thibault.saunier@collabora.com>
 *
 * gstges.c
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef __GST_GES_SOURCE_H__
#define __GST_GES_SOURCE_H__

#include <gst/gst.h>
#include <ges/ges.h>

G_BEGIN_DECLS

GType gst_ges_source_get_type (void);

#define GST_GES_SOURCE_TYPE (gst_ges_source_get_type ())
#define GST_GES_SOURCE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_GES_SOURCE_TYPE, GstGesSource))
#define GST_GES_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_GES_SOURCE_TYPE, GstGesSourceClass))
#define GST_IS_GES_SOURCE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_GES_SOURCE_TYPE))
#define GST_IS_GES_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_GES_SOURCE_TYPE))
#define GST_GES_SOURCE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_GES_SOURCE_TYPE, GstGesSourceClass))

typedef struct {
  GstBin parent;

  GESTimeline *timeline;
} GstGesSource;

typedef struct {
  GstBinClass parent;

} GstGesSourceClass;

G_END_DECLS
#endif /* __GST_GES_SOURCE_H__ */
