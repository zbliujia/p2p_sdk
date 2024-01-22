#import "Cross.h"
#include <string>
//#include <p2p.h>
#include <jzsdk.h>

@implementation Cross

- (void)startServer {
    NSThread *thread = [[NSThread alloc] initWithTarget:self selector:@selector(start) object:nil];
    [thread start];
}

- (void)start {
    const char *user_token = "11111111-1111-1111-1111-111111111111";
    const char *device_token = "2B40679B-E676-DC05-AF49-F329D1C5FB36";
    if (0 != JZSDK_Init(user_token)) {
        printf("JZSDK_Init: failed\n");
        return;
    }
    sleep(2);
    if (0 != JZSDK_StartSession(device_token)) {
        printf("JZSDK_StartSession: failed\n");
        return;
    }

    while (true) {
        std::string url_prefix = std::string(JZSDK_GetUrlPrefix());
        if (url_prefix.empty()) {
            printf("url_prefix: is empty\n");
            sleep(1);
        } else {
            printf("url_prefix:%s\n", url_prefix.c_str());
            break;
        }
    }
}

- (void)endServer {
    JZSDK_StopSession();
    JZSDK_Fini();
}

@end
