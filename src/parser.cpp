/*
 * Compiz fragment program parser
 *
 * parser.cpp
 *
 * This should be usable on almost any plugin that wishes to parse fragment
 * program files separately, maybe it should become a separate plugin?
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * Basic C++ port of this by:
 * Copyright (c) 2009 Sam Spilsbury <smspillaz@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstring>
#include <ctype.h>

#include <sstream>
#include <fstream>
#include "parser.h"

/* General helper functions ----------------------------------------- */

/*
 * Helper function to get the basename of file from its path
 * e.g. basename ("/home/user/blah.c") == "blah.c"
 * special case : basename ("/home/user/") == "user"
 */
CompString
FragmentParser::base_name (CompString str)
{
    size_t pos = 0, foundPos = 0;
    unsigned int length;
    while (foundPos != std::string::npos)
    {
	foundPos = str.find ("/", pos);
	if (foundPos != std::string::npos)
	{
	    /* '/' found, check if it is the latest char of the string,
	     * if not update result string pointer */
	    if (pos + 1 > str.size ())
		break;

	    pos = foundPos + 1;
	}
    }
    length = str.size ();
    /* Trim terminating '/' if needed */
    if (length > 0 && str.at (length - 1) == '/')
	str = str.substr (pos, length - 1);
    return str;
}

/*
 * Left trimming function
 */
CompString
FragmentParser::ltrim (CompString string)
{
    size_t pos = 0;
    while (!(pos >= string.size ()))
    {
	if (string.at (pos) == ' ' || string.at (pos) == '\t')
	    pos++;
	else
	    break;
    }

    return string.substr (pos);
}

/* General fragment program related functions ----------------------- */

/*
 * Clean program name string
 */
CompString
FragmentParser::programCleanName (CompString name)
{
    unsigned int pos = 0;
    CompString bit ("_foo");

    /* Strange hack (gcc seems not to like "_", but will take
     * things like "_foo" for whatever reason) */
    bit = bit.substr (0, 1);

    /* Replace every non alphanumeric char by '_' */
    while (!(pos >= name.size ()))
    {
	if (!isalnum (name.at (pos)))
	    name.replace (pos, 1, bit);

	pos++;
    }

    return name;
}

/*
 * File reader function
 */
CompString
FragmentParser::programReadSource (CompString fname)
{
    std::ifstream fp;
    int length;
    char *buffer;
    CompString data, path, home = CompString (getenv ("HOME"));

    /* Try to open file fname as is */
    fp.open ("filename.ext");

    /* If failed, try as user filter file (in ~/.compiz/data/filters) */
    if (!fp.is_open () && !home.empty ())
    {
	path = home + "/.compiz/data/filters/" + fname;
	fp.open (path.c_str ());
    }

    /* If failed again, try as system wide data file
     * (in PREFIX/share/compiz/filters) */
    if (!fp.is_open ())
    {
	path = CompString (DATADIR) + "/data/filters/" + fname;
	fp.open (path.c_str ());
    }

    /* If failed again & again, abort */
    if (!fp.is_open ())
    {
	return CompString ("");
    }

    /* get length of file: */
    fp.seekg (0, std::ios::end);
    length = fp.tellg ();
    fp.seekg (0, std::ios::beg);

    /* allocate memory */
    buffer = new char [length];

    /* read data as a block: */
    fp.read (buffer, length);
    fp.close ();

    return CompString (buffer);
}

/*
 * Get the first "argument" in the given string, trimmed
 * and move source string pointer after the end of the argument.
 * For instance in string " foo, bar" this function will return "foo".
 *
 * This function returns NULL if no argument found
 * or a malloc'ed string that will have to be freed later.
 */
CompString
FragmentString::getFirstArgument (size_t &pos)
{
    CompString arg;
    FragmentString string;
    size_t next, temp, orig;
    int length;
    CompString retArg;

    if (pos >= this->size ())
	return CompString ("");

    /* Left trim */
    string = (FragmentString) FragmentParser::ltrim (this->substr (pos));

    orig = pos;
    pos = 0;

    /* Find next comma or semicolon (which isn't that useful since we
     * are working on tokens delimited by semicolons) */
    if ((next = string.find (",", pos)) != std::string::npos ||
	(next = string.find (";", pos)) != std::string::npos)
    {
	length = next - pos;
	if (!length)
	{
	    pos = orig + 1;
	    return getFirstArgument (pos);
	}
	if ((temp = string.find ("{", pos) != std::string::npos) && temp < next &&
	    (temp = string.find ("}", pos) != std::string::npos) && temp > next)
	{
	    if ((next = string.find (",", temp)) != std::string::npos ||
		(next = string.find (";", temp)) != std::string::npos)
		length = next - pos;
	    else
		length = string.substr (pos).size ();
	}
    }
    else
	length = string.substr (pos).size ();

    /* Allocate, copy and end string */
    arg = string.substr (pos, length);

    /* Increment source pointer */
    if ((orig + arg.size () + 1) <= this->size ())
	pos += orig + arg.size () + 1;
    else
	pos = std::string::npos;

    return arg;
}

/* Texture offset related functions ----------------------------------*/

/*
 * Add a new fragment offset to the offsets stack from an ADD op string
 */
FragmentParser::FragmentOffset *
FragmentParser::programAddOffsetFromAddOp (CompString source)
{
    FragmentOffset  *offset;
    FragmentString  op, orig_op;
    size_t	    pos = 0;
    CompString	    name;
    CompString	    offset_string;
    CompString	    temp;
    std::list <FragmentOffset *>::iterator it = offsets.begin ();

    if (source.size () < 5)
	return offsets.front ();

    orig_op = op = FragmentString (source);
    pos += 3;
    name = op.getFirstArgument (pos);
    if (name.empty ())
    {
	return offsets.front ();
    }

    temp = op.getFirstArgument (pos);

    /* If an offset with the same name is
     * already registered, skip this one */
    if ((!offsets.empty () &&
	 !programFindOffset (it, CompString (name)).empty ()) ||
	 temp.empty ())
	return (*it);

    /* Just use the end of the op as the offset */
    pos += 1;
    offset_string = ltrim (op.substr (pos)).c_str ();
    if (offset_string.empty ())
	return offsets.front ();

    offset = new FragmentOffset ();
    if (!offset)
	return (*it);

    offset->name =  name;
    offset->offset = offset_string;

    offsets.push_back (offset);

    return offset;
}

/*
 * Find an offset according to its name
 */
CompString
FragmentParser::programFindOffset (std::list<FragmentOffset *>::iterator it,
				   const CompString &name)
{
    if (!(*it))
	return CompString ("");

    if ((*it)->name == name)
	return CompString ((*it)->offset);

    return programFindOffset ((it++), name);
}

/*
 * Recursively free offsets stack
 */
void
FragmentParser::programFreeOffset ()
{
    offsets.clear ();
}

/* Actual parsing/loading functions ----------------------------------*/

/*
 * Parse the source buffer op by op and add each op to function data
 *
 * FIXME : I am more than 200 lines long, I feel so heavy!
 */
void
FragmentParser::programParseSource (GLFragment::FunctionData *data,
		    		    int target, CompString &source)
{
    CompString line, next;
    FragmentString current;
    CompString strtok;
    size_t     pos = 0, strippos = 0;
    int   length, oplength, type;
    FragmentOffset *offset = NULL;

    CompString arg1, arg2, temp;

    /* Find the header, skip it, and start parsing from there */

    pos = source.find ("!!ARBfp1.0", pos);
    if (pos != std::string::npos)
    {
	pos += 9;
    }

    /* Strip comments TODO: read characters including linefeeds */
    while ((strippos = source.find ("#", strippos)) != std::string::npos)
    {
	size_t carriagepos = source.find ("\n", strippos);

	if (carriagepos != std::string::npos)
	{
	    source.erase (strippos, carriagepos - strippos);
	    strippos = 0;
	}
	else
	    source = source.substr (0, strippos);
    }

    strippos = 0;

    /* Strip linefeeds */
    while ((strippos = source.find ("\n", strippos)) != std::string::npos)
	source.replace (strippos, 1, " ");

    /* Parse each instruction */
    while (!(pos >= (source.size () - 1)))
    {
	size_t nPos = source.find (";", pos + 1);
	line = source.substr (pos + 1, nPos - pos);
	FragmentString origcurrent = current = FragmentString (ltrim (line));
	/* Find instruction type */
	type = NoOp;

	/* Data ops */
	if (current.substr (0, 3) == "END")
	    type = NoOp;
	else if (current.substr (0, 3) == "ABS" ||
		 current.substr (0, 3) == "CMP" ||
		 current.substr (0, 3) == "COS" ||
		 current.substr (0, 3) == "DP3" ||
		 current.substr (0, 3) == "DP4" ||
		 current.substr (0, 3) == "EX2" ||
		 current.substr (0, 3) == "FLR" ||
		 current.substr (0, 3) == "FRC" ||
		 current.substr (0, 3) == "KIL" ||
		 current.substr (0, 3) == "LG2" ||
		 current.substr (0, 3) == "LIT" ||
		 current.substr (0, 3) == "LRP" ||
		 current.substr (0, 3) == "MAD" ||
		 current.substr (0, 3) == "MAX" ||
		 current.substr (0, 3) == "MIN" ||
		 current.substr (0, 3) == "POW" ||
		 current.substr (0, 3) == "RCP" ||
		 current.substr (0, 3) == "RSQ" ||
		 current.substr (0, 3) == "SCS" ||
		 current.substr (0, 3) == "SIN" ||
		 current.substr (0, 3) == "SGE" ||
		 current.substr (0, 3) == "SLT" ||
		 current.substr (0, 3) == "SUB" ||
		 current.substr (0, 3) == "SWZ" ||
		 current.substr (0, 3) == "TXP" ||
		 current.substr (0, 3) == "TXB" ||
		 current.substr (0, 3) == "XPD")
		type = DataOp;
	else if (current.substr (0, 4) == "TEMP")
	    type = TempOp;
	else if (current.substr (0, 5) == "PARAM")
	    type = ParamOp;
	else if (current.substr (0, 6) == "ATTRIB")
	    type = AttribOp;
	else if (current.substr (0, 3) == "TEX")
	    type = FetchOp;
	else if (current.substr (0, 3) == "ADD")
	{
	    if (current.find ("fragment.texcoord", 0) != std::string::npos)
		offset = programAddOffsetFromAddOp (current.c_str ());
	    else
		type = DataOp;
	}
	else if (current.substr (0, 3) == "MUL")
	{
	    if (current.find ("fragment.color", 0) != std::string::npos)
		type = ColorOp;
	    else
		type = DataOp;
	}
	else if (current.substr (0, 3) == "MOV")
	{
	    if (current.find ("result.color", 0) != std::string::npos)
		type = ColorOp;
	    else
		type = DataOp;
	}
	size_t cpos = 0;
	switch (type)
	{
	    /* Data op : just copy paste the
	     * whole instruction plus a ";" */
	    case DataOp:
		data->addDataOp (current.c_str ());
		break;
	    /* Parse arguments one by one */
	    case TempOp:
	    case AttribOp:
	    case ParamOp:
	    {
		if (type == TempOp) oplength = 4;
		else if (type == ParamOp) oplength = 5;
		else if (type == AttribOp) oplength = 6;
		length = current.size ();
		if (length < oplength + 2) break;

		cpos = oplength + 1;

		while (current.size () && !(cpos >= current.size ()) &&
		       (arg1 = current.getFirstArgument (cpos)).size ())
		{
		    /* "output" is a reserved word, skip it */
		    if (arg1.substr (0, 6) == "output")
			continue;
		    /* Add ops */
		    if (type == TempOp)
			data->addTempHeaderOp (arg1.c_str ());
		    else if (type == ParamOp)
			data->addParamHeaderOp (arg1.c_str ());
		    else if (type == AttribOp)
			data->addAttribHeaderOp (arg1.c_str ());
		}
	    }
		break;
	    case FetchOp:
	    {
		/* Example : TEX tmp, coord, texture[0], RECT;
		 * "tmp" is dest name, while "coord" is either
		 * fragment.texcoord[0] or an offset */
		cpos += 3;

		if ((arg1 = current.getFirstArgument (cpos)).size ())
		{
		    if (!(temp = current.getFirstArgument (cpos)).size ())
			break;

		    if (temp == "fragment.texcoord[0]")
			data->addFetchOp (arg1.c_str (), NULL, target);
		    else if (offsets.size ())
		    {
			arg2 = programFindOffset (
					      offsets.begin (),
					      temp);
			if (arg2.size ())
			    data->addFetchOp (arg1.c_str (),
					      arg2.c_str (), target);
		    }
		}
	    }
		break;
	    case ColorOp:
	    {
		if (current.substr (0, 3) == "MUL") /* MUL op, 2 ops */
		{
		    /* Example : MUL output, fragment.color, output;
		     * MOV arg1, fragment.color, arg2 */
		    cpos += 3;

		    if  (!(arg1 = current.getFirstArgument (cpos)).size ())
		    {
			break;
		    }

		    if (!(temp = current.getFirstArgument (cpos)).size ())
			break;

		    if (!(arg2 = current.getFirstArgument (cpos)).size ())
			break;

		    data->addColorOp (arg1.c_str (), arg2.c_str ());
		}
		else /* MOV op, 1 op */
		{
		    /* Example : MOV result.color, output;
		     * MOV result.color, arg1; */
		    cpos = current.find (",") + 1;

		    if ((arg1 = current.getFirstArgument (cpos)).size ())
			data->addColorOp ("output", arg1.c_str ());
		}
		break;
	    }
	    default:
		break;
	}
	pos = nPos;
    }
    programFreeOffset ();
    offset = NULL;
}

/*
 * Build a Compiz Fragment Function from a source string
 */
GLFragment::FunctionId
FragmentParser::buildFragmentProgram (CompString &source,
				      CompString &name,
				      int target)
{
    GLFragment::FunctionData *data;
    int handle;
    /* Create the function data */
    data = new GLFragment::FunctionData ();
    if (!data)
	return 0;
    /* Parse the source and fill the function data */
    programParseSource (data, target, source);
    /* Create the function */
    handle = data->createFragmentFunction (name.c_str ());
    delete data;
    return handle;
}

/*
 * Load a source file and build a Compiz Fragment Function from it
 */
GLFragment::FunctionId
FragmentParser::loadFragmentProgram (CompString &file,
				     CompString &name,
				     int target)
{
    CompString source;
    GLFragment::FunctionId handle;

    /* Clean fragment program name */
    name = programCleanName (name);
    /* Read the source file */
    source = programReadSource (file);
    if (source.empty ())
    {
	return 0;
    }

    /* Build the Compiz Fragment Program */
    handle = buildFragmentProgram (source,
				   name, target);

    return handle;
}
