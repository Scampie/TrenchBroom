//
//  Face.h
//  TrenchBroom
//
//  Created by Kristian Duske on 30.01.10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Face.h"

typedef enum {
    XY, XZ, YZ
} EPlaneType;

@class MutableBrush;
@class Vector2f;
@class Vector3f;
@class Vector3i;
@class HalfSpace3D;
@class Plane3D;
@class Matrix4f;
@class PickingHit;
@class Ray3D;
@class FaceFigure;

@interface MutableFace : NSObject <Face> {
    @private
    MutableBrush* brush;
    NSNumber* faceId;
    
	Vector3i* point1;
	Vector3i* point2;
	Vector3i* point3;
	
	NSMutableString* texture;
	int xOffset;
	int yOffset;
	float rotation;
	float xScale;
	float yScale;
    
    Vector3f* norm;
    HalfSpace3D* halfSpace;
    
    int bestAxis;
    Vector3f* texAxisX;
    Vector3f* texAxisY;
    
    // transforms surface coordinates to world coordinates
    Matrix4f* surfaceMatrix;
    Matrix4f* worldMatrix; // inverse of surface matrix
}

- (id)initWithPoint1:(Vector3i *)aPoint1 point2:(Vector3i *)aPoint2 point3:(Vector3i *)aPoint3 texture:(NSString *)aTexture;
- (id)initWithTemplate:(id <Face>)theTemplate;

- (void)setBrush:(MutableBrush *)theBrush;
- (void)setPoint1:(Vector3i *)thePoint1 point2:(Vector3i *)thePoint2 point3:(Vector3i *)thePoint3;
- (void)setTexture:(NSString *)name;
- (void)setXOffset:(int)offset;
- (void)setYOffset:(int)offset;
- (void)setRotation:(float)angle;
- (void)setXScale:(float)factor;
- (void)setYScale:(float)factor;
- (void)translateOffsetsX:(int)x y:(int)y;
- (void)translateBy:(Vector3i *)theDelta;
@end