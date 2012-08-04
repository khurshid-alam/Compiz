#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <glib-object.h>

#include "test_gsettings_tests.h"
#include <gsettings-mock-schemas-config.h>
#include <ccs_gsettings_interface_wrapper.h>

using ::testing::NotNull;

class TestGSettingsWrapperWithMemoryBackendEnv :
    public ::testing::Test
{
    public:

	TestGSettingsWrapperWithMemoryBackendEnv () :
	    mockSchema ("org.compiz.mock"),
	    mockPath ("/org/compiz/mock/mock")
	{
	}

	virtual CCSObjectAllocationInterface * GetAllocator () = 0;

	virtual void SetUp ()
	{
	    g_setenv ("G_SLICE", "always-malloc", 1);
	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);
	    g_setenv ("GSETTINGS_BACKEND", "memory", 1);

	    g_type_init ();
	}

	virtual void TearDown ()
	{
	    g_unsetenv ("GSETTINGS_BACKEND");
	    g_unsetenv ("GSETTINGS_SCHEMA_DIR");
	    g_unsetenv ("G_SLICE");
	}

    protected:

	std::string mockSchema;
	std::string mockPath;
	boost::shared_ptr <CCSGSettingsWrapper> wrapper;
	GSettings   *settings;
};

class TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator :
    public TestGSettingsWrapperWithMemoryBackendEnv
{
    protected:

	CCSObjectAllocationInterface * GetAllocator ()
	{
	    return &ccsDefaultObjectAllocator;
	}
};

class TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit :
    public TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator
{
    public:

	virtual void SetUp ()
	{
	    TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator::SetUp ();

	    wrapper.reset (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
								    mockPath.c_str (),
								    GetAllocator ()),
			   boost::bind (ccsFreeGSettingsWrapper, _1));

	    ASSERT_THAT (wrapper.get (), NotNull ());

	    settings = ccsGSettingsWrapperGetGSettings (wrapper.get ());

	    ASSERT_THAT (settings, NotNull ());
	}
};

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator, TestWrapperConstruction)
{
    boost::shared_ptr <CCSGSettingsWrapper> wrapper (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
											      mockPath.c_str (),
											      &ccsDefaultObjectAllocator),
						     boost::bind (ccsFreeGSettingsWrapper, _1));

    EXPECT_THAT (wrapper.get (), NotNull ());
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator, TestGetGSettingsWrapper)
{
    boost::shared_ptr <CCSGSettingsWrapper> wrapper (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
											      mockPath.c_str (),
											      &ccsDefaultObjectAllocator),
						     boost::bind (ccsFreeGSettingsWrapper, _1));

    ASSERT_THAT (wrapper.get (), NotNull ());
    EXPECT_THAT (ccsGSettingsWrapperGetGSettings (wrapper.get ()), NotNull ());
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestSetValueOnWrapper)
{
    const unsigned int VALUE = 2;
    const std::string KEY ("integer-setting");
    boost::shared_ptr <GVariant> variant (g_variant_new ("i", VALUE, NULL),
					  boost::bind (g_variant_unref, _1));
    ccsGSettingsWrapperSetValue (wrapper.get (), KEY.c_str (), variant.get ());

    boost::shared_ptr <GVariant> value (g_settings_get_value (settings, KEY.c_str ()),
					boost::bind (g_variant_unref, _1));

    int v = g_variant_get_int32 (value.get ());
    EXPECT_EQ (VALUE, v);
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestGetValueOnWrapper)
{
    const double VALUE = 3.0;
    const std::string KEY ("float-setting");
    boost::shared_ptr <GVariant> variant (g_variant_new ("i", VALUE, NULL),
					  boost::bind (g_variant_unref, _1));
    g_settings_set_value (settings, KEY.c_str (), variant.get ());
    boost::shared_ptr <GVariant> value (ccsGSettingsWrapperGetValue (wrapper.get (),
								     KEY.c_str ()),
					boost::bind (g_variant_unref, _1));

    double v = (double) g_variant_get_double (value.get ());
    EXPECT_EQ (VALUE, v);
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestResetKeyOnWrapper)
{
    FAIL ();
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestListKeysOnWrapper)
{
    FAIL ();
}
