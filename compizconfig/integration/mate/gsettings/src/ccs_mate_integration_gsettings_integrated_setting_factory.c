#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include <ccs_gsettings_interface.h>
#include <ccs_gsettings_interface_wrapper.h>

#include <gsettings_util.h>
#include <ccs_gsettings_wrapper_factory_interface.h>

#include "ccs_mate_integration.h"
#include "ccs_mate_integrated_setting.h"
#include "ccs_mate_integration_constants.h"
#include "ccs_mate_integration_types.h"
#include "ccs_mate_integration_gsettings_integrated_setting.h"
#include "ccs_mate_integration_gsettings_integrated_setting_factory.h"

char *
ccsGSettingsIntegratedSettingsTranslateNewMATEKeyForCCS (const char *key)
{
    char *newKey = translateKeyForCCS (key);

    if (g_strcmp0 (newKey, "screenshot") == 0)
    {
	free (newKey);
	newKey = strdup ("run_command_screenshot");
    }
    else if (g_strcmp0 (newKey, "window_screenshot") == 0)
    {
	free (newKey);
	newKey = strdup ("run_command_window_screenshot");
    }
    else if (g_strcmp0 (newKey, "terminal") == 0)
    {
	free (newKey);
	newKey = strdup ("run_command_terminal");
    }

    return newKey;
}

typedef struct _CCSGSettingsIntegratedSettingFactoryPrivate CCSGSettingsIntegratedSettingFactoryPrivate;

struct _CCSGSettingsIntegratedSettingFactoryPrivate
{
    CCSGSettingsWrapperFactory *wrapperFactory;
    GHashTable  *pluginsToSettingsGSettingsWrapperQuarksHashTable;
    GHashTable  *quarksToGSettingsWrappersHashTable;
    GHashTable  *pluginsToSettingsSpecialTypesHashTable;
    GHashTable  *pluginsToSettingNameMATENameHashTable;
    CCSMATEValueChangeData *valueChangeData;
};

static void
mateGSettingsValueChanged (GSettings *settings,
			    gchar     *key,
			    gpointer  user_data)
{
    CCSMATEValueChangeData *data = (CCSMATEValueChangeData *) user_data;
    char *baseName = ccsGSettingsIntegratedSettingsTranslateNewMATEKeyForCCS (key);

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

GCallback
ccsGSettingsIntegratedSettingsChangeCallback ()
{
    return (GCallback) mateGSettingsValueChanged;
}

static CCSIntegratedSetting *
createNewGSettingsIntegratedSetting (CCSGSettingsWrapper *wrapper,
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

    CCSIntegratedSetting *gsettingsIntegratedSetting = ccsGSettingsIntegratedSettingNew (mateIntegratedSettingInfo, wrapper, ai);

    if (!gsettingsIntegratedSetting)
    {
	ccsIntegratedSettingInfoUnref ((CCSIntegratedSettingInfo *) mateIntegratedSettingInfo);
	return NULL;
    }

    return gsettingsIntegratedSetting;
}

static void
ccsGSettingsWrapperUnrefWrapper (gpointer wrapper)
{
    ccsGSettingsWrapperUnref ((CCSGSettingsWrapper *) wrapper);
}

static GHashTable *
initializeGSettingsWrappers (CCSGSettingsWrapperFactory *factory)
{
    const CCSGSettingsWrapperIntegratedSchemasQuarks *quarks = ccsMATEGSettingsWrapperQuarks ();
    GHashTable *quarksToGSettingsWrappers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, ccsGSettingsWrapperUnrefWrapper);

    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_COMPIZ_INTEGRATED),
			 ccsGSettingsWrapperFactoryNewGSettingsWrapper (factory,
									g_quark_to_string (quarks->ORG_COMPIZ_INTEGRATED),
									factory->object.object_allocation));
    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_MATE_DESKTOP_WM_KEYBINDINGS),
			 ccsGSettingsWrapperFactoryNewGSettingsWrapper (factory,
									g_quark_to_string (quarks->ORG_MATE_DESKTOP_WM_KEYBINDINGS),
									factory->object.object_allocation));
    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_MATE_DESKTOP_WM_PREFERENCES),
			 ccsGSettingsWrapperFactoryNewGSettingsWrapper (factory,
									g_quark_to_string (quarks->ORG_MATE_DESKTOP_WM_PREFERENCES),
									factory->object.object_allocation));
    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_MATE_DESKTOP_DEFAULT_APPLICATIONS_TERMINAL),
			 ccsGSettingsWrapperFactoryNewGSettingsWrapper (factory,
									g_quark_to_string (quarks->ORG_MATE_DESKTOP_DEFAULT_APPLICATIONS_TERMINAL),
									factory->object.object_allocation));
    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_MATE_SETTINGS_DAEMON_PLUGINS_MEDIA_KEYS),
			 ccsGSettingsWrapperFactoryNewGSettingsWrapper (factory,
									g_quark_to_string (quarks->ORG_MATE_SETTINGS_DAEMON_PLUGINS_MEDIA_KEYS),
									factory->object.object_allocation));

    return quarksToGSettingsWrappers;
}

CCSIntegratedSetting *
ccsGSettingsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (CCSIntegratedSettingFactory *factory,
										     CCSIntegration              *integration,
										     const char                  *pluginName,
										     const char                  *settingName,
										     CCSSettingType              type)
{
    CCSGSettingsIntegratedSettingFactoryPrivate *priv = (CCSGSettingsIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    GHashTable                              *settingsGSettingsWrapperQuarksHashTable = g_hash_table_lookup (priv->pluginsToSettingsGSettingsWrapperQuarksHashTable, pluginName);
    GHashTable                              *settingsSpecialTypesHashTable = g_hash_table_lookup (priv->pluginsToSettingsSpecialTypesHashTable, pluginName);
    GHashTable				    *settingsSettingNameMATENameHashTable = g_hash_table_lookup (priv->pluginsToSettingNameMATENameHashTable, pluginName);

    if (!priv->quarksToGSettingsWrappersHashTable)
	priv->quarksToGSettingsWrappersHashTable = initializeGSettingsWrappers (priv->wrapperFactory);

    if (settingsGSettingsWrapperQuarksHashTable &&
	settingsSpecialTypesHashTable &&
	settingsSettingNameMATENameHashTable)
    {
	GQuark  wrapperQuark = GPOINTER_TO_INT (g_hash_table_lookup (settingsGSettingsWrapperQuarksHashTable, settingName));
	CCSGSettingsWrapper *wrapper = g_hash_table_lookup (priv->quarksToGSettingsWrappersHashTable, GINT_TO_POINTER (wrapperQuark));
	SpecialOptionType specialType = (SpecialOptionType) GPOINTER_TO_INT (g_hash_table_lookup (settingsSpecialTypesHashTable, settingName));
	const gchar *integratedName = g_hash_table_lookup (settingsSettingNameMATENameHashTable, settingName);

	if (wrapper == NULL)
	    return NULL;

	return createNewGSettingsIntegratedSetting (wrapper,
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
ccsGSettingsIntegratedSettingFactoryFree (CCSIntegratedSettingFactory *factory)
{
    CCSGSettingsIntegratedSettingFactoryPrivate *priv = (CCSGSettingsIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);

    if (priv->pluginsToSettingsGSettingsWrapperQuarksHashTable)
	g_hash_table_unref (priv->pluginsToSettingsGSettingsWrapperQuarksHashTable);

    if (priv->quarksToGSettingsWrappersHashTable)
	g_hash_table_unref (priv->quarksToGSettingsWrappersHashTable);

    if (priv->pluginsToSettingsSpecialTypesHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSpecialTypesHashTable);

    if (priv->pluginsToSettingNameMATENameHashTable)
	g_hash_table_unref (priv->pluginsToSettingNameMATENameHashTable);

    ccsGSettingsWrapperFactoryUnref (priv->wrapperFactory);

    ccsObjectFinalize (factory);
    (*factory->object.object_allocation->free_) (factory->object.object_allocation->allocator, factory);
}

const CCSIntegratedSettingFactoryInterface ccsGSettingsIntegratedSettingFactoryInterface =
{
    ccsGSettingsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType,
    ccsGSettingsIntegratedSettingFactoryFree
};

CCSIntegratedSettingFactory *
ccsGSettingsIntegratedSettingFactoryNew (CCSGSettingsWrapperFactory   *wrapperFactory,
					 CCSMATEValueChangeData      *valueChangeData,
					 CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSettingFactory *factory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSettingFactory));

    if (!factory)
	return NULL;

    CCSGSettingsIntegratedSettingFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGSettingsIntegratedSettingFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, factory);
	return NULL;
    }

    ccsGSettingsWrapperFactoryRef (wrapperFactory);

    priv->wrapperFactory = wrapperFactory;
    priv->pluginsToSettingsGSettingsWrapperQuarksHashTable = ccsMATEGSettingsIntegrationPopulateSettingNameToIntegratedSchemasQuarksHashTable ();
    priv->pluginsToSettingsSpecialTypesHashTable = ccsMATEIntegrationPopulateSpecialTypesHashTables ();
    priv->pluginsToSettingNameMATENameHashTable = ccsMATEIntegrationPopulateSettingNameToMATENameHashTables ();
    priv->valueChangeData = valueChangeData;

    ccsObjectInit (factory, ai);
    ccsObjectSetPrivate (factory, (CCSPrivate *) priv);
    ccsObjectAddInterface (factory, (const CCSInterface *) &ccsGSettingsIntegratedSettingFactoryInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingFactoryInterface));

    ccsObjectRef (factory);

    return factory;
}

