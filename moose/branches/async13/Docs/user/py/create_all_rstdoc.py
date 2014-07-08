# create_all_rstdoc.py --- 
# 
# Filename: create_all_rstdoc.py
# Description: 
# Author: Subhasis Ray
# Maintainer: 
# Created: Mon Jun 30 21:35:07 2014 (+0530)
# Version: 
# Last-Updated: 
#           By: 
#     Update #: 0
# URL: 
# Keywords: 
# Compatibility: 
# 
# 

# Commentary: 
# 
# 
# 
# 

# Change log:
# 
# 
# 
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth
# Floor, Boston, MA 02110-1301, USA.
# 
# 

# Code:
"""Dump reStructuredText of moose builtin class docs as well as those
built into pymoose and moose python scripts."""

import sys
sys.path.append('../../../python')
import cStringIO
import re
import inspect
from datetime import datetime
import pydoc
import moose

# We assume any non-word-constituent character in the start of C++
# type name to be due to the name-mangling done by compiler on
# templated or user defined types.
type_mangling_regex = re.compile('^[^a-zA-Z]+')

finfotypes = dict(moose.finfotypes)

def extract_finfo_doc(cinfo, finfotype, docio, indent='   '):
    """Extract field documentation for all fields of type `finfotype`
    in class `cinfo` into `docio`.

    Parameters
    ----------
    cinfo: moose.Cinfo 
        class info object in MOOSE.
    
    ftype: str
        finfo type (valueFinfo/srcFinfo/destFinfo/lookupFinfo/sharedFinfo

    docio: StringIO
        IO object to write the documentation into
    """
    data = []
    try:
        finfo = moose.element('%s/%s' % (cinfo.path, finfotype)).vec
    except ValueError:
        return
    for field_element in finfo:
        dtype = type_mangling_regex.sub('', field_element.type)
        if finfotype.startswith('dest'):
            name = '.. py:method:: {0}'.format(field_element.fieldName)
            dtype = ''
        else:
            name = '.. py:attribute:: {0}'.format(field_element.fieldName)
            dtype = '{0}'.format(dtype)
        docio.write('\n{0}{1}'.format(indent, name))
        docio.write('\n\n{0}{0}{1} (*{2}*)'.format(indent, dtype, finfotypes[finfotype]))
        for line in field_element.docs.split('\n'):
            docio.write('\n\n{0}{0}{1}\n'.format(indent, line))

def extract_class_doc(name, docio, indent='   '):
    """Extract documentation for Cinfo object at path
    
    Parameters
    ----------
    name: str
        path of the class.

    docio: StringIO
        output object to write the documentation into.
    """
    cinfo = moose.Cinfo('/classes/%s' % (name))
    docs = cinfo.docs.strip()
    docio.write('\n{0}.. py:class:: {1}\n\n'.format(indent, cinfo.name))
    if docs:                    
        docs = docs.split('\n')
        # We need these checks to avoid mis-processing `:` within
        # description of the class
        name_done = False
        author_done = False
        descr_done = False
        for doc in docs:
            if not doc:
                continue
            if not (name_done and author_done and descr_done):
                pos = doc.find(':')         
                field = doc[:pos].strip()
                content = doc[pos+1:]
            else:
                content = doc
            if field.lower() == 'name':
                name_done = True
                continue
            elif field.lower() == 'author':
                author_done = True
                continue          
            elif field.lower() == 'description':
                descr_done = True
                content = content.lstrip()                
            docio.write('{0}   {1}\n\n'.format(indent, content))
    for finfotype in finfotypes.keys():
	extract_finfo_doc(cinfo, finfotype, docio, indent + '   ')

def extract_all_class_doc(docio, indent='   '):
    for cinfo in moose.element('/classes').children:
	extract_class_doc(cinfo.name, docio, indent=indent)

def extract_all_func_doc(docio, indent='   '):
    for fname, fdef in (inspect.getmembers(moose, inspect.isbuiltin) +
                        inspect.getmembers(moose, inspect.isfunction)):
	docio.write('\n{}.. py:func:: {}\n'.format(indent, fname))
	doc = inspect.getdoc(fdef)
	doc = doc.split('\n')
	drop = []
	for i in range(len(doc)):
	    di = doc[i]
	    doc[i] = di.strip()
	    hyphen_count = di.count('-')
	    if hyphen_count > 0 and hyphen_count == len(di) and i > 0:
		drop.append(i)
		doc[i-1] = indent + doc[i-1]
	for i in range(len(doc)):
	    if i not in drop:
		docio.write(doc[i] + '\n\n')


if __name__ == '__main__':
    classes_doc = 'moose_classes.rst'
    builtins_doc = 'moose_builtins.rst'
    overview_doc = 'moose_overview.rst'
    if len(sys.argv)  > 1:
        classes_doc = sys.argv[1]
    if len(sys.argv) > 2:
        builtins_doc = sys.argv[2]
    if len(sys.argv) > 3:
        overview_doc = sys.argv[3]
    ts = datetime.now()

    # MOOSE overview - the module level doc - this is for extracting
    # the moose docs into separate component files.
    overview_docio = open(overview_doc, 'w')
    overview_docio.write('.. MOOSE overview\n')
    overview_docio.write('.. As visible in the Python module\n')
    overview_docio.write(ts.strftime('.. Auto-generated on %B %d, %Y\n'))
    overview_docio.write('\n'.join(pydoc.getdoc(moose).split('\n')))
    overview_docio.write('\n')        
        
    
    if isinstance(overview_docio, cStringIO.OutputType):
	print overview_docio.getvalue()
    else:
	overview_docio.close()
    
    ## Builtin docs - we are going to do something like what autodoc
    ## does for sphinx. Because we cannot afford to build moose on
    ## servers like readthedocs, we ourselvs ectract the docs into rst
    ## files.
    builtins_docio = open(builtins_doc, 'w')
    builtins_docio.write('.. Documentation for all MOOSE builtin functions\n')
    builtins_docio.write('.. As visible in the Python module\n')
    builtins_docio.write(ts.strftime('.. Auto-generated on %B %d, %Y\n'))
    builtins_docio.write('''

MOOSE Builitin Classes and Functions
====================================
    ''')
    builtins_docio.write('\n.. py:module:: moose\n')    
    for item in ['vec', 'melement', 'LookupField', 'DestField', 'ElementField']:
        builtins_docio.write('\n   .. py:class:: {0}\n'.format(item))        
        builtins_docio.write('\n      ')
        class_obj = eval('moose.{0}'.format(item))
        builtins_docio.write('\n      '.join(pydoc.getdoc(class_obj).split('\n')))
        builtins_docio.write('\n')
        for name, member in inspect.getmembers(class_obj):
            if name.startswith('__'):
                continue
            if inspect.ismethod(member) or inspect.ismethoddescriptor(member):
                builtins_docio.write('\n         .. py:method:: {0}\n'.format(name))
            else:
                builtins_docio.write('\n         .. py:atribute:: {0}\n'.format(name))
            builtins_docio.write('\n            ')
            builtins_docio.write('\n            '.join(inspect.getdoc(member).split('\n')))
            builtins_docio.write('\n')
            

    for item in ['pwe', 'le', 'ce', 'showfield', 'showmsg', 'doc', 'element',
                 'getFieldNames', 'copy', 'move', 'delete',
                 'useClock', 'setClock', 'start', 'reinit', 'stop', 'isRunning',
                 'exists', 'writeSBML', 'readSBML', 'loadModel', 'saveModel',
                 'connect', 'getCwe', 'setCwe', 'getFieldDict', 'getField',
                 'seed', 'rand', 'wildcardFind', 'quit']:
        builtins_docio.write('\n   .. py:function:: {0}\n'.format(item))        
        builtins_docio.write('\n      ')
        builtins_docio.write('\n      '.join(inspect.getdoc(eval('moose.{0}'.format(item))).split('\n')))
        builtins_docio.write('\n')                    
    if isinstance(builtins_docio, cStringIO.OutputType):
	print builtins_docio.getvalue()
    else:
	builtins_docio.close()
    # This is the primary purpos
    classes_docio = open(classes_doc, 'w')
    classes_docio.write('.. Documentation for all MOOSE classes and functions\n')
    classes_docio.write('.. As visible in the Python module\n')
    classes_docio.write(ts.strftime('.. Auto-generated on %B %d, %Y\n'))
    
    classes_docio.write('''

MOOSE Classes
==================
''')
    extract_all_class_doc(classes_docio, indent='')
#     classes_docio.write('''
# =================
# MOOSE Functions
# =================
# ''')
#     extract_all_func_doc(classes_docio, indent='')
    if isinstance(classes_docio, cStringIO.OutputType):
	print classes_docio.getvalue()
    else:
	classes_docio.close()
    
    
        
    



# 
# create_all_rstdoc.py ends here
