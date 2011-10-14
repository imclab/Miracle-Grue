#
# Top-level SConstruct file for Miracle Grue.
#

# env = Environment(tools=['default','qt4'])

# On Mac builds, scons complains that Qt4 can't be found. By mergeing the PATH environment variable
# the moc tool is detected and Qt4 is detected 
import os
import commands



print ""
print "======================================================="
print "So you think you can build Miracle-Grue on your machine?"
print ""
print "* Qt detector: '%s'" % commands.getoutput("moc -v")
print "* Looking for cppunit"

cppunit_lib_dir = ""
cppunit_inc_dir = ""

try:
	print "    CPPUNIT_LIB_DIR environment variable:"
	cppunit_lib_dir = os.environ['CPPUNIT_LIB_DIR']
	print "         '%s'" % cppunit_lib_dir
	print "    CPPUNIT_INC_DIR environment variable:"
	cppunit_inc_dir = os.environ['CPPUNIT_INC_DIR']
	print "         '%s'" % cppunit_inc_dir
	print 
except:
	print "OUCH! I'm very confused right now"
	print "   Your mileage may vary"
	print 

env = Environment(ENV = {'PATH' : os.environ['PATH']}, tools=['default','qt4'])




env.EnableQt4Modules([
	'QtCore',
	'QtNetwork'
	])

env.Program('qt4Hello', 
		['hello.cc',  'src/Operation.h', 'src/SliceOperation.cc', 'src/DebugOperation.cc'])

# /usr/local/include/cppunit/
# Program('prog.c', LIBS = 'm',
#                       LIBPATH = ['/usr/lib', '/usr/local/lib'])

env.Program('unit_operation',['src/unit_operation.cc', 'src/unit_operation_main.cc'], CPPPATH=[''], LIBS = 'cppunit', LIBPATH = ['/usr/lib', '/usr/local/lib']) 