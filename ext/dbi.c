/*
  dbi.c

  This is the general interface to COPPERMAN compliant database
  interface.

  In particular, types and type numbers are defined
 */

#include <time.h>

#include "Python.h"
#include "intobject.h"

#ifdef DATETIMEFIX
# include "datetime.h"
#endif

#define DBI_EXPORT

#include "dbi.h"

typedef struct
{
  PyObject_HEAD
  PyObject *objectOf;
} DbiContainer;


PyObject *dbiValue(PyObject *o)
{
  return ((DbiContainer *) o)->objectOf;
}

static PyObject *makeDbiTypeTP(PyObject *args, PyTypeObject *ty)
{
  DbiContainer *dbt = 0;
  PyObject *ob;
  
  if (PyArg_ParseTuple(args, "O", &ob)) {
    dbt = PyObject_NEW(DbiContainer, ty);
    dbt->objectOf = ob;
	Py_INCREF(ob);
  }
  return (PyObject*)dbt;
}

static void dbiDealloc(PyObject *self)
{
  Py_DECREF(((DbiContainer *) self)->objectOf);
  PyMem_DEL(self);
}

static PyMethodDef noMethods[] = {
  0, 0
};

static PyObject *dbiGetAttr(PyObject *self,
			    char *name)
{
  if (!strcmp(name, "value")) {
    PyObject *val = dbiValue(self);
    Py_INCREF(val);
    return val;
  }
  return Py_FindMethod(noMethods, self, name);
}

static PyObject *dateStr(PyObject *o)
{
#ifdef DATETIMEFIX
  dtime_t dt_p* = (dtime_t*)dbiValue(o);
  char target[26]; /* see p 6-52 if INFORMIX v 7.1 ESQL/C manual */ 
  if ( dttoasc(dt_p, target) < 0 ) {
    PyErr_SetString(DbiDataError, "DATETIME to string conversion failed");
    return 0;
  }
  return PyString_FromStringAndSize(target, strlen(target));
#else
  long l = PyInt_AsLong(dbiValue(o));
  return PyString_FromStringAndSize(ctime(&l), 24); /* less \n */
#endif
}

static PyObject *rawStr(PyObject *o)
{
  PyObject *val = dbiValue(o);
  Py_INCREF(val);
  return val;
}

#define delg(a) dbiValue(a)->ob_type->tp_as_number

static PyObject* dt_nb_add(PyObject* a, PyObject* b)
{ return delg(a)->nb_add(dbiValue(a),b); }
static PyObject* dt_nb_subtract(PyObject* a, PyObject* b)
{ return delg(a)->nb_subtract(dbiValue(a),b); }
static PyObject* dt_nb_int(PyObject* a)
{ return delg(a)->nb_int(dbiValue(a)); }
static PyObject* dt_nb_long(PyObject* a)
{ return delg(a)->nb_long(dbiValue(a)); }
static PyObject* dt_nb_float(PyObject* a)
{ return delg(a)->nb_float(dbiValue(a)); }
static PyObject* dt_nb_oct(PyObject* a)
{ return delg(a)->nb_oct(dbiValue(a)); }
static PyObject* dt_nb_hex(PyObject* a)
{ return delg(a)->nb_hex(dbiValue(a)); }
static int dt_cmp(PyObject *a, PyObject *b)
{ return dbiValue(a)->ob_type->tp_compare(a,b); }
static int dt_nb_coerce(PyObject **, PyObject **);

PyNumberMethods dt_as_number = {
  dt_nb_add ,   /* nb_add */
  dt_nb_subtract ,   /* nb_subtract */
  0,   /* nb_multiply */
  0,   /* nb_divide */
  0,   /* nb_remainder */
  0,   /* nb_divmod */
  0,   /* nb_power */
  0,   /* nb_negative */
  0,   /* nb_positive */
  0,   /* nb_absolute */
  0,   /* nb_nonzero */
  0,   /* nb_invert */
  0,   /* nb_lshift */
  0,   /* nb_rshift */
  0,   /* nb_and */
  0,   /* nb_xor */
  0,   /* nb_or */
  dt_nb_coerce,   /* nb_coerce */
  dt_nb_int ,   /* nb_int */
  dt_nb_long ,   /* nb_long */
  dt_nb_float ,   /* nb_float */
  dt_nb_oct ,   /* nb_oct */
  dt_nb_hex    /* nb_hex */
} ;

static PyTypeObject DbiDate_Type =
{
#ifdef WIN32
  PyObject_HEAD_INIT (0)
#else
  PyObject_HEAD_INIT (&PyType_Type)
#endif
  0,			/*ob_size */
  "DbiDate",		/*tp_name */
  sizeof(DbiContainer),	/*tp_basicsize */
  0,			/*tp_itemsize */
  dbiDealloc,		/*tp_dealloc */
  0,			/*tp_print */
  dbiGetAttr,		/*tp_getattr */
  0,                    /*tp_setattr */
  dt_cmp,               /*tp_compare */
  0,                    /*tp_repr */
  &dt_as_number,        /**tp_as_number */
  0,                    /**tp_as_sequence */
  0,                    /**tp_as_mapping */
  0,                    /*tp_hash */
  0,                    /*tp_call */
  dateStr               /*tp_str */

} ;
static int dt_nb_coerce(PyObject **pv, PyObject **pw)
{
  if (PyNumber_Check(*pw)) {
    DbiContainer *dbt = PyObject_NEW(DbiContainer, &DbiDate_Type);
    dbt->objectOf = PyNumber_Int(*pw);
    Py_INCREF(*pv);
    *pw = (PyObject *)dbt;
    return 0;
  }
  return 1; /* Can't do it */
}

static PyTypeObject DbiRaw_Type =
{
#ifdef WIN32
  PyObject_HEAD_INIT (0)
#else
  PyObject_HEAD_INIT (&PyType_Type)
#endif
  0,			/*ob_size */
  "DbiRaw",		/*tp_name */
  sizeof(DbiContainer),	/*tp_basicsize */
  0,			/*tp_itemsize */
  dbiDealloc,   	/*tp_dealloc */
  0,			/*tp_print */
  dbiGetAttr,		/*tp_getattr */
  0,                    /*tp_setattr */
  0,                    /*tp_compare */
  0,                    /*tp_repr */
  0,                    /**tp_as_number */
  0,                    /**tp_as_sequence */
  0,                    /**tp_as_mapping */
  0,                    /*tp_hash */
  0,                    /*tp_call */
  rawStr                /*tp_str */
} ;

static PyTypeObject DbiRowId_Type =
{
#ifdef WIN32
  PyObject_HEAD_INIT (0)
#else
  PyObject_HEAD_INIT (&PyType_Type)
#endif
  0,			/*ob_size */
  "DbiRowId",		/*tp_name */
  sizeof(DbiContainer),	/*tp_basicsize */
  0,			/*tp_itemsize */
  dbiDealloc,   	/*tp_dealloc */
  0,			/*tp_print */
  dbiGetAttr		/*tp_getattr */
} ;

static PyObject *makeDate(PyObject *self, PyObject *args)
{
  return makeDbiTypeTP(args, &DbiDate_Type);
}

static PyObject *makeRaw(PyObject *self, PyObject *args)
{
  return makeDbiTypeTP(args, &DbiRaw_Type);
}

static PyObject *makeRowId(PyObject *self, PyObject *args)
{
  return makeDbiTypeTP(args, &DbiRowId_Type);
}

static PyObject *makeDbiType(PyObject *o, PyTypeObject *ty)
{
  DbiContainer *dbt = PyObject_NEW(DbiContainer, ty);
  dbt->objectOf = o;
  return (PyObject*) dbt;
}

PyObject *dbiMakeDate(PyObject *contents)
{
  return makeDbiType(contents, &DbiDate_Type);
}

PyObject *dbiMakeRaw(PyObject *contents)
{
  return makeDbiType(contents, &DbiRaw_Type);
}

PyObject *dbiMakeRowId(PyObject *contents)
{
  return makeDbiType(contents, &DbiRowId_Type);
}



static PyMethodDef globalMethods[] = {
  { "dbDate", makeDate, 1} ,
  { "dbiDate", makeDate, 1} ,
  { "dbRaw", makeRaw, 1} ,
  { "dbiRaw", makeRaw, 1} ,
  {0,     0}        /* Sentinel */
};

void initdbi()
{
#ifdef WIN32
  PyObject *m, *d;
  DbiDate_Type.ob_type = &PyType_Type;	
  DbiRaw_Type.ob_type = &PyType_Type;	
  DbiRowId_Type.ob_type = &PyType_Type;	
  m = Py_InitModule("dbi", globalMethods);
  d = PyModule_GetDict(m);
#else
  PyObject *m = Py_InitModule("dbi", globalMethods);
  PyObject *d = PyModule_GetDict(m);
#endif
  PyDict_SetItemString(d, "STRING",
		       DbiString = PyString_FromString("STRING"));
  PyDict_SetItemString(d, "RAW",
		       DbiRaw = PyString_FromString("RAW"));
  PyDict_SetItemString(d, "NUMBER",
		       DbiNumber = PyString_FromString("NUMBER"));
  PyDict_SetItemString(d, "DATE",
		       DbiDate = PyString_FromString("DATE"));
  PyDict_SetItemString(d, "ROWID",
		       DbiRowId = PyString_FromString("ROWID"));
  PyDict_SetItemString(d, "TYPES",
		       Py_BuildValue("(OOOOO)",
				     DbiString,
				     DbiRaw,
				     DbiNumber,
				     DbiDate,
				     DbiRowId));

  /* Establish errors */
  PyDict_SetItemString(d, "noError",
		       DbiNoError = PyString_FromString("dbi.no-error"));
  PyDict_SetItemString(d, "opError",
		       DbiOpError = PyString_FromString("dbi.operation-error"));
  PyDict_SetItemString(d, "progError",
		       DbiProgError = PyString_FromString("dbi.program-error"));
  PyDict_SetItemString(d, "integrityError",
		       DbiIntegrityError =
		       PyString_FromString("dbi.integrity-error"));
  PyDict_SetItemString(d, "dataError",
		       DbiDataError = PyString_FromString("dbi.data-error"));
  PyDict_SetItemString(d, "internalError",
		       DbiInternalError = 
		       PyString_FromString("dbi.internal-error"));

}

int dbiIsDate(const PyObject *o)
{
  return o->ob_type == &DbiDate_Type;
}

int dbiIsRaw(const PyObject *o)
{
  return o->ob_type == &DbiRaw_Type;
}

int dbiIsRowId(const PyObject *o)
{
  return o->ob_type == &DbiRowId_Type;
}