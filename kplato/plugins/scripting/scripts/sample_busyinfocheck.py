# -*- coding: utf-8 -*-

import os, datetime, sys, traceback, csv, pickle
import Kross, KPlato


class BusyinfoCheck:

    def __init__(self, scriptaction):
        proj = KPlato.project()
        self.check( proj)
        
    def check(self, project ):
        lst = []
        for i in range( project.resourceGroupCount() ):
            g = project.resourceGroupAt( i )
            for ri in range( g.resourceCount() ):
                r = g.resourceAt( ri )
                lst = r.externalAppointments()
                for iv in lst:
                    iv.insert( 0, r.id() )
                    iv.insert( 1, KPlato.data( r, 'ResourceName' ) )
                print lst

BusyinfoCheck( self )
