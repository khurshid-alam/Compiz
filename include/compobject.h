#ifndef _COMPOBJECT_H
#define _COMPOBJECT_H

#include <compiz.h>

#include <vector>
#include <boost/function.hpp>

#define COMP_OBJECT_TYPE_ALL     -1
#define COMP_OBJECT_TYPE_CORE    0
#define COMP_OBJECT_TYPE_DISPLAY 1
#define COMP_OBJECT_TYPE_SCREEN  2
#define COMP_OBJECT_TYPE_WINDOW  3
#define COMP_OBJECT_TYPE_NUM     4

#define ARRAY_SIZE(array)		 \
    (sizeof (array) / sizeof (array[0]))

#define DISPATCH_CHECK(object, dispTab, tabSize)	      \
    ((object)->objectType () < (int) (tabSize) && (dispTab)[(object)->objectType ()])

#define DISPATCH(object, dispTab, tabSize, args)   \
    if (DISPATCH_CHECK (object, dispTab, tabSize)) \
	(*(dispTab)[(object)->objectType ()]) args

#define RETURN_DISPATCH(object, dispTab, tabSize, def, args) \
    if (DISPATCH_CHECK (object, dispTab, tabSize))	     \
	return (*(dispTab)[(object)->objectType ()]) args;	     \
    else						     \
	return (def)

class PrivateObject;

class CompObject {

    public:
	typedef std::vector<bool> indices;
	typedef boost::function<bool (CompObject *)> CallBack;
	typedef int Type;

    public:
	CompObject (Type type, const char* typeName,
		    indices *iList = NULL);
	virtual ~CompObject ();

        const char *objectTypeName ();
	Type objectType ();

	void addChild (CompObject *);
	void removeFromParent ();

        bool forEachChild (CallBack proc,
			   Type     type = COMP_OBJECT_TYPE_ALL);

	virtual CompString objectName () = 0;

    public:

	std::vector<CompPrivate> privates;

    protected:
	static int allocatePrivateIndex (Type type, indices *iList);
	static void freePrivateIndex (Type type, indices *iList, int idx);

    private:
	PrivateObject *priv;
};


#endif