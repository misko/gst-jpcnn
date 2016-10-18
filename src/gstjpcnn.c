/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 root <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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

/**
 * SECTION:element-jpcnn
 *
 * FIXME:Describe jpcnn here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! jpcnn ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/video/video.h>
#include <gst/gst.h>
#include <libjpcnn.h>
#include "gstjpcnn.h"
#include <string.h>
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (gst_jpcnn_debug);
#define GST_CAT_DEFAULT gst_jpcnn_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_NETWORKA,
  PROP_NETWORKB
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ( GST_VIDEO_CAPS_MAKE ("RGB") )
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ( GST_VIDEO_CAPS_MAKE ("RGB") )
    );

#define gst_jpcnn_parent_class parent_class
G_DEFINE_TYPE (Gstjpcnn, gst_jpcnn, GST_TYPE_ELEMENT);

static void gst_jpcnn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_jpcnn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_jpcnn_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_jpcnn_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the jpcnn's class */
static void
gst_jpcnn_class_init (GstjpcnnClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_jpcnn_set_property;
  gobject_class->get_property = gst_jpcnn_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "jpcnn",
    "jpccn library plugin",
    "jpcnn",
    "Misko Dzamba misko@petbot.ca");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  g_object_class_install_property (gobject_class, PROP_NETWORKA,
      g_param_spec_string ("networka", "Network", "Network filename",
          "", G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_NETWORKB,
      g_param_spec_string ("networkb", "Network", "Network filename",
          "", G_PARAM_READWRITE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_jpcnn_init (Gstjpcnn * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_jpcnn_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_jpcnn_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
  filter->networkaHandle=NULL;
  filter->networkbHandle=NULL;
  filter->detections=0;
  filter->layer=0;
}

static void
gst_jpcnn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstjpcnn *filter = GST_JPCNN (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_NETWORKA: 
      filter->networka_fn = strdup(g_value_get_string (value));
      filter->networkaHandle=jpcnn_create_network(filter->networka_fn);
      if (filter->networkaHandle==NULL) {
	g_print("Failed to initialize network a from file!\n");
      }
      break;
    case PROP_NETWORKB: 
      filter->networkb_fn = strdup(g_value_get_string (value));
      filter->networkbHandle=jpcnn_create_network(filter->networkb_fn);
      if (filter->networkbHandle==NULL) {
	g_print("Failed to initialize network b from file!\n");
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_jpcnn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstjpcnn *filter = GST_JPCNN (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_NETWORKA:
      g_value_set_string (value, filter->networka_fn);
      break;
    case PROP_NETWORKB:
      g_value_set_string (value, filter->networkb_fn);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_jpcnn_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
  Gstjpcnn *filter;

  filter = GST_JPCNN (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;
      gint width, height; 
      gst_event_parse_caps (event, &caps);
	GstStructure *structure;
      structure = gst_caps_get_structure (caps, 0);
      gst_structure_get_int (structure, "width", &width);
      gst_structure_get_int (structure, "height", &height);
      filter->width=width;
      filter->height=height;

      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_jpcnn_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstjpcnn *filter;

  filter = GST_JPCNN (parent);

  GstMapInfo in_map;
  gst_buffer_map (buf, &in_map, GST_MAP_READ);

  if (filter->silent == FALSE)
        if (in_map.size<=0) {
                // TODO: needed?
                GST_WARNING("Received empty buffer");
                //outbuf = gst_buffer_new();
                //gst_buffer_set_caps(outbuf, GST_PAD_CAPS(filter->srcpad));
                //GST_BUFFER_TIMESTAMP(outbuf) = GST_BUFFER_TIMESTAMP(buf);
                return gst_pad_push (filter->srcpad, buf);
        }

 if (filter->networkaHandle!=NULL && filter->networkbHandle!=NULL) {
	//fprintf(stderr,"FILTER %d x %d\n",filter->width, filter->height);
  float* predictions;
  int predictionsLength;
  char** predictionsLabels;
  int predictionsLabelsLength;
      void * imageHandle = jpcnn_create_image_buffer_from_uint8_data(in_map.data, filter->width, filter->height, 3, (3 * filter->width), 0, 0);
      char * network_fn=NULL;
      if (filter->detections++%2==0) {
         jpcnn_classify_image(filter->networkaHandle, imageHandle, 0, filter->layer, &predictions, &predictionsLength, &predictionsLabels, &predictionsLabelsLength);
	 network_fn=filter->networka_fn;
      } else {
         jpcnn_classify_image(filter->networkbHandle, imageHandle, 0, filter->layer, &predictions, &predictionsLength, &predictionsLabels, &predictionsLabelsLength);
	 network_fn=filter->networkb_fn;
      }
      jpcnn_destroy_image_buffer(imageHandle);

    int m;
    float p[4]= {0,0,0,0};
    //G_TYPE_POINTER 
    //get probs for dog, cat, person, room
    for (m=0; m<predictionsLength; m++) {
	char * label = predictionsLabels[m];
	if (strcmp(label,"person")==0) {
		p[0]=predictions[m];
	} else if (strcmp(label,"room")==0) {
		p[1]=predictions[m];
	} else if (strcmp(label,"cat")==0) {
		p[2]=predictions[m];
        } else {
		p[3]+=predictions[m];
        }
    }
          GstStructure *s = gst_structure_new ("jpcnn", "person",
              G_TYPE_FLOAT, p[0], "room", G_TYPE_FLOAT,
              p[1],"cat",G_TYPE_FLOAT,p[2],"dog",G_TYPE_FLOAT,p[3], NULL);
          GstMessage * ms = gst_message_new_element (GST_OBJECT (filter), s);
          gboolean mr = gst_element_post_message (GST_ELEMENT (filter), ms);
	if (!mr) {
		g_print("Failed to send message!\n");
        }
    //free(predictions); //TODO THIS IS STILL AMEMORY LEAK IN JPCNN?
    //print the top 5 
	/*
    for (m=0; m<5; m++ ){
      double max = -1;
      int max_i = -1;
      int i;
      for (i=0; i<predictionsLength; i++) {
        if (predictions[i]>max) {
          max_i=i; max=predictions[i];
        }
      }
      if (max_i==-1) {
	continue;
      }
      predictions[max_i]=-1;
      fprintf(stdout,"%s%s", predictionsLabels[max_i],m==5-1 ? "\n" : "\t");
    }*/
  /* just push out the incoming buffer without touching it */
  }

  /* just push out the incoming buffer without touching it */
  gst_buffer_unmap (buf, &in_map);
  return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
jpcnn_init (GstPlugin * jpcnn)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template jpcnn' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_jpcnn_debug, "jpcnn",
      0, "Template jpcnn");

  return gst_element_register (jpcnn, "jpcnn", GST_RANK_NONE,
      GST_TYPE_JPCNN);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstjpcnn"
#endif

/* gstreamer looks for this structure to register jpcnns
 *
 * exchange the string 'Template jpcnn' with your jpcnn description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    jpcnn,
    "Template jpcnn",
    jpcnn_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
