# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Keith Sloan <keith@sloan-home.co.uk>               *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         * 
#*   Acknowledgements :                                                    *
#*                                                                         *
#*     Thanks to shoogen on the FreeCAD forum and Peter Li                 *
#*     for programming advice and some code.                               *
#*                                                                         *
#*                                                                         *
#***************************************************************************
__title__="FreeCAD OpenSCAD Workbench - CSG importer Version 0.05d"
__author__ = "Keith Sloan <keith@sloan-home.co.uk>"
__url__ = ["http://www.sloan-home.co.uk/ImportCSG"]

import FreeCAD, os, sys
import ply.lex as lex
import ply.yacc as yacc
import Part

from OpenSCADFeatures import RefineShape 
from OpenSCAD2Dgeom import *
from OpenSCADUtils import *
isspecialorthogonaldeterminant = isspecialorthogonalpython
from OpenSCADFeatures import Twist

if open.__module__ == '__builtin__':
    pythonopen = open # to distinguish python built-in open function from the one declared here

# Get the token map from the lexer.  This is required.
import tokrules
from tokrules import tokens

#Globals
dxfcache = {}

def open(filename):
    "called when freecad opens a file."
    global doc
    global pathName
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    if filename.lower().endswith('.scad'):
        tmpfile=callopenscad(filename)
        pathName = '' #https://github.com/openscad/openscad/issues/128
        #pathName = os.getcwd() #https://github.com/openscad/openscad/issues/128
        processcsg(tmpfile)
        os.unlink(tmpfile)
    else:
        pathName = os.path.dirname(os.path.normpath(filename))
        processcsg(filename)
    return doc

def insert(filename,docname):
    "called when freecad imports a file"
    global doc
    global pathName
    groupname = os.path.splitext(os.path.basename(filename))[0]
    try:
        doc=FreeCAD.getDocument(docname)
    except:
        doc=FreeCAD.newDocument(docname)
    importgroup = doc.addObject("App::DocumentObjectGroup",groupname)
    if filename.lower().endswith('.scad'):
        tmpfile=callopenscad(filename)
        pathName = '' #https://github.com/openscad/openscad/issues/128
        #pathName = os.getcwd() #https://github.com/openscad/openscad/issues/128
        processcsg(tmpfile)
        os.unlink(tmpfile)
    else:
        pathName = os.path.dirname(os.path.normpath(filename))
        processcsg(filename)

def processcsg(filename):
    global doc
    
    print 'ImportCSG Version 0.5d'
    # Build the lexer
    print 'Start Lex'
    lex.lex(module=tokrules)
    print 'End Lex'

    # Build the parser   
    print 'Load Parser'
    # No debug out otherwise Linux has protection exception
    #parser = yacc.yacc(debug=0)
    parser = yacc.yacc(debug=0)
    print 'Parser Loaded'
    # Give the lexer some input
    #f=open('test.scad', 'r')
    f = pythonopen(filename, 'r')
    #lexer.input(f.read())

    print 'Start Parser'
    # Swap statements to enable Parser debugging
    #result = parser.parse(f.read(),debug=1)
    result = parser.parse(f.read())
    print 'End Parser'
    print result  
    FreeCAD.Console.PrintMessage('End processing CSG file')
    doc.recompute()
    #import colorcodeshapes
    #colorcodeshapes.colorcodeshapes(doc.Objects)
    
def p_block_list_(p):
    '''
    block_list : statement 
               | block_list statement 
    '''
    print "Block List"
    print p[1]
    if(len(p) > 2) :
        print p[2]
        p[0] = p[1] + p[2]
    else :
        p[0] = p[1]
    print "End Block List"    
    
def p_group_action1(p):
    'group_action1 : group LPAREN RPAREN OBRACE block_list EBRACE'
    print "Group"
    p[0] = p[5]

def p_group_action2(p) :
    'group_action2 : group LPAREN RPAREN SEMICOL'
    print "Group2"
    p[0] = []
   
def p_boolean(p) :
    '''
    boolean : true
            | false
    '''
    p[0] = p[1]

#def p_string(p):
#    'string : QUOTE ID QUOTE'
#    p[0] = p[2]

def p_stripped_string(p):
    'stripped_string : STRING'
    p[0] = p[1].strip('"')

def p_statement(p):
    '''statement : part
                 | operation
                 | multmatrix_action
                 | group_action1
                 | group_action2
                 | color_action
                 | not_supported
    '''
    p[0] = p[1]

def p_part(p):
    '''
    part : sphere_action
         | cylinder_action
         | cube_action
         | circle_action
         | square_action
         | polygon_action_nopath
         | polygon_action_plus_path
         | polyhedron_action
         '''
    p[0] = p[1]

def p_2d_point(p):
    '2d_point : OSQUARE NUMBER COMMA NUMBER ESQUARE'
    global points_list
    print "2d Point"
    p[0] = [float(p[2]),float(p[4])]

def p_points_list_2d(p):
    '''
    points_list_2d : 2d_point COMMA
                   | points_list_2d 2d_point COMMA
                   | points_list_2d 2d_point
                   '''
    if p[2] == ',' :
        print "Start List"
        print p[1]
        p[0] = [p[1]]
    else :
        print p[1]
        print p[2]
        p[1].append(p[2])
        p[0] = p[1]
    print p[0]

def p_3d_point(p):
    '3d_point : OSQUARE NUMBER COMMA NUMBER COMMA NUMBER ESQUARE'
    global points_list
    print "3d point"
    p[0] = [p[2],p[4],p[6]]
   
def p_points_list_3d(p):
    '''
    points_list_3d : 3d_point COMMA
               | points_list_3d 3d_point COMMA
               | points_list_3d 3d_point
               '''
    if p[2] == ',' :
        print "Start List"
        print p[1]
        p[0] = [p[1]]
    else :
        print p[1]
        print p[2]
        p[1].append(p[2])
        p[0] = p[1]
    print p[0]

def p_path_points(p):
    '''
    path_points : NUMBER COMMA
                | path_points NUMBER COMMA
                | path_points NUMBER
                '''
    print "Path point"
    if p[2] == ',' :
        print 'Start list'
        print p[1]
        p[0] = [int(p[1])]
    else :
        print p[1]
        print len(p[1])
        print p[2]
        p[1].append(int(p[2]))
        p[0] = p[1]
    print p[0]


def p_path_list(p):
    'path_list : OSQUARE path_points ESQUARE'
    print 'Path List '
    print p[2]
    p[0] = p[2]

def p_path_set(p) :
    '''
    path_set : path_list
             | path_set COMMA path_list
             '''
    print 'Path Set'
    print len(p)
    if len(p) == 2 :
        p[0] = [p[1]]
    else :
        p[1].append(p[3])
        p[0] = p[1]
    print p[0]

def p_operation(p):
    '''
    operation : difference_action
              | intersection_action
              | union_action
              | rotate_extrude_action
              | linear_extrude_with_twist
              | linear_extrude_action2
              | rotate_extrude_file
              | import_file1
              | projection_action
              '''
    p[0] = p[1]

def p_not_supported(p):
    '''
    not_supported : hull
                  | minkowski
                  '''
    from PyQt4 import QtGui
    QtGui.QMessageBox.critical(None, "Unsupported Function : "+p[1], "Press OK")
    
def p_size_vector(p):
    'size_vector : OSQUARE NUMBER COMMA NUMBER COMMA NUMBER ESQUARE'
    print "size vector"
    p[0] = [p[2],p[4],p[6]]

def p_assign(p):
    'assign : ID EQ NUMBER'
    print "Assignment"
    print p[1] + ' : ' + p[3]
    p[0] = p[3]

def p_color_action(p):
    'color_action : color LPAREN vector RPAREN OBRACE block_list EBRACE'
    import math
    print "Color"
    color = tuple([float(f) for f in p[3][:3]]) #RGB
    transp = 100 - int(math.floor(100*float(p[3][3]))) #Alpha
    for obj in p[6]:
        obj.ViewObject.ShapeColor =color
        obj.ViewObject.Transparency = transp
    p[0] = p[6]
    
# Error rule for syntax errors
def p_error(p):
    print "Syntax error in input!"
    print p    

def fuse(list,name):
    global doc
    print "Fuse"
    print list
    # Is this Multi Fuse
    if ( len(list) > 2):
       print "Multi Fuse"
       myfuse = doc.addObject('Part::MultiFuse',name)
       myfuse.Shapes = list
       for subobj in myfuse.Shapes:
           subobj.ViewObject.hide()    
    else :
       print "Single Fuse"
       myfuse = doc.addObject('Part::Fuse',name)
       myfuse.Base = list[0]
       myfuse.Tool = list[1]
       myfuse.Base.ViewObject.hide()
       myfuse.Tool.ViewObject.hide()
    return(myfuse)

def p_union_action(p):
    'union_action : union LPAREN RPAREN OBRACE block_list EBRACE'
    print "union"
    newpart = fuse(p[5],p[1])
    print "Push Union Result"
    p[0] = [newpart]
    print "End Union"
    
def p_difference_action(p):  
    'difference_action : difference LPAREN RPAREN OBRACE block_list EBRACE'

    print "difference"
    print len(p[5])
    print p[5]
       
    mycut = doc.addObject('Part::Cut',p[1])
# Cut using Fuse    
    mycut.Base = p[5][0]
#    Can only Cut two objects do we need to fuse extras
    if (len(p[5]) > 2 ):
       print "Need to Fuse Extra First"
       mycut.Tool = fuse(p[5][1:],'union') 
    else :   
       mycut.Tool = p[5][1]
    mycut.Base.ViewObject.hide()
    mycut.Tool.ViewObject.hide()       
    print "Push Resulting Cut"
    p[0] = [mycut]
    print "End Cut"    

def p_intersection_action(p):
    'intersection_action : intersection LPAREN RPAREN OBRACE block_list EBRACE'
    
    print "intersection"
    # Is this Multi Common
    if (len(p[5]) > 2):    
       print "Multi Common"
       mycommon = doc.addObject('Part::MultiCommon',p[1])
       mycommon.Shapes = p[5]
       for subobj in mycommon.Shapes:
           subobj.ViewObject.hide()    
    else :
       print "Single Common"
       mycommon = doc.addObject('Part::Common',p[1])
       mycommon.Base = p[5][0]
       mycommon.Tool = p[5][1]
       mycommon.Base.ViewObject.hide()
       mycommon.Tool.ViewObject.hide()

    p[0] = [mycommon]
    print "End Intersection"

def process_rotate_extrude(obj):
    myrev = doc.addObject("Part::Revolution","RotateExtrude")
    myrev.Source = obj
    myrev.Axis = (0.00,1.00,0.00)
    myrev.Base = (0.00,0.00,0.00)
    myrev.Angle = 360.00
    myrev.Placement=FreeCAD.Placement(FreeCAD.Vector(),FreeCAD.Rotation(0,0,90))
    obj.ViewObject.hide()
    newobj=doc.addObject("Part::FeaturePython",'RefineRotateExtrude')
    RefineShape(newobj,myrev)
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetBool('useViewProviderTree'):
        from OpenSCADFeatures import ViewProviderTree
        ViewProviderTree(newobj.ViewObject)
    else:
        newobj.ViewObject.Proxy = 0
    myrev.ViewObject.hide()
    return(newobj)

def p_rotate_extrude_action(p): 
    'rotate_extrude_action : rotate_extrude LPAREN assign COMMA assign COMMA assign COMMA assign RPAREN OBRACE block_list EBRACE'
    print "Rotate Extrude"
    if (len(p[12]) > 1) :
        part = fuse(p[12],"Rotate Extrude Union")
    else :
        part = p[12][0]
    p[0] = [process_rotate_extrude(part)]
    print "End Rotate Extrude"

def p_rotate_extrude_file(p):
    'rotate_extrude_file : rotate_extrude LPAREN file EQ stripped_string COMMA layer EQ stripped_string COMMA origin EQ 2d_point COMMA assign \
    COMMA assign COMMA assign COMMA assign COMMA assign RPAREN SEMICOL'
    print "Rotate Extrude File"
    filen,ext =p[5] .rsplit('.',1)
    obj = process_import_file(filen,ext,p[9])
    p[0] = [process_rotate_extrude(obj)]
    print "End Rotate Extrude File"

def process_linear_extrude(obj,h) :
    mylinear = doc.addObject("Part::Extrusion","LinearExtrude")
    mylinear.Base = obj
    mylinear.Dir = (0,0,h)
    mylinear.Placement=FreeCAD.Placement()
    try: 
        mylinear.Solid = True
    except:
        a = 1 # Any old null statement
    obj.ViewObject.hide()
    newobj=doc.addObject("Part::FeaturePython",'RefineLinearExtrude')
    RefineShape(newobj,mylinear)
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetBool('useViewProviderTree'):
        from OpenSCADFeatures import ViewProviderTree
        ViewProviderTree(newobj.ViewObject)
    else:
        newobj.ViewObject.Proxy = 0
    mylinear.ViewObject.hide()
    return(newobj)

def process_linear_extrude_with_twist(base,height,twist) :   
    newobj=doc.addObject("Part::FeaturePython",'twist_extrude')
    Twist(newobj,base,height,-twist) #base is an FreeCAD Object, heigth and twist are floats
    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetBool('useViewProviderTree'):
        from OpenSCADFeatures import ViewProviderTree
        ViewProviderTree(newobj.ViewObject)
    else:
        newobj.ViewObject.Proxy = 0
    #import ViewProviderTree from OpenSCADFeatures
    #ViewProviderTree(obj.ViewObject)
    return(newobj)

def p_linear_extrude_with_twist(p):
    'linear_extrude_with_twist : linear_extrude LPAREN assign COMMA center EQ boolean COMMA assign COMMA assign COMMA assign COMMA \
     assign COMMA assign COMMA assign RPAREN OBRACE block_list EBRACE'
    print "Linear Extrude With Twist"
    h = float(p[3])
    print "Twist : "+p[11]
    t = float(p[11])
    s = int(p[13])
    if (len(p[22]) > 1) :
        obj = fuse(p[22],"Linear Extrude Union")
    else :
        obj = p[22][0]
    if t:
        p[0] = [process_linear_extrude_with_twist(obj,h,t)]
    else:
        p[0] = [process_linear_extrude(obj,h)]
    if p[7]=='true' :
       center(obj,0,0,h)
    print "End Linear Extrude with twist"
    
def p_linear_extrude_action2(p):
    'linear_extrude_action2 : linear_extrude LPAREN assign COMMA center EQ boolean COMMA assign COMMA assign COMMA assign COMMA \
     assign RPAREN OBRACE block_list EBRACE'
    print "Linear Extrude 2"
    h = float(p[3])
    if (len(p[18]) > 1) :
        obj = fuse(p[18],"Linear Extrude Union")
    else :
        obj = p[18][0]
    p[0] = [process_linear_extrude(obj,h)]
    if p[7]=='true' :
       center(obj,0,0,h)
    print "End Linear Extrude 2"

def p_import_file1(p):
    'import_file1 : import LPAREN file EQ stripped_string COMMA layer EQ stripped_string COMMA origin EQ 2d_point  COMMA assign COMMA assign COMMA \
    assign COMMA assign COMMA assign RPAREN SEMICOL'
    print "Import File"
    filen,ext =p[5] .rsplit('.',1)
    p[0] = [process_import_file(filen,ext,p[9])]
    print "End Import File"

def process_import_file(fname,ext,layer):
    print "Importing : "+fname+"."+ext+" Layer : "+layer
    if ext.lower() in reverseimporttypes()['Mesh']:
        obj=process_mesh_file(fname,ext)
    elif ext=='dxf' :
        obj=processDXF(fname,layer)
    else :
        print "Unsupported file extension"
    return(obj)

def process_mesh_file(fname,ext):
    import Mesh
    fullname = fname+'.'+ext
    filename = os.path.join(pathName,fullname)
    mesh1 = doc.getObject(fname) #reuse imported object
    if not mesh1:
        Mesh.insert(filename)
        mesh1=doc.getObject(fname)
    mesh1.ViewObject.hide()
    sh=Part.Shape()
    sh.makeShapeFromMesh(mesh1.Mesh.Topology,0.1)
    solid = Part.Solid(sh)
    obj=doc.addObject('Part::Feature',"Mesh")
    #ImportObject(obj,mesh1) #This object is not mutable from the GUI
    #ViewProviderTree(obj.ViewObject)
    solid=solid.removeSplitter()
    if solid.Volume < 0:
        #sh.reverse()
        #sh = sh.copy()
        solid.complement()
    obj.Shape=solid#.removeSplitter()
    return(obj)

def processDXF(fname,layer):
    global doc
    global pathName
    print "Process DXF file"
    print "File Name : "+fname
    print "Layer : "+layer
    print "PathName : "+pathName
    dxfname = fname+'.dxf'
    filename = os.path.join(pathName,dxfname)
    print "DXF Full path : "+filename
    #featname='import_dxf_%s_%s'%(objname,layera)
    # reusing an allready imported object does not work if the
    #shape in not yet calculated
    import importDXF
    global dxfcache
    layers=dxfcache.get(id(doc),[])
    print "Layers : "+str(layers)
    if layers:
        try:
            groupobj=[go for go in layers if (not layer) or go.Label == layer]
        except:
            groupobj= None
    else:
        groupobj= None
    if not groupobj:
        print "Importing Layer"
        layers = importDXF.processdxf(doc,filename) or importDXF.layers
        dxfcache[id(doc)] = layers[:]
        for l in layers:
            for o in l.Group:
                o.ViewObject.hide()
            l.ViewObject.hide()
        groupobj=[go for go in layers if (not layer) or go.Label == layer]
    edges=[]
    if not groupobj:
        print 'import of layer %s failed' % layer
    for shapeobj in groupobj[0].Group:
        edges.extend(shapeobj.Shape.Edges)
    f=edgestofaces(edges)
    #obj=doc.addObject("Part::FeaturePython",'import_dxf_%s_%s'%(objname,layera))
    obj=doc.addObject('Part::Feature',"dxf")
    #ImportObject(obj,groupobj[0]) #This object is not mutable from the GUI
    #ViewProviderTree(obj.ViewObject)
    obj.Shape=f
    print "DXF Diagnostics"
    print obj.Shape.ShapeType
    print "Closed : "+str(f.isClosed())
    print f.check()
    print [w.isClosed() for w in obj.Shape.Wires]
    return(obj)

def processSTL(fname):
    print "Process STL file"

def p_multmatrix_action(p):
    'multmatrix_action : multmatrix LPAREN matrix RPAREN OBRACE block_list EBRACE'
    print "MultMatrix"
    transform_matrix = FreeCAD.Matrix()
    print "Multmatrix"
    print p[3]
    transform_matrix.A11 = round(float(p[3][0][0]),12)
    transform_matrix.A12 = round(float(p[3][0][1]),12)
    transform_matrix.A13 = round(float(p[3][0][2]),12)
    transform_matrix.A14 = round(float(p[3][0][3]),12)
    transform_matrix.A21 = round(float(p[3][1][0]),12)
    transform_matrix.A22 = round(float(p[3][1][1]),12)
    transform_matrix.A23 = round(float(p[3][1][2]),12)
    transform_matrix.A24 = round(float(p[3][1][3]),12)
    transform_matrix.A31 = round(float(p[3][2][0]),12)
    transform_matrix.A32 = round(float(p[3][2][1]),12)
    transform_matrix.A33 = round(float(p[3][2][2]),12)
    transform_matrix.A34 = round(float(p[3][2][3]),12)
    print transform_matrix
    print "Apply Multmatrix"
#   If more than one object on the stack for multmatrix fuse first
    if (len(p[6]) > 1) :
        part = fuse(p[6],"Matrix Union")
    else :                   
        part = p[6][0]
#    part = new_part.transformGeometry(transform_matrix)
#    part = new_part.copy()
#    part.transformShape(transform_matrix)
    if (isspecialorthogonaldeterminant(fcsubmatrix(transform_matrix))) :
       print "Orthogonal"
       part.Placement=FreeCAD.Placement(transform_matrix).multiply(part.Placement)
       new_part = part
    elif FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetBool('useMultmatrixFeature'):
        from OpenSCADFeatures import MatrixTransform
        new_part=doc.addObject("Part::FeaturePython",'Matrix Deformation')
        MatrixTransform(new_part,transform_matrix,part)
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(new_part.ViewObject)
        else:
            new_part.ViewObject.Proxy = 0
        part.ViewObject.hide()
    else :
        print "Transform Geometry"
#       Need to recompute to stop transformGeometry causing a crash        
        doc.recompute()
        new_part = doc.addObject("Part::Feature","Matrix Deformation")
      #  new_part.Shape = part.Base.Shape.transformGeometry(transform_matrix)
        new_part.Shape = part.Shape.transformGeometry(transform_matrix) 
        part.ViewObject.hide()
    if False :  
#   Does not fix problemfile or beltTighener although later is closer       
        newobj=doc.addObject("Part::FeaturePython",'RefineMultMatrix')
        RefineShape(newobj,new_part)
        newobj.ViewObject.Proxy = 0
        new_part.ViewObject.hide()   
        p[0] = [newobj]
    else :
        p[0] = [new_part]
    print "Multmatrix applied"
    
def p_matrix(p):
    'matrix : OSQUARE vector COMMA vector COMMA vector COMMA vector ESQUARE'
    print "Matrix"
    p[0] = [p[2],p[4],p[6],p[8]]

def p_vector(p):
    'vector : OSQUARE NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER ESQUARE'
    print "Vector"
    p[0] = [p[2],p[4],p[6],p[8]]

def center(obj,x,y,z):
    obj.Placement = FreeCAD.Placement(\
        FreeCAD.Vector(-x/2.0,-y/2.0,-z/2.0),\
        FreeCAD.Rotation(0,0,0,1))
    
def p_sphere_action(p):
    'sphere_action : sphere LPAREN assign COMMA assign COMMA assign COMMA assign RPAREN SEMICOL'
    print "Sphere : "+p[9]
    r = float(p[9])
    mysphere = doc.addObject("Part::Sphere",p[1])
    mysphere.Radius = r
    print "Push Sphere"
    p[0] = [mysphere]
    print "End Sphere"

def myPolygon(n,r1):
    # Adapted from Draft::_Polygon
    import math
    print "My Polygon"
    angle = math.pi*2/n
    nodes = [FreeCAD.Vector(r1,0,0)]
    for i in range(n-1) :
        th = (i+1) * angle
        nodes.append(FreeCAD.Vector(r1*math.cos(th),r1*math.sin(th),0))
    nodes.append(nodes[0])
    polygonwire = Part.makePolygon(nodes)

    polygon = doc.addObject("Part::Feature","Polygon")
    polygon.Shape = Part.Face(polygonwire)
    return(polygon)

def p_cylinder_action(p):
    'cylinder_action : cylinder LPAREN assign COMMA assign COMMA assign COMMA assign COMMA assign COMMA assign COMMA center EQ boolean RPAREN SEMICOL'
    print "Cylinder"
    h = float(p[9])
    r1 = float(p[11])
    r2 = float(p[13])
    print p[9] + ' : ' + p[11] + ' : ' + p[13]
    if ( r1 == r2 ):   
        print "Make Cylinder"
        n = int(p[3])
        fnmax = FreeCAD.ParamGet(\
            "User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetInt('useMaxFN')
        if n < 3 or fnmax != 0 and n > fnmax:
            mycyl=doc.addObject("Part::Cylinder",p[1])
            mycyl.Height = h
            mycyl.Radius = r1
        else :
            print "Make Prism"
            mycyl=doc.addObject("Part::Extrusion","prism")
            mycyl.Dir = (0,0,h)
            try :
                import Draft
                mycyl.Base = Draft.makePolygon(n,r1)
            except :
                # If Draft can't import (probably due to lack of Pivy on Mac and
                # Linux builds of FreeCAD), this is a fallback.
                # or old level of FreeCAD
                print "Draft makePolygon Failed, falling back on manual polygon"
                mycyl.Base = myPolygon(n,r1)

            else :
                pass
                    
            mycyl.Base.ViewObject.hide()
            # mycyl.Solid = True
            
    else:
        print "Make Cone"
        mycyl=doc.addObject("Part::Cone",p[1])
        mycyl.Height = h
        mycyl.Radius1 = r1
        mycyl.Radius2 = r2
    print "Center = "+str(p[17])
    if p[17]=='true' :
       center(mycyl,0,0,h)  
    if False :  
#   Does not fix problemfile or beltTighener although later is closer       
        newobj=doc.addObject("Part::FeaturePython",'RefineCylinder')
        RefineShape(newobj,mycyl)
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
            GetBool('useViewProviderTree'):
            from OpenSCADFeatures import ViewProviderTree
            ViewProviderTree(newobj.ViewObject)
        else:
            newobj.ViewObject.Proxy = 0
        mycyl.ViewObject.hide()   
        p[0] = [newobj]
    else :
        p[0] = [mycyl]
    print "End Cylinder"
    
    
def p_cube_action(p):
    'cube_action : cube LPAREN size EQ size_vector COMMA center EQ boolean RPAREN SEMICOL'
    global doc
    l = float(p[5][0])
    w = float(p[5][1])
    h = float(p[5][2])
    print "cube : "+p[5][0] + ' : ' + p[5][1] +' : '+ p[5][2]
    mycube=doc.addObject('Part::Box',p[1])
    mycube.Length=l
    mycube.Width=w
    mycube.Height=h
    print "Center = "+str(p[9])
    if p[9]=='true' :
       center(mycube,l,w,h);                                   
    p[0] = [mycube]
    print "End Cube"

def p_circle_action(p) :
    'circle_action : circle LPAREN assign COMMA assign COMMA assign COMMA assign RPAREN SEMICOL'
    print "Circle : "+str(p[9])
    r = float(p[9])
    n = int(p[3])
    fnmax = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD").\
        GetInt('useMaxFN')
    import Draft
    if n == 0 or fnmax != 0 and n > fnmax:
       mycircle = Draft.makeCircle(r)
       #mycircle = doc.addObject('Part::Circle',p[1])
       #mycircle.Radius = r
    else :
       mycircle = Draft.makePolygon(n,r)
    print "Push Circle"   
    p[0] = [mycircle]

def p_square_action(p) :
    'square_action : square LPAREN size EQ 2d_point COMMA center EQ boolean RPAREN SEMICOL'
    print "Square"
    x = float(p[5][0])
    y = float(p[5][1])
    mysquare = doc.addObject('Part::Plane',p[1])
    mysquare.Length=x
    mysquare.Width=y
    if p[9]=='true' :
       center(mysquare,x,y,0)
    p[0] = [mysquare]

def convert_points_list_to_vector(l):
    v = []
    for i in l :
        print i
        v.append(FreeCAD.Vector(i[0],i[1]))
    print v
    return(v)


def p_polygon_action_nopath(p) :
    'polygon_action_nopath : polygon LPAREN points EQ OSQUARE points_list_2d ESQUARE COMMA paths EQ undef COMMA assign RPAREN SEMICOL'
    print "Polygon"
    print p[6]
    v = convert_points_list_to_vector(p[6])
    mypolygon = doc.addObject('Part::Feature',p[1])
    print "Make Parts"
    # Close Polygon
    v.append(v[0])
    parts = Part.makePolygon(v)
    print "update object"
    mypolygon.Shape = Part.Face(parts)
    p[0] = [mypolygon]       

def p_polygon_action_plus_path(p) :
    'polygon_action_plus_path : polygon LPAREN points EQ OSQUARE points_list_2d ESQUARE COMMA paths EQ OSQUARE path_set ESQUARE COMMA assign RPAREN SEMICOL'
    print "Polygon with Path"
    print p[6]
    v = convert_points_list_to_vector(p[6])
    print "Path Set List"
    print p[12]
    for i in p[12] :
         print i
         mypolygon = doc.addObject('Part::Feature','wire')
         path_list = []
         for j in i :
             j = int(j)
             print j
             path_list.append(v[j])
#        Close path
         path_list.append(v[int(i[0])])
         print 'Path List'
         print path_list
         wire = Part.makePolygon(path_list)
         mypolygon.Shape = Part.Face(wire)
         p[0] = [mypolygon]
#        This only pushes last polygon

def make_face(v1,v2,v3):
    wire = Part.makePolygon([v1,v2,v3,v1])
    face = Part.Face(wire)
    return face

def p_polyhedron_action(p) :
    'polyhedron_action : polyhedron LPAREN points EQ OSQUARE points_list_3d ESQUARE COMMA triangles EQ OSQUARE points_list_3d ESQUARE COMMA assign RPAREN SEMICOL'
    print "Polyhedron Points"
    v = []
    for i in p[6] :
        print i
        v.append(FreeCAD.Vector(float(i[0]),float(i[1]),float(i[2])))
    print v
    print "Polyhedron triangles"
    print p[12]
    faces_list = []    
    mypolyhed = doc.addObject('Part::Feature',p[1])
    for i in p[12] :
        print i
        f = make_face(v[int(i[0])],v[int(i[1])],v[int(i[2])])
        faces_list.append(f)
    shell=Part.makeShell(faces_list)
    mypolyhed.Shape=Part.Solid(shell) 
    p[0] = [mypolyhed]       

def p_projection_action(p) :
    'projection_action : projection LPAREN cut EQ boolean COMMA assign RPAREN OBRACE block_list EBRACE'
    print 'Projection'
    from PyQt4 import QtGui
    QtGui.QMessageBox.critical(None, "Projection Not yet Coded waiting for Peter Li"," Press OK")

