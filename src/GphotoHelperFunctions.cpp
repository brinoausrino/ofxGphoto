#include "GphotoHelperFunctions.h"

namespace ofxGphoto{
/*
 * This function opens a camera depending on the specified model and port.
 */

static GPPortInfoList		*portinfolist = NULL;
static CameraAbilitiesList	*abilities = NULL;

/*
 * This function looks up a label or key entry of
 * a configuration widget.
 * The functions descend recursively, so you can just
 * specify the last component.
 */

static int
_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) {
	int ret;
	ret = gp_widget_get_child_by_name (widget, key, child);
	if (ret < GP_OK)
		ret = gp_widget_get_child_by_label (widget, key, child);
	return ret;
}

int sampleOpenCamera (Camera ** camera, const char *model, const char *port, GPContext *context) {
	int		ret, m, p;
	CameraAbilities	a;
	GPPortInfo	pi;

	ret = gp_camera_new (camera);
	if (ret < GP_OK) return ret;

	if (!abilities) {
		/* Load all the camera drivers we have... */
		ret = gp_abilities_list_new (&abilities);
		if (ret < GP_OK) return ret;
		ret = gp_abilities_list_load (abilities, context);
		if (ret < GP_OK) return ret;
	}

	/* First lookup the model / driver */
        m = gp_abilities_list_lookup_model (abilities, model);
	if (m < GP_OK) return ret;
        ret = gp_abilities_list_get_abilities (abilities, m, &a);
	if (ret < GP_OK) return ret;
        ret = gp_camera_set_abilities (*camera, a);
	if (ret < GP_OK) return ret;

	if (!portinfolist) {
		/* Load all the port drivers we have... */
		ret = gp_port_info_list_new (&portinfolist);
		if (ret < GP_OK) return ret;
		ret = gp_port_info_list_load (portinfolist);
		if (ret < 0) return ret;
		ret = gp_port_info_list_count (portinfolist);
		if (ret < 0) return ret;
	}

	/* Then associate the camera with the specified port */
        p = gp_port_info_list_lookup_path (portinfolist, port);
        switch (p) {
        case GP_ERROR_UNKNOWN_PORT:
			ofLogError()<< "The port you specified "
                        "('%s') can not be found. Please "
                        "specify one of the ports found by "
                        "'gphoto2 --list-ports' and make "
                        "sure the spelling is correct "
						"(i.e. with prefix 'serial:' or 'usb:')."
							   << port;
                break;
        default:
                break;
        }
        if (p < GP_OK) return p;

        ret = gp_port_info_list_get_info (portinfolist, p, &pi);
        if (ret < GP_OK) return ret;
        ret = gp_camera_set_port_info (*camera, pi);
        if (ret < GP_OK) return ret;
        return GP_OK;
}

/* Gets a string configuration value.
 * This can be:
 *  - A Text widget
 *  - The current selection of a Radio Button choice
 *  - The current selection of a Menu choice
 *
 * Sample (for Canons eg):
 *   get_config_value_string (camera, "owner", &ownerstr, context);
 */
int getConfigValueString(Camera *camera, const char *key, char **str, GPContext *context)
{
    CameraWidget		*widget = NULL, *child = NULL;
            CameraWidgetType	type;
            int			ret;
            char			*val;

            ret = gp_camera_get_single_config (camera, key, &child, context);
            if (ret == GP_OK) {
					if (!child) ofLogError("ofxGphoto::getDeviceInfo") << "child is NULL?";
                    widget = child;
            } else {
                    ret = gp_camera_get_config (camera, &widget, context);
                    if (ret < GP_OK) {
							ofLogError("ofxGphoto::getDeviceInfo") << "camera_get_config failed: " << ret;
                            return ret;
                    }
                    ret = _lookup_widget (widget, key, &child);
                    if (ret < GP_OK) {
							ofLogError("ofxGphoto::getDeviceInfo") <<  "lookup widget failed: " << ret;
                            goto out;
                    }
            }

            /* This type check is optional, if you know what type the label
             * has already. If you are not sure, better check. */
            ret = gp_widget_get_type (child, &type);
            if (ret < GP_OK) {
					ofLogError("ofxGphoto::getDeviceInfo") <<  "widget get type failed: "<< ret;
                    goto out;
            }
            switch (type) {
            case GP_WIDGET_MENU:
            case GP_WIDGET_RADIO:
            case GP_WIDGET_TEXT:
                    break;
            default:
				   ofLogError("ofxGphoto::getDeviceInfo") <<  "widget has bad type " << type;
                    ret = GP_ERROR_BAD_PARAMETERS;
                    goto out;
            }

            /* This is the actual query call. Note that we just
             * a pointer reference to the string, not a copy... */
            ret = gp_widget_get_value (child, &val);
            if (ret < GP_OK) {
					ofLogError("ofxGphoto::getDeviceInfo") <<  "could not query widget value: " << ret;
                    goto out;
            }
            /* Create a new copy for our caller. */
            *str = strdup (val);
    out:
            gp_widget_free (widget);
            return ret;
}

}
