#import "Cross.h"
#include <string>
#include <url_signature.h>
#include <libt2u.h>

@implementation Cross
- (NSString*)signatureUrl:(NSString *)url{
    std::string str = [url UTF8String];
    std::string result = SignatureUrl(str);
    NSString *newUrl = [NSString stringWithUTF8String:result.c_str()];
    return newUrl;
}

- (void)callLib {
    t2u_init("", 0, "");
}

@end
