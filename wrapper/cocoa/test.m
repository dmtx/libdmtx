#import <Cocoa/Cocoa.h>
#import "SHDataMatrixReader.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        NSLog(@"Usage : test path_to_image");
        return 0;
    }

    NSAutoreleasePool * ARP = [[NSAutoreleasePool alloc] init];

    NSImage * image = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];

    SHDataMatrixReader * reader = [SHDataMatrixReader sharedDataMatrixReader];

    NSString * decodedMessage = [reader decodeBarcodeFromImage:image];

    NSLog(@"Decoded : %@", decodedMessage);

    [image release];
    return 0;
}
