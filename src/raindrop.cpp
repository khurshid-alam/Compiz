#include "private.h"

#include <algorithm>

RaindropAnim::RaindropAnim (CompWindow *w,
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
RaindropAnim::initGrid ()
{
    mGridWidth = 20;
    mGridHeight = 20;
}

void
RaindropAnim::step ()
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

    /*
    float centerx = wx + mModel->scale ().x () *
	    (owidth * 0.5 - outExtents.left);
    float centery = wy + mModel->scale ().y () *
	    (oheight * 0.5 - outExtents.top);
    */

    float waveHalfWidth = 0.1; // in grid terms
    float waveAmp = (pow ((float)oheight / ::screen->height (), 0.4) * 0.08);
    float wavePosition = -waveHalfWidth +
	    (1. + 2.*waveHalfWidth) * getRaindropProgress ();

    GridModel::GridObject *object = mModel->objects ();
    unsigned int n = mModel->numObjects ();
    for (unsigned int i = 0; i < n; i++, object++)
    {
	Point3d &objPos = object->position ();

        float origx = wx + mModel->scale ().x () *
		(owidth * object->gridPosition ().x () -
		outExtents.left);
        objPos.setX (origx);

	float origy = wy + mModel->scale ().y () *
		(oheight * object->gridPosition ().y () -
		outExtents.top);
        objPos.setY (origy);

	// find distance to center in grid terms
	float gridDistance = sqrt (pow (object->gridPosition ().x ()-0.5, 2) +
			           pow (object->gridPosition ().y ()-0.5, 2)) *
			     sqrt (2);

	float distFromWaveCenter = fabs (gridDistance - wavePosition);
	if (distFromWaveCenter < waveHalfWidth)
	    objPos.setZ (waveAmp * (cos (distFromWaveCenter *
				         3.14159265 / waveHalfWidth) + 1) / 2);
	else
	    objPos.setZ (0);
    }
}
