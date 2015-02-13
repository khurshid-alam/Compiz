#include <stdlib.h>
#include <string.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include "ccs_mate_integration_gconf_integrated_setting.h"
#include "ccs_mate_integrated_setting.h"
#include "ccs_mate_integration_constants.h"


/* CCSGConfIntegratedSetting implementation */
typedef struct _CCSGConfIntegratedSettingPrivate CCSGConfIntegratedSettingPrivate;

struct _CCSGConfIntegratedSettingPrivate
{
    CCSMATEIntegratedSettingInfo *mateIntegratedSettingInfo;
    GConfClient		      *client;
    const char		      *sectionName;
};

SpecialOptionType
ccsGConfIntegratedSettingGetSpecialOptionType (CCSMATEIntegratedSettingInfo *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsMATEIntegratedSettingInfoGetSpecialOptionType (priv->mateIntegratedSettingInfo);
}

const char *
ccsGConfIntegratedSettingGetMATEName (CCSMATEIntegratedSettingInfo *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsMATEIntegratedSettingInfoGetMATEName (priv->mateIntegratedSettingInfo);
}

CCSSettingValue *
ccsGConfIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    CCSSettingValue		     *v = calloc (1, sizeof (CCSSettingValue));
    const char			     *mateKeyName = ccsMATEIntegratedSettingInfoGetMATEName ((CCSMATEIntegratedSettingInfo *) setting);
    char			     *mateKeyPath = g_strconcat (priv->sectionName, mateKeyName, NULL);

    v->isListChild = FALSE;
    v->parent = NULL;
    v->refCount = 1;

    GConfValue *gconfValue;
    GError     *err = NULL;

    gconfValue = gconf_client_get (priv->client,
				   mateKeyPath,
				   &err);

    if (!gconfValue)
    {
	ccsError ("NULL encountered while reading GConf setting");
	free (mateKeyPath);
	free (v);
	return NULL;
    }

    if (err)
    {
	ccsError ("%s", err->message);
	g_error_free (err);
	free (mateKeyPath);
	free (v);
	return NULL;
    }

    switch (type)
    {
	case TypeInt:
	    if (gconfValue->type != GCONF_VALUE_INT)
	    {
		ccsError ("Expected integer value");
		free (v);
		v = NULL;
		break;
	    }

	    v->value.asInt = gconf_value_get_int (gconfValue);
	    break;
	case TypeBool:
	    if (gconfValue->type != GCONF_VALUE_BOOL)
	    {
		ccsError ("Expected boolean value");
		free (v);
		v = NULL;
		break;
	    }

	    v->value.asBool = gconf_value_get_bool (gconfValue) ? TRUE : FALSE;
	    break;
	case TypeString:
	case TypeKey:
	    if (gconfValue->type != GCONF_VALUE_STRING)
	    {
		ccsError ("Expected string value");
		free (v);
		v = NULL;
		break;
	    }

	    const char *str = gconf_value_get_string (gconfValue);

	    v->value.asString = strdup (str ? str : "");
	    break;
	default:
	    g_assert_not_reached ();
    }

    gconf_value_free (gconfValue);
    free (mateKeyPath);

    return v;
}

void
ccsGConfIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue *v, CCSSettingType type)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    const char			     *mateKeyName = ccsMATEIntegratedSettingInfoGetMATEName ((CCSMATEIntegratedSettingInfo *) setting);
    char			     *mateKeyPath = g_strconcat (priv->sectionName, mateKeyName, NULL);
    GError			     *err = NULL;

    switch (type)
    {
	case TypeInt:
	    {
		int currentValue = gconf_client_get_int (priv->client, mateKeyPath, &err);

		if (!err && (currentValue != v->value.asInt))
		    gconf_client_set_int(priv->client, mateKeyPath,
					 v->value.asInt, NULL);
	    }
	    break;
	case TypeBool:
	    {
		Bool     newValue = v->value.asBool;
		gboolean currentValue;

		currentValue = gconf_client_get_bool (priv->client, mateKeyPath, &err);

		if (!err && ((currentValue && !newValue) ||
			     (!currentValue && newValue)))
		    gconf_client_set_bool (priv->client, mateKeyPath,
					   newValue, NULL);
	    }
	    break;
	case TypeString:
	case TypeKey:
	    {
		char  *newValue = v->value.asString;
		gchar *currentValue;

		currentValue = gconf_client_get_string (priv->client, mateKeyPath, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (priv->client, mateKeyPath,
						 newValue, NULL);
		    g_free (currentValue);
		}
	    }
	    break;
	default:
	    g_assert_not_reached ();
	    break;
    }

    if (err)
    {
	ccsError ("%s", err->message);
	g_error_free (err);
    }

    free (mateKeyPath);
}

const char *
ccsGConfIntegratedSettingInfoPluginName (CCSIntegratedSettingInfo *info)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoPluginName ((CCSIntegratedSettingInfo *) priv->mateIntegratedSettingInfo);
}

const char *
ccsGConfIntegratedSettingInfoSettingName (CCSIntegratedSettingInfo *info)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoSettingName ((CCSIntegratedSettingInfo *) priv->mateIntegratedSettingInfo);
}

CCSSettingType
ccsGConfIntegratedSettingInfoGetType (CCSIntegratedSettingInfo *info)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoGetType ((CCSIntegratedSettingInfo *) priv->mateIntegratedSettingInfo);
}

void
ccsGConfIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingInfoUnref ((CCSIntegratedSettingInfo *) priv->mateIntegratedSettingInfo);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

void
ccsGConfIntegratedSettingInfoFree (CCSIntegratedSettingInfo *info)
{
    ccsGConfIntegratedSettingFree ((CCSIntegratedSetting *) info);
}

void
ccsGConfMATEIntegratedSettingInfoFree (CCSMATEIntegratedSettingInfo *info)
{
    ccsGConfIntegratedSettingFree ((CCSIntegratedSetting *) info);
}

const CCSMATEIntegratedSettingInfoInterface ccsGConfMATEIntegratedSettingInfoInterface =
{
    ccsGConfIntegratedSettingGetSpecialOptionType,
    ccsGConfIntegratedSettingGetMATEName,
    ccsGConfMATEIntegratedSettingInfoFree
};

const CCSIntegratedSettingInterface ccsGConfIntegratedSettingInterface =
{
    ccsGConfIntegratedSettingReadValue,
    ccsGConfIntegratedSettingWriteValue,
    ccsGConfIntegratedSettingFree
};

const CCSIntegratedSettingInfoInterface ccsGConfIntegratedSettingInfoInterface =
{
    ccsGConfIntegratedSettingInfoPluginName,
    ccsGConfIntegratedSettingInfoSettingName,
    ccsGConfIntegratedSettingInfoGetType,
    ccsGConfIntegratedSettingInfoFree
};

CCSIntegratedSetting *
ccsGConfIntegratedSettingNew (CCSMATEIntegratedSettingInfo *base,
			      GConfClient		*client,
			      const char		*section,
			      CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGConfIntegratedSettingPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegratedSettingPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, priv);
	return NULL;
    }

    priv->mateIntegratedSettingInfo = base;
    priv->client = client;
    priv->sectionName = section;

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfIntegratedSettingInfoInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInfoInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfMATEIntegratedSettingInfoInterface, GET_INTERFACE_TYPE (CCSMATEIntegratedSettingInfoInterface));
    ccsIntegratedSettingRef (setting);

    return setting;
}
