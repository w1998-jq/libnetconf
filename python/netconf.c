#include <Python.h>

#include <syslog.h>

#include <libnetconf.h>

extern PyTypeObject ncSessionType;

static int syslogEnabled = 1;
static void clb_print(NC_VERB_LEVEL level, const char* msg)
{
	switch (level) {
	case NC_VERB_ERROR:
		PyErr_SetString(PyExc_Exception, msg);
		if (syslogEnabled) {syslog(LOG_ERR, "%s", msg);}
		break;
	case NC_VERB_WARNING:
		if (syslogEnabled) {syslog(LOG_WARNING, "%s", msg);}
		break;
	case NC_VERB_VERBOSE:
		if (syslogEnabled) {syslog(LOG_INFO, "%s", msg);}
		break;
	case NC_VERB_DEBUG:
		if (syslogEnabled) {syslog(LOG_DEBUG, "%s", msg);}
		break;
	}
}

static PyObject *setSyslog(PyObject *self, PyObject *args, PyObject *keywds)
{
	char* name = NULL;
	static char* logname = NULL;
	static int option = LOG_PID;
	static int facility = LOG_DAEMON;

	static char *kwlist[] = {"enabled", "name", "option", "facility", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, keywds, "p|sii", kwlist, &syslogEnabled, &name, &option, &facility)) {
		return NULL;
	}

	if (name) {
		free(logname);
		logname = strdup(name);

		closelog();
		openlog(logname, option, facility);
	}

	Py_RETURN_NONE;
}

static PyObject *setVerbosity(PyObject *self, PyObject *args)
{
	int level = NC_VERB_ERROR; /* 0 */

	if (! PyArg_ParseTuple(args, "i", &level)) {
		return NULL;
	}

	/* normalize level value if not from the enum */
	if (level < NC_VERB_ERROR) {
		nc_verbosity(NC_VERB_ERROR);
	} else if (level > NC_VERB_DEBUG) {
		nc_verbosity(NC_VERB_DEBUG);
	} else {
		nc_verbosity(level);
	}

	Py_RETURN_NONE;
}

static PyMethodDef netconfMethods[] = {
		{"setVerbosity", (PyCFunction)setVerbosity, METH_VARARGS, "Set verbose level (0-3)."},
		{"setSyslog", (PyCFunction)setSyslog, METH_VARARGS | METH_KEYWORDS, "Set application settings for syslog."},
		{NULL, NULL, 0, NULL}
};

static struct PyModuleDef ncModule = {
		PyModuleDef_HEAD_INIT,
		"netconf",
		"NETCONF Protocol implementation",
		-1,
		netconfMethods,
};

/* module initializer */
PyMODINIT_FUNC PyInit_netconf(void)
{
	PyObject *nc;

	/* initiate libnetconf - all subsystems */
	nc_init(NC_INIT_ALL);

	/* set print callback */
	nc_callback_print (clb_print);

	ncSessionType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ncSessionType) < 0) {
	    return NULL;
	}

	/* create netconf as the Python module */
	nc = PyModule_Create(&ncModule);
	if (nc == NULL) {
		return NULL;
	}

    Py_INCREF(&ncSessionType);
    PyModule_AddObject(nc, "Session", (PyObject *)&ncSessionType);

	return nc;
}
