from  __future__ import  print_function

import os
import sys
import re
import datetime
import string

includesParser = re.compile( r'\s*#include\s*"(.*)"' )
commentParser1 = re.compile( r'^\s*/\*')
commentParser2 = re.compile( r'^ \*')
blankParser = re.compile( r'^\s*$')
seenHeaders = set([])

path = os.path.dirname(os.path.realpath( os.path.dirname(sys.argv[0])))
rootPath = os.path.join( path, 'include/' )
outputPath = os.path.join( path, 'single_include/ecs.h' )

includeImpl = True

out = open( outputPath, 'w' )

def write( line ):
    out.write( line )

def parseFile( path, filename ):
    f = open( path + filename, 'r' )
    blanks = 0
    for line in f:
        m = includesParser.match( line )
        if m:
            header = m.group(1)
            headerPath, sep, headerFile = header.rpartition( "/" )
            if not headerFile in seenHeaders:
                seenHeaders.add( headerFile )
                write( "// #included from: {0}\n".format( header ) )
                if os.path.exists( path + headerPath + sep + headerFile ):
                    parseFile( path + headerPath + sep, headerFile )
                else:
                    parseFile( rootPath + headerPath + sep, headerFile )
        else:
            #if (not guardParser.match( line ) or defineParser.match( line ) ) and not commentParser1.match( line )and not commentParser2.match( line ):
            if blankParser.match( line ):
                blanks = blanks + 1
            else:
                blanks = 0
            if blanks < 2:
                write( line.rstrip() + "\n" )

out.write( "///\n" )
out.write( "/// OpenEcs v{0}\n".format( "0.1.101" ) )
out.write( "/// Generated: {0}\n".format( datetime.datetime.now() ) )
out.write( "/// ----------------------------------------------------------\n" )
out.write( "/// This file has been generated from multiple files. Do not modify\n" )
out.write( "/// ----------------------------------------------------------\n" )
out.write( "///\n" )
out.write( "#ifndef ECS_SINGLE_INCLUDE_H\n" )
out.write( "#define ECS_SINGLE_INCLUDE_H\n" )

parseFile( rootPath, 'ecs.h' )

out.write( "#endif // ECS_SINGLE_INCLUDE_H\n\n" )

print ("Generated single include for OpenEcs\n" )