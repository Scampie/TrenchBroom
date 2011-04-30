//
//  EntityBoundsRenderer.m
//  TrenchBroom
//
//  Created by Kristian Duske on 25.04.11.
//  Copyright 2011 TU Berlin. All rights reserved.
//

#import "EntityBoundsRenderer.h"
#import "Entity.h"
#import "VBOBuffer.h"
#import "VBOMemBlock.h"
#import "EntityDefinitionManager.h"
#import "EntityDefinition.h"

static NSString* QuadsBlockKey = @"bounds quads";
static NSString* LinesBlockKey = @"bounds lines";

@interface EntityBoundsRenderer (private)

- (void)validate;

@end

@implementation EntityBoundsRenderer (private)

- (void) validate {
    if ([removedEntities count] > 0) {
        NSEnumerator* entityEn = [removedEntities objectEnumerator];
        id <Entity> entity;
        while ((entity = [entityEn nextObject])) {
            [entity setMemBlock:nil forKey:QuadsBlockKey];
            [entity setMemBlock:nil forKey:LinesBlockKey];
        }
    }

    if ([addedEntities count] > 0) {
        TVector3f c, t, o;
        float defaultColor[] = {1, 1, 1};
        
        NSEnumerator* entityEn = [addedEntities objectEnumerator];
        id <Entity> entity;
        
        [quads activate];
        [quads mapBuffer];
        while ((entity = [entityEn nextObject])) {
            VBOMemBlock* quadsBlock = [entity memBlockForKey:QuadsBlockKey];
            if (quadsBlock == nil) {
                quadsBlock = [quads allocMemBlock:2 * 4 * (3 + 3) * sizeof(float)];
                [entity setMemBlock:quadsBlock forKey:QuadsBlockKey];
            }
            
            if ([quadsBlock state] != BS_USED_VALID) {
                EntityDefinition* definition = [definitionManager definitionForName:[entity propertyForKey:@"classname"]];
                TBoundingBox* bounds = definition != nil && [definition type] == EDT_POINT ? [definition bounds] : [entity bounds];
                float* color = definition != nil ? [definition color] : defaultColor;
                
                NSString* originStr = [entity propertyForKey:@"origin"];
                if (originStr != nil)
                    parseV3f(originStr, NSMakeRange(0, [originStr length]), &o);
                else
                    o = NullVector;
                
                int offset = 0;
                c.x = color[0];
                c.y = color[1];
                c.z = color[2];
                
                addV3f(&bounds->min, &o, &t);
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];

                t.x = o.x + bounds->max.x;
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];

                t.y = o.y + bounds->max.y;
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];

                t.x = o.x + bounds->min.x;
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];
                
                addV3f(&bounds->max, &o, &t);
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];

                t.y = o.y + bounds->min.y;
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];
                
                t.x = o.x + bounds->min.x;
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];
                
                t.y = o.y + bounds->max.y;
                offset = [quadsBlock writeVector3f:&c offset:offset];
                offset = [quadsBlock writeVector3f:&t offset:offset];
                
                [quadsBlock setState:BS_USED_VALID];
            }
        }
        [quads pack];
        [quads unmapBuffer];
        [quads deactivate];

        [lines activate];
        [lines mapBuffer];
        entityEn = [addedEntities objectEnumerator];
        while ((entity = [entityEn nextObject])) {
            VBOMemBlock* linesBlock = [entity memBlockForKey:LinesBlockKey];
            if (linesBlock == nil) {
                linesBlock = [lines allocMemBlock:2 * 4 * (3 + 3) * sizeof(float)];
                [entity setMemBlock:linesBlock forKey:LinesBlockKey];
            }
            
            if ([linesBlock state] != BS_USED_VALID) {
                EntityDefinition* definition = [definitionManager definitionForName:[entity propertyForKey:@"classname"]];
                TBoundingBox* bounds = definition != nil && [definition type] == EDT_POINT ? [definition bounds] : [entity bounds];
                float* color = definition != nil ? [definition color] : defaultColor;
                
                NSString* originStr = [entity propertyForKey:@"origin"];
                if (originStr != nil)
                    parseV3f(originStr, NSMakeRange(0, [originStr length]), &o);
                else
                    o = NullVector;
                
                int offset = 0;
                c.x = color[0];
                c.y = color[1];
                c.z = color[2];

                addV3f(&bounds->min, &o, &t);
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                t.z = o.z + bounds->max.z;
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                addV3f(&bounds->min, &o, &t);
                t.x = o.x + bounds->max.x;
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                t.z = o.z + bounds->max.z;
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                addV3f(&bounds->max, &o, &t);
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                t.z = o.z + bounds->min.z;
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                addV3f(&bounds->max, &o, &t);
                t.x = o.x + bounds->min.x;
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                t.z = o.z + bounds->min.z;
                offset = [linesBlock writeVector3f:&c offset:offset];
                offset = [linesBlock writeVector3f:&t offset:offset];
                
                [linesBlock setState:BS_USED_VALID];
            }
        }
        [lines pack];
        [lines unmapBuffer];
        [lines deactivate];
    }
}

@end

@implementation EntityBoundsRenderer

- (id)init {
    if (self = [super init]) {
        quads = [[VBOBuffer alloc] initWithTotalCapacity:0xFFFF];
        lines = [[VBOBuffer alloc] initWithTotalCapacity:0xFFFF];
        
        addedEntities = [[NSMutableSet alloc] init];
        removedEntities = [[NSMutableSet alloc] init];
    }
    
    return self;
}

- (id)initWithEntityDefinitionManager:(EntityDefinitionManager *)theDefinitionManager {
    NSAssert(theDefinitionManager != nil, @"entity definition manager must not be nil");
    
    if (self = [self init]) {
        definitionManager = [theDefinitionManager retain];
    }
    
    return self;
}

- (void)addEntity:(id <Entity>)entity {
    NSAssert(entity != nil, @"entity must not be nil");
    [addedEntities addObject:entity];
}

- (void)removeEntity:(id <Entity>)entity {
    NSAssert(entity != nil, @"entity must not be nil");
    
    if ([addedEntities containsObject:entity])
        [addedEntities removeObject:entity];
    else
        [removedEntities addObject:entity];
}

- (void)render {
    [self validate];
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);
    
    int quadCount = ([quads totalCapacity] - [quads freeCapacity]) / (6 * sizeof(float));
    [quads activate];
    glInterleavedArrays(GL_C3F_V3F, 0, 0);
    glDrawArrays(GL_QUADS, 0, quadCount);
    [quads deactivate];
    
    int lineCount = ([lines totalCapacity] - [lines freeCapacity]) / (6 * sizeof(float));
    [lines activate];
    glInterleavedArrays(GL_C3F_V3F, 0, 0);
    glDrawArrays(GL_LINES, 0, lineCount);
    [lines deactivate];
    
    glEnable(GL_CULL_FACE);
}

- (void)dealloc {
    [quads release];
    [lines release];
    [addedEntities release];
    [removedEntities release];
    [definitionManager release];
    [super dealloc];
}

@end