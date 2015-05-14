/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs_mate_integrated_setting.h>
#include "compizconfig_ccs_mock_mate_integrated_setting_composition.h"

typedef struct _CCSMockMATEIntegratedSettingCompositionPrivate
{
    CCSIntegratedSetting          *integratedSetting;
    CCSMATEIntegratedSettingInfo *mateIntegratedSettingInfo;
    CCSIntegratedSettingInfo      *integratedSettingInfo;
} CCSMockMATEIntegratedSettingCompositionPrivate;

static CCSIntegratedSetting *
allocateCCSIntegratedSetting (CCSObjectAllocationInterface *allocator)
{
    CCSIntegratedSetting *setting =
	    (*allocator->calloc_) (allocator->allocator,
				   1,
				   sizeof (CCSIntegratedSetting));

    ccsObjectInit (setting, allocator);

    return setting;
}

static CCSMockMATEIntegratedSettingCompositionPrivate *
allocatePrivate (CCSIntegratedSetting         *integratedSetting,
		 CCSObjectAllocationInterface *allocator)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv =
	    (*allocator->calloc_) (allocator->allocator,
				   1,
				   sizeof (CCSMockMATEIntegratedSettingCompositionPrivate));

    if (!priv)
    {
	ccsObjectFinalize (integratedSetting);
	(*allocator->free_) (allocator->allocator, integratedSetting);
	return NULL;
    }

    return priv;
}

static SpecialOptionType
ccsMockCompositionIntegratedSettingGetSpecialOptionType (CCSMATEIntegratedSettingInfo *setting)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv = GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    return ccsMATEIntegratedSettingInfoGetSpecialOptionType (priv->mateIntegratedSettingInfo);
}

static const char *
ccsMockCompositionIntegratedSettingGetMATEName (CCSMATEIntegratedSettingInfo *setting)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv = GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    return ccsMATEIntegratedSettingInfoGetMATEName (priv->mateIntegratedSettingInfo);
}

static CCSSettingValue *
ccsMockCompositionIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv =
	    GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    return ccsIntegratedSettingReadValue (priv->integratedSetting, type);
}

static void
ccsMockCompositionIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue *v, CCSSettingType type)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv =
	    GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    ccsIntegratedSettingWriteValue (priv->integratedSetting, v, type);
}

static const char *
ccsMockCompositionIntegratedSettingInfoPluginName (CCSIntegratedSettingInfo *setting)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv = GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    return ccsIntegratedSettingInfoPluginName (priv->integratedSettingInfo);
}

static const char *
ccsMockCompositionIntegratedSettingInfoSettingName (CCSIntegratedSettingInfo *setting)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv = GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    return ccsIntegratedSettingInfoSettingName (priv->integratedSettingInfo);
}

static CCSSettingType
ccsMockCompositionIntegratedSettingInfoGetType (CCSIntegratedSettingInfo *setting)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv = GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, setting);

    return ccsIntegratedSettingInfoGetType (priv->integratedSettingInfo);
}

static void
ccsMockCompositionIntegratedSettingFree (CCSIntegratedSetting        *integratedSetting)
{
    CCSMockMATEIntegratedSettingCompositionPrivate *priv =
	    GET_PRIVATE (CCSMockMATEIntegratedSettingCompositionPrivate, integratedSetting);

    ccsIntegratedSettingUnref (priv->integratedSetting);
    ccsMATEIntegratedSettingInfoUnref (priv->mateIntegratedSettingInfo);

    ccsObjectFinalize (integratedSetting);
    (*integratedSetting->object.object_allocation->free_)
	    (integratedSetting->object.object_allocation->allocator, integratedSetting);
}

static void
ccsMockCompositionIntegratedSettingInfoFree (CCSIntegratedSettingInfo *info)
{
    return ccsMockCompositionIntegratedSettingFree ((CCSIntegratedSetting *) info);
}

static void
ccsMockCompositionMATEIntegratedSettingInfoFree (CCSMATEIntegratedSettingInfo *info)
{
    return ccsMockCompositionIntegratedSettingFree ((CCSIntegratedSetting *) info);
}

const CCSMATEIntegratedSettingInfoInterface ccsMockCompositionMATEIntegratedSettingInfo =
{
    ccsMockCompositionIntegratedSettingGetSpecialOptionType,
    ccsMockCompositionIntegratedSettingGetMATEName,
    ccsMockCompositionMATEIntegratedSettingInfoFree
};

const CCSIntegratedSettingInterface ccsMockCompositionIntegratedSetting =
{
    ccsMockCompositionIntegratedSettingReadValue,
    ccsMockCompositionIntegratedSettingWriteValue,
    ccsMockCompositionIntegratedSettingFree
};

const CCSIntegratedSettingInfoInterface ccsMockCompositionIntegratedSettingInfo =
{
    ccsMockCompositionIntegratedSettingInfoPluginName,
    ccsMockCompositionIntegratedSettingInfoSettingName,
    ccsMockCompositionIntegratedSettingInfoGetType,
    ccsMockCompositionIntegratedSettingInfoFree
};

CCSIntegratedSetting *
ccsMockCompositionIntegratedSettingNew (CCSIntegratedSetting          *integratedSetting,
					CCSMATEIntegratedSettingInfo *mateInfo,
					CCSIntegratedSettingInfo      *settingInfo,
					CCSObjectAllocationInterface  *allocator)
{
    CCSIntegratedSetting *composition = allocateCCSIntegratedSetting (allocator);

    if (!composition)
	return NULL;

    CCSMockMATEIntegratedSettingCompositionPrivate *priv = allocatePrivate (composition,
									     allocator);

    if (!priv)
	return NULL;

    const CCSInterface *integratedSettingImpl =
	    (const CCSInterface *) (&ccsMockCompositionIntegratedSetting);
    const CCSInterface *integratedSettingInfoImpl =
	    (const CCSInterface *) (&ccsMockCompositionIntegratedSettingInfo);
    const CCSInterface *mateSettingImpl =
	    (const CCSInterface *) (&ccsMockCompositionMATEIntegratedSettingInfo);

    priv->integratedSetting          = integratedSetting;
    priv->mateIntegratedSettingInfo = mateInfo;
    priv->integratedSettingInfo      = settingInfo;

    ccsIntegratedSettingRef (priv->integratedSetting);
    ccsMATEIntegratedSettingInfoRef (priv->mateIntegratedSettingInfo);

    ccsObjectSetPrivate (composition, (CCSPrivate *) (priv));
    ccsObjectAddInterface (composition,
			   integratedSettingImpl,
			   GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (composition,
			   integratedSettingInfoImpl,
			   GET_INTERFACE_TYPE (CCSIntegratedSettingInfoInterface));
    ccsObjectAddInterface (composition,
			   mateSettingImpl,
			   GET_INTERFACE_TYPE (CCSMATEIntegratedSettingInfoInterface));

    ccsObjectRef (composition);

    return composition;
}

