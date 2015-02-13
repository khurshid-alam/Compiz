#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include "ccs_mate_integrated_setting.h"
#include "ccs_mate_integration_constants.h"

INTERFACE_TYPE (CCSMATEIntegratedSettingInfoInterface);

CCSREF_OBJ (MATEIntegratedSettingInfo, CCSMATEIntegratedSettingInfo);

SpecialOptionType
ccsMATEIntegratedSettingInfoGetSpecialOptionType (CCSMATEIntegratedSettingInfo *info)
{
    return (*(GET_INTERFACE (CCSMATEIntegratedSettingInfoInterface, info))->getSpecialOptionType) (info);
}

const char *
ccsMATEIntegratedSettingInfoGetMATEName (CCSMATEIntegratedSettingInfo *info)
{
    return (*(GET_INTERFACE (CCSMATEIntegratedSettingInfoInterface, info))->getMATEName) (info);
}

/* CCSMATEIntegratedSettingDefaultImpl implementation */

typedef struct _CCSMATEIntegratedSettingInfoDefaultImplPrivate CCSMATEIntegratedSettingInfoDefaultImplPrivate;

struct _CCSMATEIntegratedSettingInfoDefaultImplPrivate
{
    SpecialOptionType type;
    const char	      *mateName;
    CCSIntegratedSettingInfo *sharedIntegratedSettingInfo;
};

Bool
ccsMATEIntegrationFindSettingsMatchingPredicate (CCSIntegratedSetting *setting,
						  void		       *userData)
{
    const char *findMateName = (const char *) userData;
    const char *mateNameOfSetting = ccsMATEIntegratedSettingInfoGetMATEName ((CCSMATEIntegratedSettingInfo *) setting);

    if (strcmp (findMateName, mateNameOfSetting) == 0)
	return TRUE;

    return FALSE;
}

SpecialOptionType
ccsMATEIntegratedSettingGetSpecialOptionDefault (CCSMATEIntegratedSettingInfo *info)
{
    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSMATEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return priv->type;
}

const char *
ccsMATEIntegratedSettingGetMATENameDefault (CCSMATEIntegratedSettingInfo *info)
{
    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSMATEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return priv->mateName;
}

const char *
ccsMATEIntegratedSettingInfoPluginName (CCSIntegratedSettingInfo *info)
{
    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSMATEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoPluginName (priv->sharedIntegratedSettingInfo);
}

const char *
ccsMATEIntegratedSettingInfoSettingName (CCSIntegratedSettingInfo *info)
{
    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSMATEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoSettingName (priv->sharedIntegratedSettingInfo);
}

CCSSettingType
ccsMATEIntegratedSettingInfoGetType (CCSIntegratedSettingInfo *info)
{
    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSMATEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoGetType (priv->sharedIntegratedSettingInfo);
}

void
ccsMATESharedIntegratedSettingInfoFree (CCSIntegratedSettingInfo *info)
{
    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSMATEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    ccsIntegratedSettingInfoUnref (priv->sharedIntegratedSettingInfo);
    ccsObjectFinalize (info);

    (*info->object.object_allocation->free_) (info->object.object_allocation->allocator, info);
}

static void
ccsMATEIntegratedSettingInfoFree (CCSMATEIntegratedSettingInfo *info)
{
    ccsMATESharedIntegratedSettingInfoFree ((CCSIntegratedSettingInfo *) info);
}

CCSMATEIntegratedSettingInfoInterface ccsMATEIntegratedSettingInfoDefaultImplInterface =
{
    ccsMATEIntegratedSettingGetSpecialOptionDefault,
    ccsMATEIntegratedSettingGetMATENameDefault,
    ccsMATEIntegratedSettingInfoFree
};

const CCSIntegratedSettingInfoInterface ccsMATEIntegratedSettingInfoInterface =
{
    ccsMATEIntegratedSettingInfoPluginName,
    ccsMATEIntegratedSettingInfoSettingName,
    ccsMATEIntegratedSettingInfoGetType,
    ccsMATESharedIntegratedSettingInfoFree
};

void
ccsFreeMATEIntegratedSettingInfo (CCSMATEIntegratedSettingInfo *info)
{
    (*(GET_INTERFACE (CCSMATEIntegratedSettingInfoInterface, info))->free) (info);
}

CCSMATEIntegratedSettingInfo *
ccsMATEIntegratedSettingInfoNew (CCSIntegratedSettingInfo *base,
				  SpecialOptionType    type,
				  const char	   *mateName,
				  CCSObjectAllocationInterface *ai)
{
    CCSMATEIntegratedSettingInfo *info = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSMATEIntegratedSettingInfo));

    if (!info)
	return NULL;

    CCSMATEIntegratedSettingInfoDefaultImplPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSMATEIntegratedSettingInfoDefaultImplPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, info);
	return NULL;
    }

    priv->sharedIntegratedSettingInfo = base;
    priv->mateName = mateName;
    priv->type = type;

    ccsObjectInit (info, ai);
    ccsObjectSetPrivate (info, (CCSPrivate *) priv);
    ccsObjectAddInterface (info, (const CCSInterface *) &ccsMATEIntegratedSettingInfoInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInfoInterface));
    ccsObjectAddInterface (info, (const CCSInterface *) &ccsMATEIntegratedSettingInfoDefaultImplInterface, GET_INTERFACE_TYPE (CCSMATEIntegratedSettingInfoInterface));
    ccsIntegratedSettingInfoRef ((CCSIntegratedSettingInfo *) info);

    return info;
}
