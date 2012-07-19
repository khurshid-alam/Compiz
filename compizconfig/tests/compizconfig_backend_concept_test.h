#ifndef _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST
#define _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST

#include <list>

#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <X11/keysym.h>

#include <ccs-backend.h>
#include <ccs.h>

#include <compizconfig_ccs_plugin_mock.h>
#include <compizconfig_ccs_setting_mock.h>
#include <compizconfig_ccs_context_mock.h>

using ::testing::Eq;
using ::testing::SetArgPointee;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;

MATCHER(IsTrue, "Is True") { if (arg) return true; else return false; }
MATCHER(IsFalse, "Is False") { if (!arg) return true; else return false; }

class ListEqualityMatcher :
    public MatcherInterface <CCSSettingValueList>
{
    public:

	ListEqualityMatcher (CCSSettingListInfo info,
			     CCSSettingValueList cmp) :
	    mInfo (info),
	    mCmp (cmp)
	{
	}

	virtual bool MatchAndExplain (CCSSettingValueList x, MatchResultListener *listener) const
	{
	    return ccsCompareLists (x, mCmp, mInfo);
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "lists are equal";
	}

	virtual void DescribeNegationTo (std::ostream *os) const
	{
	    *os << "lists are not equal";
	}

    private:

	CCSSettingListInfo mInfo;
	CCSSettingValueList mCmp;
};

Matcher<CCSSettingValueList> ListEqual (CCSSettingListInfo info,
					CCSSettingValueList cmp)
{
    return MakeMatcher (new ListEqualityMatcher (info, cmp));
}

bool
operator== (const CCSSettingColorValue &lhs,
	    const CCSSettingColorValue &rhs)
{
    if (ccsIsEqualColor (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingColorValue &v)
{
    return os << "Red: " << std::hex << v.color.red << "Blue: " << std::hex << v.color.blue << "Green: " << v.color.green << "Alpha: " << v.color.alpha
       << std::dec << std::endl;
}

bool
operator== (const CCSSettingKeyValue &lhs,
	    const CCSSettingKeyValue &rhs)
{
    if (ccsIsEqualKey (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingKeyValue &v)
{
    return os << "Keysym: " << v.keysym << " KeyModMask " << std::hex << v.keyModMask << std::dec << std::endl;
}

bool
operator== (const CCSSettingButtonValue &lhs,
	    const CCSSettingButtonValue &rhs)
{
    if (ccsIsEqualButton (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingButtonValue &v)
{
    return os << "Button " << v.button << "Button Key Mask: " << std::hex << v.buttonModMask << "Edge Mask: " << v.edgeMask << std::dec << std::endl;
}

class CharacterWrapper :
    boost::noncopyable
{
    public:

	explicit CharacterWrapper (char *c) :
	    mChar (c)
	{
	}

	~CharacterWrapper ()
	{
	    free (mChar);
	}

	operator char * ()
	{
	    return mChar;
	}

	operator const char * () const
	{
	    return mChar;
	}

    private:

	char *mChar;
};

class CCSListWrapper :
    boost::noncopyable
{
    public:

	typedef boost::shared_ptr <CCSListWrapper> Ptr;

	CCSListWrapper (CCSSettingValueList list, bool freeItems, CCSSettingType type) :
	    mList (list),
	    mFreeItems (freeItems),
	    mType (type)
	{
	}

	CCSSettingType type () { return mType; }

	operator CCSSettingValueList ()
	{
	    return mList;
	}

	operator CCSSettingValueList () const
	{
	    return mList;
	}

	~CCSListWrapper ()
	{
	    ccsSettingValueListFree (mList, mFreeItems ? TRUE : FALSE);
	}
    private:

	CCSSettingValueList mList;
	bool		    mFreeItems;
	CCSSettingType      mType;
};

typedef boost::variant <bool,
			int,
			float,
			const char *,
			CCSSettingColorValue,
			CCSSettingKeyValue,
			CCSSettingButtonValue,
			unsigned int,
			CCSListWrapper::Ptr> VariantTypes;

class CCSBackendConceptTestEnvironmentInterface
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestEnvironmentInterface> Ptr;

	virtual ~CCSBackendConceptTestEnvironmentInterface () {};
	virtual CCSBackend * SetUp (CCSContext *context,
				    CCSContextGMock *gmockContext) = 0;
	virtual void TearDown (CCSBackend *) = 0;

	virtual void PreWrite (CCSContextGMock *,
			       CCSPluginGMock  *,
			       CCSSettingGMock *,
			       CCSSettingType) = 0;
	virtual void PostWrite (CCSContextGMock *,
				CCSPluginGMock  *,
				CCSSettingGMock *,
				CCSSettingType) = 0;

	virtual void WriteBoolAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteIntegerAtKey (const std::string &plugin,
					const std::string &key,
					const VariantTypes &value) = 0;
	virtual void WriteFloatAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value) = 0;
	virtual void WriteStringAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteColorAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteKeyAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteButtonAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteEdgeAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteMatchAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value) = 0;
	virtual void WriteBellAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteListAtKey (const std::string &plugin,
				     const std::string &key,
				     const VariantTypes &value) = 0;

	virtual void PreRead (CCSContextGMock *,
			      CCSPluginGMock  *,
			      CCSSettingGMock *,
			      CCSSettingType) = 0;
	virtual void PostRead (CCSContextGMock *,
			       CCSPluginGMock  *,
			       CCSSettingGMock *,
			       CCSSettingType) = 0;

	virtual Bool ReadBoolAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key) = 0;
	virtual float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key) = 0;
	virtual const char * ReadStringAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual const char * ReadMatchAtKey (const std::string &plugin,
				      const std::string &key) = 0;
	virtual Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingValueList ReadListAtKey (const std::string &plugin,
				     const std::string &key) = 0;
};

class CCSBackendConceptTestEnvironmentFactoryInterface
{
    public:

	virtual ~CCSBackendConceptTestEnvironmentFactoryInterface () {}

	virtual CCSBackendConceptTestEnvironmentInterface::Ptr ConstructTestEnv () = 0;
};

template <typename I>
class CCSBackendConceptTestEnvironmentFactory :
    public CCSBackendConceptTestEnvironmentFactoryInterface
{
    public:

	CCSBackendConceptTestEnvironmentInterface::Ptr
	ConstructTestEnv ()
	{
	    return boost::shared_static_cast <I> (boost::make_shared <I> ());
	}
};

namespace
{

typedef boost::function <void ()> WriteFunc;

Bool boolToBool (bool v) { return v ? TRUE : FALSE; }

CCSSettingGMock * getSettingGMockFromSetting (CCSSetting *setting) { return (CCSSettingGMock *) ccsObjectGetPrivate (setting); }

void SetIntWriteExpectation (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value,
			     CCSSetting       *setting,
			     const WriteFunc &write,
			     const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getInt (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <int> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadIntegerAtKey (plugin, key), boost::get <int> (value));
}

void SetBoolWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getBool (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boolToBool (boost::get <bool> (value))),
							 Return (TRUE)));
    write ();

    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_THAT (env->ReadBoolAtKey (plugin, key), IsTrue ());
    else
	EXPECT_THAT (env->ReadBoolAtKey (plugin, key), IsFalse ());
}

void SetFloatWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       CCSSetting       *setting,
			       const WriteFunc &write,
			       const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getFloat (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <float> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadFloatAtKey (plugin, key), boost::get <float> (value));
}

void SetStringWriteExpectation (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value,
				CCSSetting       *setting,
				const WriteFunc &write,
				const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getString (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     const_cast <char *> (boost::get <const char *> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (std::string (env->ReadStringAtKey (plugin, key)), std::string (boost::get <const char *> (value)));
}

void SetColorWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       CCSSetting       *setting,
			       const WriteFunc &write,
			       const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getColor (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <CCSSettingColorValue> (value)),
							 Return (TRUE)));
    write ();

    /* Storage on most backends is lossy, so simulate this */
    CCSSettingColorValue v = boost::get <CCSSettingColorValue> (value);
    char *str = ccsColorToString (&v);

    ccsStringToColor (str, &v);

    free (str);

    EXPECT_EQ (env->ReadColorAtKey (plugin, key), v);
}

void SetKeyWriteExpectation (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value,
			     CCSSetting       *setting,
			     const WriteFunc &write,
			     const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getKey (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <CCSSettingKeyValue> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadKeyAtKey (plugin, key), boost::get <CCSSettingKeyValue> (value));
}

void SetButtonWriteExpectation (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value,
				CCSSetting       *setting,
				const WriteFunc &write,
				const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getButton (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <CCSSettingButtonValue> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadButtonAtKey (plugin, key), boost::get <CCSSettingButtonValue> (value));
}

void SetEdgeWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getEdge (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <unsigned int> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadEdgeAtKey (plugin, key), boost::get <unsigned int> (value));
}

void SetBellWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getBell (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boolToBool (boost::get <bool> (value))),
							 Return (TRUE)));
    write ();
    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_THAT (env->ReadBellAtKey (plugin, key), IsTrue ());
    else
	EXPECT_THAT (env->ReadBellAtKey (plugin, key), IsFalse ());
}

void SetMatchWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       CCSSetting       *setting,
			       const WriteFunc &write,
			       const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getMatch (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     const_cast <char *> (boost::get <const char *> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (std::string (env->ReadMatchAtKey (plugin, key)), std::string (boost::get <const char *> (value)));
}

void SetListWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    CCSSettingValueList list = *(boost::get <boost::shared_ptr <CCSListWrapper> > (value));
    boost::shared_ptr <CCSSettingInfo> info (boost::make_shared <CCSSettingInfo> ());

    info->forList.listType = (boost::get <boost::shared_ptr <CCSListWrapper> > (value))->type ();

    EXPECT_CALL (*gmock, getInfo ()).WillRepeatedly (Return (info.get ()));
    EXPECT_CALL (*gmock, getList (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     list),
							 Return (TRUE)));
    write ();
    EXPECT_THAT (env->ReadListAtKey (plugin, key), ListEqual (ccsSettingGetInfo (setting)->forList, list));
}

void SetIntReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setInt (boost::get <int> (value), _));
}

void SetBoolReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_CALL (*gmock, setBool (IsTrue (), _));
    else
	EXPECT_CALL (*gmock, setBool (IsFalse (), _));
}

void SetBellReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_CALL (*gmock, setBell (IsTrue (), _));
    else
	EXPECT_CALL (*gmock, setBell (IsFalse (), _));
}

void SetFloatReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setFloat (boost::get <float> (value), _));
}

void SetStringReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setString (Eq (std::string (boost::get <const char *> (value))), _));
}

void SetMatchReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setMatch (Eq (std::string (boost::get <const char *> (value))), _));
}

void SetColorReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    CCSSettingColorValue v = boost::get <CCSSettingColorValue> (value);
    char *str = ccsColorToString (&v);

    ccsStringToColor (str, &v);

    free (str);

    EXPECT_CALL (*gmock, setColor (v, _));
}

void SetKeyReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setKey (boost::get <CCSSettingKeyValue> (value), _));
}

void SetButtonReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setButton (boost::get <CCSSettingButtonValue> (value), _));
}

void SetEdgeReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setEdge (boost::get <unsigned int> (value), _));
}

CCSSettingInfo globalListInfo;

void SetListReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    globalListInfo.forList.listType = (boost::get <boost::shared_ptr <CCSListWrapper> > (value))->type ();
    globalListInfo.forList.listInfo = NULL;

    ON_CALL (*gmock, getInfo ()).WillByDefault (Return (&globalListInfo));
    EXPECT_CALL (*gmock, setList (ListEqual (globalListInfo.forList, *(boost::get <boost::shared_ptr <CCSListWrapper> > (value))), _));
}

}

class CCSBackendConceptTestParamInterface
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestParamInterface> Ptr;

	typedef void (CCSBackendConceptTestEnvironmentInterface::*NativeWriteMethod) (const std::string &plugin,
										      const std::string &keyname,
										      const VariantTypes &value);

	typedef boost::function <void (CCSSettingGMock *,
				       const VariantTypes &)> SetReadExpectation;
	typedef boost::function <void (const std::string &,
				       const std::string &,
				       const VariantTypes &,
				       CCSSetting        *,
				       const WriteFunc &,
				       const CCSBackendConceptTestEnvironmentInterface::Ptr & )> SetWriteExpectation;

	virtual void TearDown (CCSBackend *) = 0;

	virtual CCSBackendConceptTestEnvironmentInterface::Ptr testEnv () = 0;
	virtual VariantTypes & value () = 0;
	virtual void nativeWrite (const CCSBackendConceptTestEnvironmentInterface::Ptr &  iface,
				  const std::string &plugin,
				  const std::string &keyname,
				  const VariantTypes &value) = 0;
	virtual CCSSettingType & type () = 0;
	virtual std::string & keyname () = 0;
	virtual SetWriteExpectation & setWriteExpectationAndWrite () = 0;
	virtual SetReadExpectation & setReadExpectation () = 0;
	virtual std::string & what () = 0;
};

template <typename I>
class CCSBackendConceptTestParam :
    public CCSBackendConceptTestParamInterface
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestParam <I> > Ptr;

	CCSBackendConceptTestParam (CCSBackendConceptTestEnvironmentFactoryInterface *testEnvFactory,
				    const VariantTypes &value,
				    const NativeWriteMethod &write,
				    const CCSSettingType &type,
				    const std::string &keyname,
				    const SetReadExpectation &setReadExpectation,
				    const SetWriteExpectation &setWriteExpectation,
				    const std::string &what) :
	    mTestEnvFactory (testEnvFactory),
	    mTestEnv (),
	    mValue (value),
	    mNativeWrite (write),
	    mType (type),
	    mKeyname (keyname),
	    mSetReadExpectation (setReadExpectation),
	    mSetWriteExpectation (setWriteExpectation),
	    mWhat (what)
	{
	}

	void TearDown (CCSBackend *b)
	{
	    if (mTestEnv)
		mTestEnv->TearDown (b);

	    mTestEnv.reset ();
	}

	CCSBackendConceptTestEnvironmentInterface::Ptr testEnv ()
	{
	    if (!mTestEnv)
		mTestEnv = mTestEnvFactory->ConstructTestEnv ();

	    return mTestEnv;
	}

	VariantTypes & value () { return mValue; }
	void nativeWrite (const CCSBackendConceptTestEnvironmentInterface::Ptr &  iface,
			  const std::string &plugin,
			  const std::string &keyname,
			  const VariantTypes &value)
	{
	    return ((iface.get ())->*mNativeWrite) (plugin, keyname, value);
	}
	CCSSettingType & type () { return mType; }
	std::string & keyname () { return mKeyname; }
	CCSBackendConceptTestParamInterface::SetReadExpectation & setReadExpectation () { return mSetReadExpectation; }
	CCSBackendConceptTestParamInterface::SetWriteExpectation & setWriteExpectationAndWrite () { return mSetWriteExpectation; }
	std::string & what () { return mWhat; }

    private:

	CCSBackendConceptTestEnvironmentFactoryInterface *mTestEnvFactory;
	CCSBackendConceptTestEnvironmentInterface::Ptr mTestEnv;
	VariantTypes mValue;
	NativeWriteMethod mNativeWrite;
	CCSSettingType mType;
	std::string mKeyname;
	SetReadExpectation mSetReadExpectation;
	SetWriteExpectation mSetWriteExpectation;
	std::string mWhat;

};

class CCSBackendConformanceTest :
    public ::testing::TestWithParam <CCSBackendConceptTestParamInterface::Ptr>
{
    public:

	virtual ~CCSBackendConformanceTest () {}

	CCSBackendConformanceTest () :
	    profileName ("mock")
	{
	}

	CCSBackend * GetBackend ()
	{
	    return mBackend;
	}

	virtual void SetUp ()
	{
	    CCSBackendConformanceTest::SpawnContext (&context);
	    gmockContext = (CCSContextGMock *) ccsObjectGetPrivate (context);

	    ON_CALL (*gmockContext, getProfile ()).WillByDefault (Return (profileName.c_str ()));

	    mBackend = GetParam ()->testEnv ()->SetUp (context, gmockContext);
	}

	virtual void TearDown ()
	{
	    CCSBackendConformanceTest::GetParam ()->TearDown (mBackend);

	    for (std::list <CCSContext *>::iterator it = mSpawnedContexts.begin ();
		 it != mSpawnedContexts.end ();
		 it++)
		ccsFreeMockContext (*it);

	    for (std::list <CCSPlugin *>::iterator it = mSpawnedPlugins.begin ();
		 it != mSpawnedPlugins.end ();
		 it++)
		ccsFreeMockPlugin (*it);

	    for (std::list <CCSSetting *>::iterator it = mSpawnedSettings.begin ();
		 it != mSpawnedSettings.end ();
		 it++)
		ccsFreeMockSetting (*it);
	}

    protected:

	/* Having the returned context, setting and plugin
	 * as out params is awkward, but GTest doesn't let
	 * you use ASSERT_* unless the function returns void
	 */
	void
	SpawnContext (CCSContext **context)
	{
	    *context = ccsMockContextNew ();
	    mSpawnedContexts.push_back (*context);
	}

	void
	SpawnPlugin (const std::string &name, CCSContext *context, CCSPlugin **plugin)
	{
	    *plugin = ccsMockPluginNew ();
	    mSpawnedPlugins.push_back (*plugin);

	    CCSPluginGMock *gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (*plugin);

	    ASSERT_FALSE (name.empty ());
	    ASSERT_TRUE (context);

	    ON_CALL (*gmockPlugin, getName ()).WillByDefault (Return ((char *) name.c_str ()));
	    ON_CALL (*gmockPlugin, getContext ()).WillByDefault (Return (context));
	}

	void
	SpawnSetting (const std::string &name,
		      CCSSettingType	type,
		      CCSPlugin		*plugin,
		      CCSSetting	**setting)
	{
	    *setting = ccsMockSettingNew ();
	    mSpawnedSettings.push_back (*setting);

	    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (*setting);

	    ASSERT_FALSE (name.empty ());
	    ASSERT_NE (type, TypeNum);
	    ASSERT_TRUE (plugin);

	    ON_CALL (*gmockSetting, getName ()).WillByDefault (Return ((char *) name.c_str ()));
	    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (type));
	    ON_CALL (*gmockSetting, getParent ()).WillByDefault (Return (plugin));
	}

    protected:

	CCSContext *context;
	CCSContextGMock *gmockContext;

    private:

	std::list <CCSContext *> mSpawnedContexts;
	std::list <CCSPlugin  *> mSpawnedPlugins;
	std::list <CCSSetting *> mSpawnedSettings;

	CCSBackend *mBackend;

	std::string profileName;
};

namespace compizconfig
{
namespace test
{
namespace impl
{

Bool boolValues[] = { TRUE, FALSE, TRUE };
int intValues[] = { 1, 2, 3 };
float floatValues[] = { 1.0, 2.0, 3.0 };
const char * stringValues[] = { "foo", "grill", "bar" };

unsigned short max = std::numeric_limits <unsigned short>::max ();
unsigned short maxD2 = max / 2;
unsigned short maxD4 = max / 4;
unsigned short maxD8 = max / 8;

CCSSettingColorValue colorValues[3] = { { .color = { maxD2 , maxD4, maxD8, max } },
					{ .color = { maxD8, maxD4, maxD2, max } },
					{ .color = { max, maxD4, maxD2,  maxD8 } }
				      };

CCSSettingKeyValue keyValue = { XK_A,
				(1 << 0)};

CCSSettingButtonValue buttonValue = { 1,
				      (1 << 0),
				      (1 << 1) };
}

template <typename I>
::testing::internal::ParamGenerator<typename CCSBackendConceptTestParamInterface::Ptr>
GenerateTestingParametersForBackendInterface ()
{
    static CCSBackendConceptTestEnvironmentFactory <I> interfaceFactory;
    static CCSBackendConceptTestEnvironmentFactoryInterface *backendEnvFactory = &interfaceFactory;

    typedef CCSBackendConceptTestParam<I> ConceptParam;

    /* Make these all method pointers and do the bind in the tests themselves */

    static typename CCSBackendConceptTestParamInterface::Ptr testParam[] =
    {
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (1),
					   &CCSBackendConceptTestEnvironmentInterface::WriteIntegerAtKey,
					   TypeInt,
					   "integer_setting",
					   boost::bind (SetIntReadExpectation, _1, _2),
					   boost::bind (SetIntWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestInt"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (true),
					   &CCSBackendConceptTestEnvironmentInterface::WriteBoolAtKey,
					   TypeBool,
					   "boolean_setting",
					   boost::bind (SetBoolReadExpectation, _1, _2),
					   boost::bind (SetBoolWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestBool"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <float> (3.0)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteFloatAtKey,
					   TypeFloat,
					   "float_setting",
					   boost::bind (SetFloatReadExpectation, _1, _2),
					   boost::bind (SetFloatWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestFloat"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <const char *> ("foo")),
					   &CCSBackendConceptTestEnvironmentInterface::WriteStringAtKey,
					   TypeString,
					   "string_setting",
					   boost::bind (SetStringReadExpectation, _1, _2),
					   boost::bind (SetStringWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestString"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <const char *> ("foo=bar")),
					   &CCSBackendConceptTestEnvironmentInterface::WriteMatchAtKey,
					   TypeMatch,
					   "match_setting",
					   boost::bind (SetMatchReadExpectation, _1, _2),
					   boost::bind (SetMatchWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestMatch"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (true),
					   &CCSBackendConceptTestEnvironmentInterface::WriteBellAtKey,
					   TypeBell,
					   "bell_setting",
					   boost::bind (SetBellReadExpectation, _1, _2),
					   boost::bind (SetBellWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestBell"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (impl::colorValues[0]),
					   &CCSBackendConceptTestEnvironmentInterface::WriteColorAtKey,
					   TypeColor,
					   "color_setting",
					   boost::bind (SetColorReadExpectation, _1, _2),
					   boost::bind (SetColorWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestColor"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (impl::keyValue),
					   &CCSBackendConceptTestEnvironmentInterface::WriteKeyAtKey,
					   TypeKey,
					   "key_setting",
					   boost::bind (SetKeyReadExpectation, _1, _2),
					   boost::bind (SetKeyWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestKey"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (impl::buttonValue),
					   &CCSBackendConceptTestEnvironmentInterface::WriteButtonAtKey,
					   TypeButton,
					   "button_setting",
					   boost::bind (SetButtonReadExpectation, _1, _2),
					   boost::bind (SetButtonWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestButton"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <unsigned int> (1)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteEdgeAtKey,
					   TypeEdge,
					   "edge_setting",
					   boost::bind (SetEdgeReadExpectation, _1, _2),
					   boost::bind (SetEdgeWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestEdge"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromIntArray (impl::intValues,
															   sizeof (impl::intValues) / sizeof (impl::intValues[0]),
															   NULL), false, TypeInt)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "int_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListInt"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromFloatArray (impl::floatValues,
															     sizeof (impl::floatValues) / sizeof (impl::floatValues[0]),
															     NULL), false, TypeFloat)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "float_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListInt"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromBoolArray (impl::boolValues,
															   sizeof (impl::boolValues) / sizeof (impl::boolValues[0]),
															   NULL), false, TypeBool)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "bool_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListBool"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromStringArray (impl::stringValues,
															      sizeof (impl::stringValues) / sizeof (impl::stringValues[0]),
															      NULL), false, TypeString)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "string_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListString"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromColorArray (impl::colorValues,
															     sizeof (impl::colorValues) / sizeof (impl::colorValues[0]),
															     NULL), false, TypeColor)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "color_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListColor")
    };

    return ::testing::ValuesIn (testParam);
}
}
}

class CCSBackendConformanceTestReadWrite :
    public CCSBackendConformanceTest
{
    public:

	virtual ~CCSBackendConformanceTestReadWrite () {}

	virtual void SetUp ()
	{
	    CCSBackendConformanceTest::SetUp ();

	    pluginName = "mock";
	    settingName = GetParam ()->keyname ();
	    VALUE = GetParam ()->value ();

	    CCSBackendConformanceTest::SpawnPlugin (pluginName, context, &plugin);
	    CCSBackendConformanceTest::SpawnSetting (settingName, GetParam ()->type (), plugin, &setting);

	    gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (plugin);
	    gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);
	}

	virtual void TearDown ()
	{
	    CCSBackendConformanceTest::TearDown ();
	}

    protected:

	std::string pluginName;
	std::string settingName;
	VariantTypes VALUE;
	CCSPlugin *plugin;
	CCSSetting *setting;
	CCSPluginGMock  *gmockPlugin;
	CCSSettingGMock *gmockSetting;

};

TEST_P (CCSBackendConformanceTestReadWrite, TestReadValue)
{
    SCOPED_TRACE (CCSBackendConformanceTest::GetParam ()->what () + "Read");

    CCSBackendConformanceTest::GetParam ()->testEnv ()->PreRead (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTest::GetParam ()->nativeWrite (CCSBackendConformanceTest::GetParam ()->testEnv (),
							 pluginName, settingName, VALUE);
    CCSBackendConformanceTest::GetParam ()->testEnv ()->PostRead (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTest::GetParam ()->setReadExpectation () (gmockSetting, VALUE);

    ccsBackendReadSetting (CCSBackendConformanceTest::GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTestReadWrite, TestWriteValue)
{
    SCOPED_TRACE (CCSBackendConformanceTest::GetParam ()->what () + "Write");

    CCSBackendConformanceTest::GetParam ()->testEnv ()->PreWrite (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTest::GetParam ()->setWriteExpectationAndWrite () (pluginName,
									   settingName,
									   VALUE,
									   setting,
									   boost::bind (ccsBackendWriteSetting,
											CCSBackendConformanceTest::GetBackend (),
											context,
										       setting),
									   GetParam ()->testEnv ());
    CCSBackendConformanceTest::GetParam ()->testEnv ()->PostWrite (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());

}


#endif

