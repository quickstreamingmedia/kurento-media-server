/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "PointerDetector2Filter.hpp"

#include "KmsMediaPointerDetector2FilterType_constants.h"

#include "utils/utils.hpp"
#include "utils/marshalling.hpp"
#include "KmsMediaDataType_constants.h"
#include "KmsMediaErrorCodes_constants.h"

#include "protocol/TBinaryProtocol.h"
#include "transport/TBufferTransports.h"

#define GST_CAT_DEFAULT kurento_pointer_detector2_filter
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "KurentoPointerDetector2Filter"

#define WINDOWS_LAYOUT "windows-layout"
#define CALIBRATION_AREA "calibration-area"
#define CALIBRATE_COLOR "calibrate-color"

using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::protocol::TBinaryProtocol;

namespace kurento
{

void
pointerDetector2_receive_message (GstBus *bus, GstMessage *message,
                                  gpointer pointerDetector2)
{
  const GstStructure *st;
  gchar *windowID;
  const gchar *type;
  std::string windowIDStr, typeStr;
  PointerDetector2Filter *filter = (PointerDetector2Filter *) pointerDetector2;

  if (GST_MESSAGE_SRC (message) != GST_OBJECT (filter->pointerDetector2) ||
      GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT) {
    return;
  }

  st = gst_message_get_structure (message);
  type = gst_structure_get_name (st);

  if ( (g_strcmp0 (type, "window-out") != 0) &&
       (g_strcmp0 (type, "window-in") != 0) ) {
    GST_WARNING ("The message does not have the correct name");
    return;
  }

  if (!gst_structure_get (st, "window", G_TYPE_STRING , &windowID, NULL) ) {
    GST_WARNING ("The message does not contain the window ID");
    return;
  }

  windowIDStr = windowID;
  typeStr = type;

  g_free (windowID);

  filter->raiseEvent (typeStr, windowIDStr);

}

/* default constructor */
PointerDetector2Filter::PointerDetector2Filter (
  MediaSet &mediaSet, std::shared_ptr<MediaPipeline> parent,
  const std::map<std::string, KmsMediaParam> &params)
  : Filter (mediaSet, parent,
            g_KmsMediaPointerDetector2FilterType_constants.TYPE_NAME, params)
{
  const KmsMediaParam *p;
  KmsMediaPointerDetector2ConstructorParams constructorParams;
  KmsMediaPointerDetectorWindowSet windowSet;
  GstStructure *calibrationArea;

  g_object_set (element, "filter-factory", "pointerdetector2", NULL);

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (parent->pipeline) );
  GstElement *pointerDetector2;

  g_object_get (G_OBJECT (element), "filter", &pointerDetector2, NULL);

  this->pointerDetector2 = pointerDetector2;

  if (this->pointerDetector2 == NULL) {
    g_object_unref (bus);
    KmsMediaServerException except;

    createKmsMediaServerException (except,
                                   g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_NOT_AVAILAIBLE,
                                   "Media Object not available");
    throw except;
  }

  p = getParam (params,
                g_KmsMediaPointerDetector2FilterType_constants.CONSTRUCTOR_PARAMS_DATA_TYPE);

  if (p == NULL) {
    KmsMediaServerException except;

    g_object_unref (bus);
    g_object_unref (pointerDetector2);

    createKmsMediaServerException (except,
                                   g_KmsMediaErrorCodes_constants.UNMARSHALL_ERROR,
                                   "Constructor Parameters not found");
    throw except;
  }

  unmarshalStruct (constructorParams, p->data);

  calibrationArea = gst_structure_new (
                      "calibration_area",
                      "x", G_TYPE_INT, constructorParams.colorCalibrationRegion.point.x,
                      "y", G_TYPE_INT, constructorParams.colorCalibrationRegion.point.y,
                      "width", G_TYPE_INT, constructorParams.colorCalibrationRegion.width,
                      "height", G_TYPE_INT, constructorParams.colorCalibrationRegion.height,
                      NULL);

  g_object_set (G_OBJECT (this->pointerDetector2), CALIBRATION_AREA,
                calibrationArea, NULL);
  gst_structure_free (calibrationArea);

  if (constructorParams.__isset.windowSet) {
    GstStructure *buttonsLayout;
    //there are data about windows
    /* set the window layout list */
    windowSet = constructorParams.windowSet;
    buttonsLayout = gst_structure_new_empty  ("windowsLayout");

    for (auto it = windowSet.windows.begin(); it != windowSet.windows.end(); ++it) {
      KmsMediaPointerDetectorWindow windowInfo = *it;
      GstStructure *buttonsLayoutAux;

      buttonsLayoutAux = gst_structure_new (
                           windowInfo.id.c_str(),
                           "upRightCornerX", G_TYPE_INT, windowInfo.topRightCornerX,
                           "upRightCornerY", G_TYPE_INT, windowInfo.topRightCornerY,
                           "width", G_TYPE_INT, windowInfo.width,
                           "height", G_TYPE_INT, windowInfo.height,
                           "id", G_TYPE_STRING, windowInfo.id.c_str(),
                           NULL);

      if (windowInfo.__isset.inactiveOverlayImageUri) {
        gst_structure_set (buttonsLayoutAux, "inactive_uri",
                           G_TYPE_STRING, windowInfo.inactiveOverlayImageUri.c_str(), NULL);
      }

      if (windowInfo.__isset.overlayTransparency) {
        gst_structure_set (buttonsLayoutAux, "transparency",
                           G_TYPE_DOUBLE, windowInfo.overlayTransparency, NULL);
      }

      if (windowInfo.__isset.activeOverlayImageUri) {
        gst_structure_set (buttonsLayoutAux, "active_uri",
                           G_TYPE_STRING, windowInfo.activeOverlayImageUri.c_str(), NULL);
      }

      gst_structure_set (buttonsLayout,
                         windowInfo.id.c_str(), GST_TYPE_STRUCTURE, buttonsLayoutAux,
                         NULL);
      gst_structure_free (buttonsLayoutAux);
    }

    g_object_set (G_OBJECT (this->pointerDetector2), WINDOWS_LAYOUT, buttonsLayout,
                  NULL);
    gst_structure_free (buttonsLayout);
    windowSet.windows.clear();
  }

  bus_handler_id = g_signal_connect (bus, "message",
                                     G_CALLBACK (pointerDetector2_receive_message), this);
  g_object_unref (bus);
  // There is no need to reference pointerdetector because its life cycle is the same as the filter life cycle
  g_object_unref (pointerDetector2);
}

PointerDetector2Filter::~PointerDetector2Filter() throw ()
{
  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (
                                        std::dynamic_pointer_cast<MediaPipeline> (parent)->pipeline ) );
  g_signal_handler_disconnect (bus, bus_handler_id);
  g_object_unref (bus);
}

void
PointerDetector2Filter::raiseEvent (const std::string &type,
                                    const std::string &windowID)
{
  KmsMediaEventData eventData;

  createStringEventData (eventData, windowID);

  if ( type.compare ("window-out") == 0) {
    GST_DEBUG ("Raise event. Type: %s, Window ID: %s", type.c_str(),
               windowID.c_str() );

    sendEvent (g_KmsMediaPointerDetector2FilterType_constants.EVENT_WINDOW_OUT,
               eventData);
  } else {
    GST_DEBUG ("Raise event. Type: %s, Window ID: %s", type.c_str(),
               windowID.c_str() );

    sendEvent (g_KmsMediaPointerDetector2FilterType_constants.EVENT_WINDOW_IN,
               eventData);
  }
}

void
PointerDetector2Filter::addWindow (KmsMediaPointerDetectorWindow window)
{
  GstStructure *buttonsLayout, *buttonsLayoutAux;

  buttonsLayoutAux = gst_structure_new (
                       window.id.c_str(),
                       "upRightCornerX", G_TYPE_INT, window.topRightCornerX,
                       "upRightCornerY", G_TYPE_INT, window.topRightCornerY,
                       "width", G_TYPE_INT, window.width,
                       "height", G_TYPE_INT, window.height,
                       "id", G_TYPE_STRING, window.id.c_str(),
                       NULL);

  if (window.__isset.inactiveOverlayImageUri) {
    gst_structure_set (buttonsLayoutAux, "inactive_uri",
                       G_TYPE_STRING, window.inactiveOverlayImageUri.c_str(), NULL);
  }

  if (window.__isset.overlayTransparency) {
    gst_structure_set (buttonsLayoutAux, "transparency",
                       G_TYPE_DOUBLE, window.overlayTransparency, NULL);
  }

  if (window.__isset.activeOverlayImageUri) {
    gst_structure_set (buttonsLayoutAux, "active_uri",
                       G_TYPE_STRING, window.activeOverlayImageUri.c_str(), NULL);
  }


  /* The function obtains the actual window list */
  g_object_get (G_OBJECT (pointerDetector2), WINDOWS_LAYOUT, &buttonsLayout,
                NULL);
  gst_structure_set (buttonsLayout,
                     window.id.c_str(), GST_TYPE_STRUCTURE, buttonsLayoutAux,
                     NULL);

  g_object_set (G_OBJECT (pointerDetector2), WINDOWS_LAYOUT, buttonsLayout, NULL);

  gst_structure_free (buttonsLayout);
  gst_structure_free (buttonsLayoutAux);
}

void
PointerDetector2Filter::removeWindow (std::string id)
{
  GstStructure *buttonsLayout;
  gint len;

  /* The function obtains the actual window list */
  g_object_get (G_OBJECT (pointerDetector2), WINDOWS_LAYOUT, &buttonsLayout,
                NULL);
  len = gst_structure_n_fields (buttonsLayout);

  if (len == 0) {
    GST_WARNING ("There are no windows in the layout");
    return;
  }

  for (int i = 0; i < len; i++) {
    const gchar *name;
    name = gst_structure_nth_field_name (buttonsLayout, i);

    if ( g_strcmp0 (name, id.c_str() ) == 0) {
      /* this window will be removed */
      gst_structure_remove_field (buttonsLayout, name);
    }
  }

  /* Set the buttons layout list without the window with id = id */
  g_object_set (G_OBJECT (pointerDetector2), WINDOWS_LAYOUT, buttonsLayout, NULL);

  gst_structure_free (buttonsLayout);
}

void
PointerDetector2Filter::clearWindows()
{
  GstStructure *buttonsLayout;

  buttonsLayout = gst_structure_new_empty  ("buttonsLayout");
  g_object_set (G_OBJECT (this->pointerDetector2), WINDOWS_LAYOUT, buttonsLayout,
                NULL);
  gst_structure_free (buttonsLayout);
}

void
PointerDetector2Filter::calibrateColorToTrack ()
{
  g_signal_emit_by_name (this->pointerDetector2, "calibrate-color", NULL);
}

void
PointerDetector2Filter::invoke (KmsMediaInvocationReturn &_return,
                                const std::string &command,
                                const std::map< std::string, KmsMediaParam > &params)
throw (KmsMediaServerException)
{
  if (g_KmsMediaPointerDetector2FilterType_constants.ADD_NEW_WINDOW.compare (
        command) == 0) {
    KmsMediaPointerDetectorWindow windowInfo;
    const KmsMediaParam *p;
    /* extract window params from param */
    p = getParam (params,
                  g_KmsMediaPointerDetector2FilterType_constants.ADD_NEW_WINDOW_PARAM_WINDOW);

    if (p != NULL) {
      unmarshalStruct (windowInfo, p->data);
      /* create window */
      addWindow (windowInfo);
    }
  } else if (
    g_KmsMediaPointerDetector2FilterType_constants.REMOVE_WINDOW.compare (
      command) == 0) {
    std::string id;

    getStringParam (id, params,
                    g_KmsMediaPointerDetector2FilterType_constants.REMOVE_WINDOW_PARAM_WINDOW_ID);
    removeWindow (id);
  } else if (
    g_KmsMediaPointerDetector2FilterType_constants.CLEAR_WINDOWS.compare (
      command) == 0) {
    clearWindows();
  } else if (
    g_KmsMediaPointerDetector2FilterType_constants.TRACK_COLOR_FROM_CALIBRATION_REGION.compare (
      command) == 0) {
    calibrateColorToTrack ();
  }
}

void
PointerDetector2Filter::subscribe (std::string &_return,
                                   const std::string &eventType,
                                   const std::string &handlerAddress,
                                   const int32_t handlerPort)
throw (KmsMediaServerException)
{
  if (g_KmsMediaPointerDetector2FilterType_constants.EVENT_WINDOW_IN ==
      eventType) {
    mediaHandlerManager.addMediaHandler (_return, eventType, handlerAddress,
                                         handlerPort);
  } else  if (g_KmsMediaPointerDetector2FilterType_constants.EVENT_WINDOW_OUT ==
              eventType) {
    mediaHandlerManager.addMediaHandler (_return, eventType, handlerAddress,
                                         handlerPort);
  } else {
    Filter::subscribe (_return, eventType, handlerAddress, handlerPort);
  }
}

PointerDetector2Filter::StaticConstructor
PointerDetector2Filter::staticConstructor;

PointerDetector2Filter::StaticConstructor::StaticConstructor()
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);
}

} // kurento
