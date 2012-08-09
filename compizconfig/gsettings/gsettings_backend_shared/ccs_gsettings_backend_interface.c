#include <ccs-backend.h>
#include "ccs_gsettings_backend_interface.h"

INTERFACE_TYPE (CCSGSettingsBackendInterface);

void
ccsGSettingsBackendRegisterGConfClient (CCSBackend *backend)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendRegisterGConfClient) (backend);
}

void
ccsGSettingsBackendUnregisterGConfClient (CCSBackend *backend)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUnregisterGConfClient) (backend);
}

const char *
ccsGSettingsBackendGetCurrentProfile (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetCurrentProfile) (backend);
}

GVariant *
ccsGSettingsBackendGetExistingProfiles (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetExistingProfiles) (backend);
}

void
ccsGSettingsBackendSetExistingProfiles (CCSBackend *backend, GVariant *value)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendSetExistingProfiles) (backend, value);
}

void
ccsGSettingsBackendSetCurrentProfile (CCSBackend *backend, const gchar *value)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendSetCurrentProfile) (backend, value);
}

GVariant *
ccsGSettingsBackendGetPluginsWithSetKeys (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetPluginsWithSetKeys) (backend);
}

void
ccsGSettingsBackendClearPluginsWithSetKeys (CCSBackend *backend)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendClearPluginsWithSetKeys) (backend);
}

void
ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (CCSBackend *backend,
						       CCSContext *context,
						       GVariant   *pluginKeys,
						       const char *profile)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUnsetAllChangedPluginKeysInProfile) (backend, context, pluginKeys, profile);
}

gboolean
ccsGSettingsBackendUpdateProfile (CCSBackend *backend, CCSContext *context)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUpdateProfile) (backend, context);
}

void
ccsGSettingsBackendUpdateCurrentProfileName (CCSBackend *backend, const char *profile)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUpdateCurrentProfileName) (backend, profile);
}

void
ccsGSettingsBackendAddProfile (CCSBackend *backend, const char *profile)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendAddProfile) (backend, profile);
}
