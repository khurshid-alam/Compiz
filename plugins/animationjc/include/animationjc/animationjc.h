#ifndef _COMPIZ_ANIMATIONJC_H
#define _COMPIZ_ANIMATIONJC_H

#define ANIMATIONADDON_ABI 20091206

#include <core/pluginclasshandler.h>

#include <vector>
#include <boost/ptr_container/ptr_vector.hpp>

#include <opengl/opengl.h>

using namespace::std;

class PrivateAnimJCScreen;


/// Base class for all polygon- and particle-based animations
class BaseAddonAnim :
    virtual public Animation
{
public:
    BaseAddonAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float duration,
		   const AnimEffect info,
		   const CompRect &icon);
    ~BaseAddonAnim () {}

    bool needsDepthTest () { return mDoDepthTest; }

protected:
    /// Gets info about the extension plugin that implements this animation.
    ExtensionPluginInfo *getExtensionPluginInfo ();

    CompositeScreen *mCScreen;
    GLScreen *mGScreen;

    bool mDoDepthTest;  ///< Whether depth testing should be used in the effect
};

#endif
