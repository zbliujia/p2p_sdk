#import "Cross.h"
#include <string>
//#include <p2p.h>
#include <jzsdk.h>

@implementation Cross

- (NSString *)startServer:(NSString *)user withDevice:(NSString *)device {
    if (0 != JZSDK_Init([user cStringUsingEncoding:NSUTF8StringEncoding])) {
        printf("JZSDK_Init: failed\n");
        return @"";
    }
    sleep(2);
    if (0 != JZSDK_StartSession([device cStringUsingEncoding:NSUTF8StringEncoding])) {
        printf("JZSDK_StartSession: failed\n");
        return @"";
    }

    int whileCount = 0;
    while (true) {
        // 最多等待10s
        if (whileCount >= 10) {
            break;
        }
        std::string url_prefix = std::string(JZSDK_GetUrlPrefix());
        if (url_prefix.empty()) {
            printf("url_prefix: is empty\n");
            sleep(1);
            whileCount++;
        } else {
            printf("url_prefix:%s\n", url_prefix.c_str());
            return [NSString stringWithCString:url_prefix.c_str() encoding:NSUTF8StringEncoding];
        }
    }
    return @"";
}

- (void)endServer {
    JZSDK_StopSession();
    JZSDK_Fini();
}

@end
