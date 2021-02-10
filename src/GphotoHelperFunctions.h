#ifndef GPHOTOHELPERFUNCTIONS_H
#define GPHOTOHELPERFUNCTIONS_H

#include <gphoto2/gphoto2-camera.h>
#include "ofLog.h"


namespace ofxGphoto{
    int sampleOpenCamera (Camera ** camera, const char *model, const char *port, GPContext *context) ;
    int getConfigValueString (Camera *camera, const char *key, char **str, GPContext *context);
}

#endif // GPHOTOHELPERFUNCTIONS_H
