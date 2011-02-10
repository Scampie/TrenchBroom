//
//  FaceOffsetTool.h
//  TrenchBroom
//
//  Created by Kristian Duske on 05.02.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class Face;
@class SelectionManager;
@class Ray3D;

@interface FaceOffsetTool : NSObject {
    @private
    Face* face;
    SelectionManager* selectionManager;
}

/*!
    @function
    @abstract   Initializes the face offset tool with the specified face.
    @param      theFace The face which is edited through this tool.
    @param      theSelectionManager The selection manager.
    @result     The initialized face offset tool.
*/
- (id)initWithFace:(Face *)theFace selectionManager:(SelectionManager *)theSelectionManager;

/*!
    @function
    @abstract   Renders the visual handle for this tool
*/
- (void)render;

- (BOOL)hitByRay:(Ray3D *)theRay;

@end