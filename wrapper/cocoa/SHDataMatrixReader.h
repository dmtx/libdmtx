/*
Cocoa wrapper for libdmtx

Created by Stefan Hafeneger on 28.05.08.
Copyright (C) 2008 CocoaHeads Aachen. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* $Id$ */

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

@interface SHDataMatrixReader : NSObject {

}
#pragma mark Allocation
+ (id)sharedDataMatrixReader;
#pragma mark Instance
#if TARGET_OS_IPHONE
- (NSString *)decodeBarcodeFromImage:(UIImage *)image;
#else
- (NSString *)decodeBarcodeFromImage:(NSImage *)image;
#endif
@end
