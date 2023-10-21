#import "Cross.h"
#include <string>
#include <url_signature.h>
#include <libt2u.h>
#include <p2p.h>

@implementation Cross
- (NSString*)signatureUrl:(NSString *)url{
    std::string str = [url UTF8String];
    std::string result = SignatureUrl(str);
    NSString *newUrl = [NSString stringWithUTF8String:result.c_str()];
    return newUrl;
}

- (void)callLib {
    NSThread *thread = [[NSThread alloc] initWithTarget:self selector:@selector(start) object:nil];
    [thread start];
}

- (void)start {
    beginListen(9900);
}

@end
