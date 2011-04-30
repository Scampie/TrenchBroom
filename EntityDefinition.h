//
//  EntityDefinition.h
//  TrenchBroom
//
//  Created by Kristian Duske on 21.04.11.
//  Copyright 2011 TU Berlin. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Math.h"

typedef enum {
    EDT_POINT,
    EDT_BRUSH,
    EDT_BASE
} EEntityDefinitionType;

@interface EntityDefinition : NSObject {
    @private
    EEntityDefinitionType type;
    NSString* name;
    float color[3];
    TBoundingBox bounds;
    NSMutableArray* flags;
    NSArray* properties;
    NSString* description;
}

- (id)initBaseDefinitionWithName:(NSString *)theName flags:(NSArray *)theFlags properties:(NSArray *)theProperties;
- (id)initPointDefinitionWithName:(NSString *)theName color:(float *)theColor bounds:(TBoundingBox *)theBounds flags:(NSArray *)theFlags properties:(NSArray *)theProperties description:(NSString *)theDescription;
- (id)initBrushDefinitionWithName:(NSString *)theName color:(float *)theColor flags:(NSArray *)theFlags properties:(NSArray *)theProperties description:(NSString *)theDescription;

- (EEntityDefinitionType)type;
- (NSString *)name;
- (float *)color;
- (TBoundingBox *)bounds;
- (NSArray *)flags;
- (NSArray *)properties;
- (NSString *)description;

- (NSComparisonResult)compareByName:(EntityDefinition *)definition;

@end