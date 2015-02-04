/**
 *
 * GSettings libcompizconfig backend
 *
 * ccs_mate_integration_gsettings_wrapper_factory.c
 *
 * Copyright (c) 2012 Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored By:
 *	Sam Spilsbury <sam.spilsbury@canonical.com>
 *
 **/
#include <ccs-object.h>
#include "ccs_gsettings_wrapper_factory.h"
#include "ccs_gsettings_wrapper_factory_interface.h"
#include "ccs_gsettings_interface.h"
#include "ccs_gsettings_interface_wrapper.h"
#include "ccs_mate_integration_gsettings_wrapper_factory.h"

/* CCSGSettingsWrapperFactory implementation */
typedef struct _CCSMATEIntegrationGSettingsWrapperFactoryPrivate CCSMATEIntegrationGSettingsWrapperFactoryPrivate;
struct _CCSMATEIntegrationGSettingsWrapperFactoryPrivate
{
    CCSGSettingsWrapperFactory *wrapperFactory;
    GCallback                  callback;

    /* This is expected to stay alive during the
     * lifetime of this object */
    CCSMATEValueChangeData    *data;
};

static void
ccsMATEIntegrationGSettingsWrapperFree (CCSGSettingsWrapperFactory *wrapperFactory)
{
    CCSMATEIntegrationGSettingsWrapperFactoryPrivate *priv =
	    GET_PRIVATE (CCSMATEIntegrationGSettingsWrapperFactoryPrivate, wrapperFactory);

    ccsGSettingsWrapperFactoryUnref (priv->wrapperFactory);

    ccsObjectFinalize (wrapperFactory);
    (*wrapperFactory->object.object_allocation->free_) (wrapperFactory->object.object_allocation->allocator,
							wrapperFactory);
}

static void
connectWrapperToChangedSignal (CCSGSettingsWrapper                               *wrapper,
			       CCSMATEIntegrationGSettingsWrapperFactoryPrivate *priv)
{
    ccsGSettingsWrapperConnectToChangedSignal (wrapper, priv->callback, priv->data);
}

static CCSGSettingsWrapper *
ccsMATEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper (CCSGSettingsWrapperFactory   *factory,
							       const gchar                  *schemaName,
							       CCSObjectAllocationInterface *ai)
{
    CCSMATEIntegrationGSettingsWrapperFactoryPrivate *priv =
	    GET_PRIVATE (CCSMATEIntegrationGSettingsWrapperFactoryPrivate, factory);
    CCSGSettingsWrapper *wrapper = ccsGSettingsWrapperFactoryNewGSettingsWrapper (priv->wrapperFactory,
										  schemaName,
										  factory->object.object_allocation);

    if (wrapper != NULL)
	connectWrapperToChangedSignal (wrapper, priv);

    return wrapper;
}

static CCSGSettingsWrapper *
ccsMATEIntegrationGSettingsWrapperFactoryNewGSettingsWrapperWithPath (CCSGSettingsWrapperFactory   *factory,
								       const gchar                  *schemaName,
								       const gchar                  *path,
								       CCSObjectAllocationInterface *ai)
{
    CCSMATEIntegrationGSettingsWrapperFactoryPrivate *priv =
	    GET_PRIVATE (CCSMATEIntegrationGSettingsWrapperFactoryPrivate, factory);
    CCSGSettingsWrapper *wrapper = ccsGSettingsWrapperFactoryNewGSettingsWrapperWithPath (priv->wrapperFactory,
											  schemaName,
											  path,
											  factory->object.object_allocation);

    connectWrapperToChangedSignal (wrapper, priv);

    return wrapper;
}

const CCSGSettingsWrapperFactoryInterface ccsMATEIntegrationGSettingsWrapperFactoryInterface =
{
    ccsMATEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper,
    ccsMATEIntegrationGSettingsWrapperFactoryNewGSettingsWrapperWithPath,
    ccsMATEIntegrationGSettingsWrapperFree
};

CCSGSettingsWrapperFactory *
ccsMATEIntegrationGSettingsWrapperFactoryDefaultImplNew (CCSObjectAllocationInterface *ai,
							  CCSGSettingsWrapperFactory   *factory,
							  GCallback                    callback,
							  CCSMATEValueChangeData      *data)
{
    CCSGSettingsWrapperFactory *wrapperFactory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGSettingsWrapperFactory));

    if (!wrapperFactory)
	return NULL;

    CCSMATEIntegrationGSettingsWrapperFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSMATEIntegrationGSettingsWrapperFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, wrapperFactory);
	return NULL;
    }

    ccsGSettingsWrapperFactoryRef (factory);

    priv->wrapperFactory = factory;
    priv->callback = callback;
    priv->data = data;

    ccsObjectInit (wrapperFactory, ai);
    ccsObjectAddInterface (wrapperFactory, (const CCSInterface *) &ccsMATEIntegrationGSettingsWrapperFactoryInterface, GET_INTERFACE_TYPE (CCSGSettingsWrapperFactoryInterface));
    ccsObjectSetPrivate (wrapperFactory, (CCSPrivate *) priv);

    ccsGSettingsWrapperFactoryRef (wrapperFactory);

    return wrapperFactory;
}
