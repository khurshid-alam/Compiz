#include <stdlib.h>
#include <string.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include "ccs_mate_integration.h"
#include "ccs_mate_integrated_setting.h"
#include "ccs_mate_integration_constants.h"
#include "ccs_mate_integration_gconf_integrated_setting.h"

typedef struct _CCSGConfIntegratedSettingFactoryPrivate CCSGConfIntegratedSettingFactoryPrivate;

struct _CCSGConfIntegratedSettingFactoryPrivate
{
    GConfClient *client;
    guint       mateGConfNotifyIds[NUM_WATCHED_DIRS];
    GHashTable  *pluginsToSettingsSectionsHashTable;
    GHashTable  *pluginsToSettingsSpecialTypesHashTable;
    GHashTable  *pluginsToSettingNameMATENameHashTable;
    CCSMATEValueChangeData *valueChangedData;
};

static void
mateGConfValueChanged (GConfClient *client,
			guint       cnxn_id,
			GConfEntry  *entry,
			gpointer    user_data)
{
    CCSMATEValueChangeData *data = (CCSMATEValueChangeData *) user_data;
    const gchar *keyName = gconf_entry_get_key (entry);
    gchar       *baseName = g_path_get_basename (keyName);

    /* We don't care if integration is not enabled */
    if (!ccsGetIntegrationEnabled (data->context))
	return;

    CCSIntegratedSettingList settingList = ccsIntegratedSettingsStorageFindMatchingSettingsByPredicate (data->storage,
													ccsMATEIntegrationFindSettingsMatchingPredicate,
													baseName);

    ccsIntegrationUpdateIntegratedSettings (data->integration,
					    data->context,
					    settingList);

    g_free (baseName);
}

static CCSIntegratedSetting *
createNewGConfIntegratedSetting (GConfClient *client,
				 const char  *sectionName,
				 const char  *mateName,
				 const char  *pluginName,
				 const char  *settingName,
				 CCSSettingType type,
				 SpecialOptionType specialOptionType,
				 CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSettingInfo *sharedIntegratedSettingInfo = ccsSharedIntegratedSettingInfoNew (pluginName, settingName, type, ai);

    if (!sharedIntegratedSettingInfo)
	return NULL;

    CCSMATEIntegratedSettingInfo *mateIntegratedSettingInfo = ccsMATEIntegratedSettingInfoNew (sharedIntegratedSettingInfo, specialOptionType, mateName, ai);

    if (!mateIntegratedSettingInfo)
    {
	ccsIntegratedSettingInfoUnref (sharedIntegratedSettingInfo);
	return NULL;
    }

    CCSIntegratedSetting *gconfIntegratedSetting = ccsGConfIntegratedSettingNew (mateIntegratedSettingInfo, client, sectionName, ai);

    if (!gconfIntegratedSetting)
    {
	ccsIntegratedSettingInfoUnref ((CCSIntegratedSettingInfo *) mateIntegratedSettingInfo);
	return NULL;
    }

    return gconfIntegratedSetting;
}

static void
finiGConfClient (GConfClient    *client,
		 guint		*mateGConfNotifyIds)
{
    int i;

    gconf_client_clear_cache (client);

    for (i = 0; i < NUM_WATCHED_DIRS; ++i)
    {
	if (mateGConfNotifyIds[i])
	{
	    gconf_client_notify_remove (client, mateGConfNotifyIds[0]);
	    mateGConfNotifyIds[i] = 0;
	}
	gconf_client_remove_dir (client, watchedGConfMateDirectories[i], NULL);
    }
    gconf_client_suggest_sync (client, NULL);

    g_object_unref (client);
}

static void
registerGConfClient (GConfClient    *client,
		     guint	    *mateGConfNotifyIds,
		     CCSMATEValueChangeData *data,
		     GConfClientNotifyFunc func)
{
    int i;

    for (i = 0; i < NUM_WATCHED_DIRS; ++i)
	mateGConfNotifyIds[i] = gconf_client_notify_add (client,
							  watchedGConfMateDirectories[i],
							  func, (gpointer) data,
							  NULL, NULL);
}

static void
initGConfClient (CCSIntegratedSettingFactory *factory)
{
    int i;
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    priv->client = gconf_client_get_default ();

    for (i = 0; i < NUM_WATCHED_DIRS; ++i)
	gconf_client_add_dir (priv->client, watchedGConfMateDirectories[i],
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
}

CCSIntegratedSetting *
ccsGConfIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (CCSIntegratedSettingFactory *factory,
										 CCSIntegration		     *integration,
										 const char		     *pluginName,
										 const char		     *settingName,
										 CCSSettingType		     type)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    GHashTable                              *settingsSectionsHashTable = g_hash_table_lookup (priv->pluginsToSettingsSectionsHashTable, pluginName);
    GHashTable                              *settingsSpecialTypesHashTable = g_hash_table_lookup (priv->pluginsToSettingsSpecialTypesHashTable, pluginName);
    GHashTable				    *settingsSettingNameMATENameHashTable = g_hash_table_lookup (priv->pluginsToSettingNameMATENameHashTable, pluginName);

    if (!priv->client)
	initGConfClient (factory);

    if (!priv->mateGConfNotifyIds[0])
	registerGConfClient (priv->client, priv->mateGConfNotifyIds, priv->valueChangedData, mateGConfValueChanged);

    if (settingsSectionsHashTable &&
	settingsSpecialTypesHashTable &&
	settingsSettingNameMATENameHashTable)
    {
	const gchar *sectionName = g_hash_table_lookup (settingsSectionsHashTable, settingName);
	SpecialOptionType specialType = (SpecialOptionType) GPOINTER_TO_INT (g_hash_table_lookup (settingsSpecialTypesHashTable, settingName));
	const gchar *integratedName = g_hash_table_lookup (settingsSettingNameMATENameHashTable, settingName);

	return createNewGConfIntegratedSetting (priv->client,
						sectionName,
						integratedName,
						pluginName,
						settingName,
						type,
						specialType,
						factory->object.object_allocation);
    }


    return NULL;
}

void
ccsGConfIntegratedSettingFactoryFree (CCSIntegratedSettingFactory *factory)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);

    if (priv->client)
	finiGConfClient (priv->client, priv->mateGConfNotifyIds);

    if (priv->pluginsToSettingsSectionsHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSectionsHashTable);

    if (priv->pluginsToSettingsSpecialTypesHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSpecialTypesHashTable);

    if (priv->pluginsToSettingNameMATENameHashTable)
	g_hash_table_unref (priv->pluginsToSettingNameMATENameHashTable);

    ccsObjectFinalize (factory);
    (*factory->object.object_allocation->free_) (factory->object.object_allocation->allocator, factory);
}


const CCSIntegratedSettingFactoryInterface ccsGConfIntegratedSettingFactoryInterface =
{
    ccsGConfIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType,
    ccsGConfIntegratedSettingFactoryFree
};

CCSIntegratedSettingFactory *
ccsGConfIntegratedSettingFactoryNew (GConfClient		  *client,
				     CCSMATEValueChangeData	  *valueChangedData,
				     CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSettingFactory *factory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSettingFactory));

    if (!factory)
	return NULL;

    CCSGConfIntegratedSettingFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegratedSettingFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, factory);
	return NULL;
    }

    if (client)
    {
	int i;
	priv->client = (GConfClient *) g_object_ref (client);
	for (i = 0; i < NUM_WATCHED_DIRS; ++i)
	    gconf_client_add_dir (priv->client, watchedGConfMateDirectories[i],
				  GCONF_CLIENT_PRELOAD_NONE, NULL);
    }
    else
	priv->client = NULL;

    priv->pluginsToSettingsSectionsHashTable = ccsMATEIntegrationPopulateCategoriesHashTables ();
    priv->pluginsToSettingsSpecialTypesHashTable = ccsMATEIntegrationPopulateSpecialTypesHashTables ();
    priv->pluginsToSettingNameMATENameHashTable = ccsMATEIntegrationPopulateSettingNameToMATENameHashTables ();
    priv->valueChangedData = valueChangedData;

    ccsObjectInit (factory, ai);
    ccsObjectSetPrivate (factory, (CCSPrivate *) priv);
    ccsObjectAddInterface (factory, (const CCSInterface *) &ccsGConfIntegratedSettingFactoryInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingFactoryInterface));

    ccsIntegratedSettingFactoryRef (factory);

    return factory;
}

