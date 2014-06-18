# This file is part of MOOSE simulator: http://moose.ncbs.res.in.

# MOOSE is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# MOOSE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with MOOSE.  If not, see <http://www.gnu.org/licenses/>.


"""backend.py: 

Last modified: Tue Jun 17, 2014  01:59PM

"""
    
__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2013, Dilawar Singh and NCBS Bangalore"
__credits__          = ["NCBS Bangalore"]
__license__          = "GNU GPL"
__version__          = "1.0.0"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"
__status__           = "Development"


import sys
import os
from .. import _moose 
from .. import print_utils 
from collections import defaultdict

class Backend(object):
    """ Base class for all backend """

    def __init__(self, *args):
        super(Backend, self).__init__()
        self.args = args
        self.compartments = []
        self.pulseGens = []
        self.tables = []
        # A set of tuple of sourceCompartment.path and targetCompartment.path
        self.connections = set()
        self.clock = _moose.wildcardFind('/clock')[0]

    def filterPaths(self, mooseObjs, ignorePat=None):
        """Filter paths """
        def ignore(x):
            if ignorePat.search(x.path):
                return False
            return True
        if ignorePat:
            mooseObjs = filter(ignore, mooseObjs)
        return mooseObjs

    def getComparments(self, **kwargs):
        '''Get all compartments in moose '''
        comps = _moose.wildcardFind('/##[TYPE=Compartment]')
        self.compartments = comps
        return comps

    def getPulseGens(self, **kwargs):
        """ Get all the pulse generators """
        self.pulseGens = _moose.wildcardFind('/##[TYPE=PulseGen]')
        return self.pulseGens

    def getTables(self, **kwargs):
        """ Get all table we are recording from"""
        self.tables = _moose.wildcardFind('/##[TYPE=Table]')
        return self.tables

    def populateStoreHouse(self, **kwargs):
        """ Populate all data-structures related with Compartments, Tables, and
        pulse generators.
        """
        print_utils.dump("INFO", "Getting moose-datastructure for backend.")
        self.getComparments(**kwargs)
        self.getTables(**kwargs)
        self.getPulseGens(**kwargs)
