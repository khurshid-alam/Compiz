#include "private.h"

#include <algorithm>

BlackHoleAnim::BlackHoleAnim (CompWindow *w,
			      WindowEvent curWindowEvent,
			      float duration,
			      const AnimEffect info,
			      const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    GridTransformAnim::GridTransformAnim (w, curWindowEvent, duration, info,
                                          icon)
{
}

void
BlackHoleAnim::initGrid ()
{
    mGridWidth = 10;
    mGridHeight = 10;
}

void
BlackHoleAnim::step ()
{
    CompRect winRect (mAWindow->savedRectsValid () ?
                      mAWindow->saveWinRect () :
                      mWindow->geometry ());
    CompRect outRect (mAWindow->savedRectsValid () ?
                      mAWindow->savedOutRect () :
                      mWindow->outputRect ());
    CompWindowExtents outExtents (mAWindow->savedRectsValid () ?
			          mAWindow->savedOutExtents () :
			          mWindow->output ());

    int wx = winRect.x ();
    int wy = winRect.y ();

    int owidth = outRect.width ();
    int oheight = outRect.height ();

    float centerx = wx + mModel->scale ().x () *
	    (owidth * 0.5 - outExtents.left);
    float centery = wy + mModel->scale ().y () *
	    (oheight * 0.5 - outExtents.top);

    GridModel::GridObject *object = mModel->objects ();
    unsigned int n = mModel->numObjects ();
    for (unsigned int i = 0; i < n; i++, object++)
    {
	// find distance to center in grid terms, 0..1
	float gridDistance = 2 * max (fabs (object->gridPosition ().x ()-0.5),
				      fabs (object->gridPosition ().y ()-0.5));

	// use that to find tau
	//float tau = pow (gridDistance, 2) / 4.;
	float tau = 1./8;

	// use that and t to find r
	//float r = exp (-getBlackHoleProgress () / tau);
	float cutoff = gridDistance/2;
	float r = 1;
	if (getBlackHoleProgress () > cutoff)
	{
	    r = exp (-(getBlackHoleProgress () - cutoff) / tau);
	}

	// find real original coordinates
	float origx = wx + mModel->scale ().x () *
		(owidth * object->gridPosition ().x () -
		outExtents.left);
	float origy = wy + mModel->scale ().y () *
                (oheight * object->gridPosition ().y () -
                outExtents.top);

	// normalize, multiply by r
	Point3d &objPos = object->position ();
	objPos.setX ((origx-centerx) * r + centerx);
	objPos.setY ((origy-centery) * r + centery);
	//objPos.setX (origx);
	//objPos.setY (origy);
	objPos.setZ (0);
    }
}
